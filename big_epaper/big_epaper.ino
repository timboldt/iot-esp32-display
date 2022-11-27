#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Fonts/FreeSans18pt7b.h>
#include <GxEPD2_BW.h>

// Waveshare EPD.
#define EPD_BUSY 14
#define EPD_RESET 32
#define EPD_DC 33
#define EPD_CS 15

#define MAX_DISPLAY_BUFFER_SIZE 15000ul  // ~15k is a good compromise
#define MAX_HEIGHT(EPD)                                        \
    (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
         ? EPD::HEIGHT                                         \
         : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

// Waveshare 4.2" B/W EPD.
GxEPD2_BW<GxEPD2_420, MAX_HEIGHT(GxEPD2_420)> display(GxEPD2_420(EPD_CS, EPD_DC,
                                                                 EPD_RESET,
                                                                 EPD_BUSY));

void setup() {
    Serial.begin(115200);

    // Initialize the display: Waveshare 4.2" B/W EPD.
    Serial.println("Initializing display...");
    display.init();

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Writing to display...");
    display.firstPage();
    do {
        display.fillScreen(GxEPD_WHITE);

        char label[] = "Hello world!";
        display.setFont(&FreeSans18pt7b);
        int16_t fx, fy;
        uint16_t fw, fh;
        display.getTextBounds(label, 50, 50, &fx, &fy, &fw, &fh);
        // Center justified: display.setCursor(200 - fw / 2, fh);
        display.setCursor(200 - fw / 2, fh + 100);
        display.setTextColor(GxEPD_BLACK);
        display.print(label);
    } while (display.nextPage());
    display.hibernate();
    Serial.println("Write to display complete.");
    digitalWrite(LED_BUILTIN, LOW);
    delay(180000);
}
