#include "adafruit_io.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "arduino_secrets.h"

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
                  size_t num_vals, float vals[], String* description) {
    *description = "NO DATA";
    if (!client) {
        return 0;
    }

    size_t val_count = 0;
    DynamicJsonDocument doc(32768);

    HTTPClient https;
    const String url = "https://io.adafruit.com/api/v2/" ADAFRUIT_IO_USERNAME
                       "/feeds/" +
                       feed_name + "/data/chart?hours=120&resolution=30";
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
                val_count = min(num_vals, data.size());
                for (size_t i = 0; i < val_count; i++) {
                    JsonArray elem = data[i];
                    vals[i] = elem[1].as<float>();
                }
                *description = doc["feed"]["name"].as<String>();
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

bool send_data(WiFiClientSecure* client, const String& feed_name, float val) {
    bool is_ok = false;

    if (!client) {
        return is_ok;
    }

    HTTPClient https;
    const String url = "https://io.adafruit.com/api/v2/" ADAFRUIT_IO_USERNAME
                       "/feeds/" +
                       feed_name + "/data";
    https.addHeader("X-AIO-Key", ADAFRUIT_IO_KEY);
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    if (https.begin(*client, url)) {
        int httpCode = https.POST("value=" + String(val));
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has
            // been handled
            Serial.printf("HTTP POST... code: %d\n", httpCode);

            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                is_ok = true;
            }
        } else {
            Serial.printf("HTTPS POST failed, error: %s\n",
                          https.errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf("Failed to make HTTP call to %s", url.c_str());
    }

    return is_ok;
}