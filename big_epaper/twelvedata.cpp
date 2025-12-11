#include "twelvedata.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "arduino_secrets.h"

void get_time(WiFiClientSecure* client, String* time) {
    *time = "UNKNOWN";
    if (!client) {
        return;
    }
    HTTPClient http;
    const String url = "http://worldtimeapi.org/api/timezone/America/Los_Angeles";

    if (http.begin(url)) {
        int httpCode = http.GET();
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                DynamicJsonDocument doc(2048);
                DeserializationError error = deserializeJson(doc, http.getString());

                if (!error) {
                    String datetime = doc["datetime"].as<String>();
                    // Parse ISO 8601 datetime and format it
                    // Format: "December 10, 2025 3:45 PM"
                    if (datetime.length() >= 19) {
                        int year = datetime.substring(0, 4).toInt();
                        int month = datetime.substring(5, 7).toInt();
                        int day = datetime.substring(8, 10).toInt();
                        int hour = datetime.substring(11, 13).toInt();
                        int minute = datetime.substring(14, 16).toInt();

                        const char* months[] = {"", "January", "February", "March", "April", "May", "June",
                                               "July", "August", "September", "October", "November", "December"};

                        String ampm = "AM";
                        int display_hour = hour;
                        if (hour >= 12) {
                            ampm = "PM";
                            if (hour > 12) display_hour = hour - 12;
                        }
                        if (display_hour == 0) display_hour = 12;

                        *time = String(months[month]) + " " + String(day) + ", " +
                               String(year) + " " + String(display_hour) + ":" +
                               (minute < 10 ? "0" : "") + String(minute) + " " + ampm;
                    }
                }
            }
        } else {
            Serial.printf("HTTP GET failed, error: %s\r\n",
                         http.errorToString(httpCode).c_str());
        }
        http.end();
    }
}

size_t fetch_data(WiFiClientSecure* client, const String& symbol,
                  size_t hours, size_t num_vals, float vals[],
                  String* description) {
    *description = "NO DATA";
    if (!client) {
        return 0;
    }

    size_t val_count = 0;
    DynamicJsonDocument doc(32768);

    // Determine appropriate interval based on hours requested
    String interval;
    int outputsize = num_vals;

    if (hours <= 24) {
        interval = "1h";
        outputsize = min((int)num_vals, (int)hours);
    } else if (hours <= 24 * 7) {
        interval = "4h";
        outputsize = min((int)num_vals, (int)(hours / 4));
    } else if (hours <= 24 * 30) {
        interval = "1day";
        outputsize = min((int)num_vals, (int)(hours / 24));
    } else {
        interval = "1day";
        outputsize = min((int)num_vals, (int)(hours / 24));
    }

    // Limit to reasonable size
    if (outputsize > 300) outputsize = 300;

    // Convert symbol format if needed (e.g., "BTC/USD" stays as is, "TSLA" stays as is)
    String api_symbol = symbol;

    HTTPClient https;
    const String url = "https://api.twelvedata.com/time_series?symbol=" +
                       api_symbol + "&interval=" + interval +
                       "&outputsize=" + String(outputsize) +
                       "&apikey=" TWELVEDATA_API_KEY;

    Serial.printf("HTTP GET %s\r\n", url.c_str());

    if (https.begin(*client, url)) {
        int httpCode = https.GET();
        if (httpCode > 0) {
            Serial.printf("HTTP GET... code: %d\r\n", httpCode);

            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                DeserializationError error =
                    deserializeJson(doc, https.getString());

                if (error) {
                    Serial.print("Could not parse data from HTTP request: ");
                    Serial.println(error.c_str());
                    return 0;
                }

                // Check for API error
                if (doc.containsKey("status") && doc["status"] == "error") {
                    Serial.printf("API error: %s\r\n", doc["message"].as<const char*>());
                    return 0;
                }

                JsonArray values = doc["values"];
                if (!values.isNull()) {
                    val_count = min(num_vals, values.size());
                    // Reverse the array since TwelveData returns newest first
                    for (size_t i = 0; i < val_count; i++) {
                        JsonObject elem = values[val_count - 1 - i];
                        vals[i] = elem["close"].as<float>();
                    }

                    // Get description from metadata
                    if (doc.containsKey("meta")) {
                        *description = doc["meta"]["symbol"].as<String>();
                        if (doc["meta"].containsKey("currency")) {
                            *description += " (" + doc["meta"]["currency"].as<String>() + ")";
                        }
                    } else {
                        *description = api_symbol;
                    }
                }
            }
        } else {
            Serial.printf("HTTPS GET failed, error: %s\r\n",
                          https.errorToString(httpCode).c_str());
        }
        https.end();
    } else {
        Serial.printf("Failed to make HTTP call to %s\r\n", url.c_str());
    }

    return val_count;
}
