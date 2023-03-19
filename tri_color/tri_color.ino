#include <Adafruit_GFX.h>
#include <Adafruit_ThinkInk.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <algorithm>
#include <stdint.h>

#include "adafruit_io.h"
#include "arduino_secrets.h"
#include "graph.h"

const size_t MAX_VALS = 300;

#define EPD_BUSY -1
#define EPD_RESET -1
#define EPD_DC 33
#define EPD_CS 15
#define SRAM_CS 32

// 2.9" Tri-Color Featherwing
ThinkInk_290_Tricolor_Z10 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

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
    while (wifi.run() != WL_CONNECTED) {
        Serial.print(">");
        delay(1000);
    }
    Serial.println("Connected.");

    Serial.println("Initializing display...");
    display.begin();

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

    send_data(client, "tricolor-battery", battery_voltage());

    Config config;
    if (get_config(client, &config)) {
        config.rows = std::min((int16_t)2, std::max((int16_t)1, config.rows));
        config.cols = std::min((int16_t)4, std::max((int16_t)1, config.cols));
    } else {
        Serial.println("Failed to read config. Reverting to default values.");
        config.days = 7;
        config.rows = 1;
        config.cols = 3;
        config.feeds = {"finance.coinbase-btc-usd", "finance.kraken-usdtzusd",
                        "finance.bitfinex-ustusd"};
    }

    Serial.println("Writing to display...");
    display.clearBuffer();

    const int16_t graph_width = display.width() / config.cols;
    const int16_t graph_height = display.height() / config.rows;
    for (int16_t x = 0; x < display.width() / graph_width; x++) {
        for (int16_t y = 0; y < display.height() / graph_height; y++) {
            String feed_name;
            size_t label_idx = x + y * display.width() / graph_width;
            if (label_idx < config.feeds.size()) {
                feed_name = config.feeds[label_idx];
            }
            Serial.printf("Processing graph (%d, %d)\n", x, y);
            if (feed_name.length() > 0) {
                String name;
                float vals[MAX_VALS];
                size_t val_count =
                    fetch_data(client, feed_name, MAX_VALS, vals, &name);
                uint16_t line_color = ((x + y) % 2 == 0) ? EPD_BLACK : EPD_RED;
                draw_graph(&display, name, line_color, x * graph_width,
                           y * graph_height, graph_width, graph_height,
                           val_count, vals);
            }
        }
    }

    String time;
    get_time(client, &time);
    show_status(&display, time, battery_voltage(), 2, 8);
    show_battery_icon(&display, battery_voltage());

    // Display values and then power down display (sleep == true).
    display.display(true);
    Serial.println("Write to display complete.");

    digitalWrite(LED_BUILTIN, LOW);

    //
    // Power down.
    //
    pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_I2C_POWER, LOW);
    if (battery_voltage() > 3.9) {
        esp_sleep_enable_timer_wakeup(3 * 60 * 1000000ULL);
    } else {
        esp_sleep_enable_timer_wakeup(60 * 60 * 1000000ULL);
    }
    esp_deep_sleep_start();

    // we never reach here
    delay(3 * 60 * 1000);
}
