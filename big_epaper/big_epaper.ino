#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <stdint.h>

#include "adafruit_io.h"
#include "arduino_secrets.h"
#include "display.h"
#include "graph.h"

const size_t MAX_VALS = 300;

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

    // Initialize the display: Waveshare 4.2" B/W EPD.
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

    Serial.println("Writing to display...");
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        String labels[] = {"",
                           "weather.temp",
                           "mbr.temperature",
                           "mbr.pressure",
                           "finance.coinbase-btc-usd",
                           "finance.kraken-usdtzusd",

                           "finance.bitfinex-ustusd",
                           "mbr.humidity",
                           "mbr.abs-humidity",
                           "mbr-sgp30.co2",
                           "mbr-sgp30.tvoc",
                           "mbr.lux-db"};

        const int16_t graph_width = 200;
        const int16_t graph_height = 100;
        for (int16_t x = 0; x < display.width() / graph_width; x++) {
            for (int16_t y = 0; y < display.height() / graph_height; y++) {
                String feed_name =
                    labels[x + y * display.width() / graph_width];
                Serial.printf("Processing graph (%d, %d)\n", x, y);
                if (feed_name.length() == 0) {
                    String time;
                    get_time(client, &time);
                    display.setFont(&FreeSans9pt7b);
                    display.setTextColor(GxEPD_BLACK);
                    display.setCursor(x * graph_width + 2,
                                      y * graph_height + 60);
                    display.print(time);
                    display.setCursor(x * graph_width + 2,
                                      y * graph_height + graph_height * 4 / 5);
                    display.printf("%.2f V", battery_voltage());
                } else {
                    // TODO: It is horrible that we are refetching if the full
                    // display can't be drawn in one step. Move the data
                    // fetch outside the loop.
                    String name;
                    float vals[MAX_VALS];
                    size_t val_count =
                        fetch_data(client, feed_name, MAX_VALS, vals, &name);
                    draw_graph(name, x * graph_width, y * graph_height,
                               graph_width, graph_height, val_count, vals);
                }
            }
        }
    } while (display.nextPage());
    display.hibernate();
    Serial.println("Write to display complete.");
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
    digitalWrite(NEOPIXEL_I2C_POWER, LOW);
    if (battery_voltage() > 3.8) {
        esp_sleep_enable_timer_wakeup(3 * 60 * 1000000ULL);
    } else {
        esp_sleep_enable_timer_wakeup(15 * 60 * 1000000ULL);
    }
    esp_deep_sleep_start();
    // we never reach here
    delay(3 * 60 * 1000);
}
