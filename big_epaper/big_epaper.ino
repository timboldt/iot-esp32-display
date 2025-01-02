#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <stdint.h>

#include <algorithm>

#include "adafruit_io.h"
#include "arduino_secrets.h"
#include "graph.h"

const size_t MAX_VALS = 300;

const int SLEEP_SECONDS = 6 * 3600;

#define MAX_DISPLAY_BUFFER_SIZE 15000ul  // ~15k is a good compromise
#define MAX_HEIGHT(EPD)                                        \
    (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
         ? EPD::HEIGHT                                         \
         : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

#define EPD_BUSY 14
#define EPD_RESET 32
#define EPD_DC 33
#define EPD_CS 15

// Waveshare 4.2" B/W EPD.
GxEPD2_BW<GxEPD2_420, MAX_HEIGHT(GxEPD2_420)> display(GxEPD2_420(EPD_CS, EPD_DC,
                                                                 EPD_RESET,
                                                                 EPD_BUSY));

WiFiMulti wifi;

float battery_voltage() {
    float raw = analogRead(BATT_MONITOR);
    // 50% voltage divider with a 3.3V reference and 4096 divisions.
    return raw * 2.0 * 3.3 / 4096;
}

void setup() {
    Serial.begin(115200);

    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    wifi.addAP(SECRET_WIFI_SSID, SECRET_WIFI_PASSWORD);
    //wifi.addAP(OPEN_WIFI_SSID);
    while (wifi.run() != WL_CONNECTED) {
        Serial.print(">");
        delay(1000);
    }
    Serial.println("Connected.");

    Serial.println("Initializing display...");
    display.init();

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);

    WiFiClientSecure* client = new WiFiClientSecure;
    if (client) {
        client->setInsecure();  // skip verification
    } else {
        Serial.println("Failed to create TCP client.");
    }

    send_data(client, "bigpaper-battery", battery_voltage());

    Config config;
    if (get_config(client, &config)) {
        config.rows = std::min((int16_t)3, std::max((int16_t)1, config.rows));
        config.cols = std::min((int16_t)3, std::max((int16_t)1, config.cols));
    } else {
        Serial.println("Failed to read config. Reverting to default values.");
        config.days = 7;
        config.rows = 3;
        config.cols = 2;
        config.feeds = {"weather.temp",
                        "mbr.abs-humidity",
                        "mbr.pressure",
                        "finance.coinbase-btc-usd",
                        "finance.kraken-usdtzusd",
                        "finance.bitfinex-ustusd",
                        "mbr.temperature",
                        "mbr.humidity",
                        "mbr.abs-humidity",
                        "mbr-sgp30.co2",
                        "mbr-sgp30.tvoc",
                        "mbr.lux-db"};
    }

    Serial.println("Writing to display...");
    display.firstPage();
    display.fillScreen(GxEPD_WHITE);

    const int16_t title_bar_height = 15;
    const int16_t graph_width = display.width() / config.cols;
    const int16_t graph_height =
        (display.height() - title_bar_height) / config.rows;
    for (int16_t x = 0; x < config.cols; x++) {
        for (int16_t y = 0; y < config.rows; y++) {
            String feed_name;
            size_t label_idx = x + y * config.cols;
            if (label_idx < config.feeds.size()) {
                feed_name = config.feeds[label_idx];
            }
            Serial.printf("Processing graph %s (%d, %d)\r\n", feed_name.c_str(),
                          x, y);
            if (feed_name.length() > 0) {
                String name;
                float vals[MAX_VALS];
                size_t val_count =
                    fetch_data(client, feed_name, config.days * 24, MAX_VALS, vals, &name);
                draw_graph(&display, name, GxEPD_BLACK, x * graph_width,
                           y * graph_height + title_bar_height, graph_width,
                           graph_height, val_count, vals);
            }
        }
    }

    String time;
    get_time(client, &time);
    show_status(&display, time, battery_voltage(), 2, 16);
    show_battery_icon(&display, battery_voltage());

    if (display.nextPage()) {
        Serial.println(
            "Display buffer is too small. Doesn't fit on single page.");
    }
    display.hibernate();
    Serial.println("Write to display complete.");

    digitalWrite(LED_BUILTIN, LOW);

    //
    // Power down.
    //
    pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_I2C_POWER, LOW);
    esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * 1000000ULL);
    esp_deep_sleep_start();

    // we never reach here
    delay(3 * 60 * 1000);
}
