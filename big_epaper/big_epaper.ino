#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiMulti.h>
#include <stdint.h>

#include "arduino_secrets.h"
#include "display.h"
#include "graph.h"

WiFiMulti wifi;

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
        {
            HTTPClient https;
            const String& url = "https://www.example.com";
            if (https.begin(*client, url)) {
                int httpCode = https.GET();
                if (httpCode > 0) {
                    // HTTP header has been send and Server response header has
                    // been handled
                    Serial.printf("HTTP GET... code: %d\n", httpCode);

                    if (httpCode == HTTP_CODE_OK ||
                        httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                        String payload = https.getString();
                        Serial.println(payload);
                    }
                } else {
                    Serial.printf("HTTPS GET failed, error: %s\n",
                                  https.errorToString(httpCode).c_str());
                }
                https.end();
            } else {
                Serial.printf("Failed to make HTTP call to %s", url);
            }
        }
    } else {
        Serial.println("Failed to create TCP client.");
    }

    Serial.println("Writing to display...");
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        float vals[] = {
            16638.54190, 16669.98116, 16358.55164, 16861.55528, 16106.95631,
            16563.23739, 16678.76226, 16864.04261, 15066.46833, 15913.20199,
            15642.25906, 15020.62563, 15106.67964, 15024.92191, 16571.18351,
            16001.98618, 15794.06545, 16655.78695, 16963.05022, 16961.88637,
            16472.57971, 15428.88701, 15987.22679, 16418.53457, 16439.55000,
            15221.72126, 16826.08964, 16209.60129, 15958.39792, 15822.10587,
            16804.17842, 15008.03018, 16868.92679, 15075.79270, 15731.01944,
            16317.67791, 15922.29704, 15565.26314, 16821.85772, 15466.74799,
            15815.03529, 16651.00241, 15789.00090, 15270.61038, 16959.20242,
            16985.61689, 16659.62916, 15629.26682, 16544.35313, 16799.81701,
            16498.94905, 16924.70388, 16693.46236, 15080.88698, 16813.28603,
            15383.52871, 16195.50436, 15548.65688, 15361.32341, 16884.41586,
            15451.67901, 15481.44128, 16756.79224, 16367.57833, 15686.24130,
            16537.19697, 15493.40588, 15977.54016, 16302.29786, 16256.97954,
            16900.87430, 15893.32913, 15428.39823, 16470.98999, 16874.98074,
            15741.55647, 15552.69214, 15475.30586, 16428.95662, 15947.34984,
            15052.69041, 16043.89888, 15838.52323, 16325.77347, 15748.91923,
            16125.01253, 15937.78421, 16640.41191, 15961.16713, 16377.07830,
            15278.38841, 15408.43163, 16135.90296, 15985.93070, 15169.83181,
            15924.77735, 15207.59864, 15737.99671, 16139.15736, 16287.47645,
            15932.63431, 15198.41265, 16675.50752, 15446.39062, 15180.52042,
            15840.38413, 16082.44095, 16041.47588, 16908.04800, 15680.01461,
            15254.90406, 16595.19578, 15248.37684, 15427.13633, 16360.05981,
            15899.92842, 15879.77037, 15517.57411, 15828.17007, 15350.67638,
            16390.97764, 16587.67455, 15212.86824, 15489.11097, 15827.05341,
            16179.85523, 16278.08307, 16260.96886, 15168.61698, 15482.89906,
            16577.64351, 16963.09394, 15884.92830, 15210.17204, 15286.83593,
            16391.47510, 16094.49346, 15039.58979, 16704.56755, 15227.40489,
            16943.46137, 15781.58728, 16127.58022, 15988.62103, 15151.21191,
            16941.52656, 16225.15019, 15281.36213, 16845.44770, 15846.53129,
            15205.13848, 16942.46580, 15546.07706, 15220.32132, 15772.11656,
            15446.51592, 16439.52067, 15452.14754, 16345.99264, 16687.62291,
            15125.02955, 16171.69344, 15348.24911, 16978.22548, 16538.40916,
            15113.05242, 15933.21555, 16374.36491, 15309.12229, 16335.51548,
            15960.29305, 16528.36029, 15797.15033, 16567.00798, 15113.32483,
            16252.89691, 15708.13886, 15533.01495, 15553.98058, 15131.76169,
            16186.31118, 15474.97588, 16476.13223, 15918.37202, 16181.02312,
            16921.87185, 16114.56106, 15345.68468, 15298.72128, 16704.79397,
            15368.69832, 16003.28265, 16999.66897, 15254.85826, 15090.09817,
            16750.02748, 16103.80357, 15443.05650, 16526.24101, 16945.87675};

        String labels[] = {"",
                           "finance.coinbase-btc-usd",
                           "finance.kraken-usdtzusd",
                           "finance.bitfinex-ustusd",
                           "mbr.temperature",
                           "mbr.pressure",
                           "mbr.humidity",
                           "mbr.abs-humidity",
                           "mbr-sgp30.co2",
                           "mbr-sgp30.tvoc",
                           "mbr.lux-db",
                           "mbr-tsl2591.infrared",
                           "weather.temp",
                           "weather.humidity",
                           "tricolor-battery",
                           "bigpaper-battery"};
        for (int16_t x = 0; x < 4; x++) {
            for (int16_t y = 0; y < 3; y++) {
                if (x == 0 && y == 0) {
                    // TODO: Show current date/time and voltage.
                } else {
                    draw_graph(labels[x + y * 4], x * 100, y * 100, 100, 100,
                               200, vals);
                }
            }
        }
    } while (display.nextPage());
    display.hibernate();
    Serial.println("Write to display complete.");
    digitalWrite(LED_BUILTIN, LOW);
    delay(180000);
}
