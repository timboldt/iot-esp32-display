#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <stdint.h>

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

void get_time(WiFiClientSecure* client, String* time) {
    *time = "UNKNOWN";
    if (!client) {
        return;
    }
    HTTPClient https;
    const String url = "https://io.adafruit.com/api/v2/" ADAFRUIT_IO_USERNAME
                       "/integrations/time/strftime?fmt=%25I:%25M%20%25p";
    https.addHeader("X-AIO-Key", ADAFRUIT_IO_KEY);
    if (https.begin(*client, url)) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            Serial.printf("HTTP GET... code: %d\n", httpCode);

            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                *time = https.getString();
            }
        } else {
            Serial.printf("HTTPS GET failed, error: %s\n",
                          https.errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf("Failed to make HTTP call to %s", url.c_str());
    }
}

size_t fetch_data(WiFiClientSecure* client, const String& feed_name,
                  float vals[], String* name) {
    *name = "NO DATA";
    if (!client) {
        return 0;
    }

    size_t val_count = 0;
    DynamicJsonDocument doc(32768);

    HTTPClient https;
    const String url = "https://io.adafruit.com/api/v2/" ADAFRUIT_IO_USERNAME
                       "/feeds/" +
                       feed_name + "/data/chart?hours=168&resolution=60";
    https.addHeader("X-AIO-Key", ADAFRUIT_IO_KEY);
    if (https.begin(*client, url)) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has
            // been handled
            Serial.printf("HTTP GET... code: %d\n", httpCode);

            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                DeserializationError error =
                    deserializeJson(doc, https.getString());

                if (error) {
                    Serial.print("Could not parse data from HTTP request: ");
                    Serial.println(error.c_str());
                    return 0;
                }

                JsonArray data = doc["data"];
                val_count = min(MAX_VALS, data.size());
                for (size_t i = 0; i < val_count; i++) {
                    JsonArray elem = data[i];
                    vals[i] = elem[1].as<float>();
                }
                *name = doc["feed"]["name"].as<String>();
            }
        } else {
            Serial.printf("HTTPS GET failed, error: %s\n",
                          https.errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf("Failed to make HTTP call to %s", url.c_str());
    }

    return val_count;
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
                    String name;
                    float vals[MAX_VALS];
                    size_t val_count =
                        fetch_data(client, feed_name, vals, &name);
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
