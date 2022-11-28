#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>
#include <stdint.h>
#include <string.h>

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

void DrawGauge(int16_t x, int16_t y, const String &label, float raw_value,
               float min_value, float max_value) {
    const int16_t gauge_radius = 40;
    const int16_t tick_radius = 35;
    const int16_t needle_radius = 37;
    // const int16_t needle_width = 3;
    const int16_t pin_width = 2;
    float value = raw_value;

    if (value < min_value) {
        value = min_value;
    }
    if (value > max_value) {
        value = max_value;
    }
    float gauge_value = (value - min_value) / (max_value - min_value) - 0.5;

    // Draw the guage face.
    display.drawCircle(x, y, gauge_radius, GxEPD_BLACK);
    for (int i = -5; i <= 5; i++) {
        float dx = sin(i * 2 * PI / 16);
        float dy = cos(i * 2 * PI / 16);
        display.drawLine(x + tick_radius * dx, y - tick_radius * dy,
                         x + gauge_radius * dx, y - gauge_radius * dy,
                         GxEPD_BLACK);
    }
    display.fillRect(x - gauge_radius, y + 10, x + gauge_radius, y + 24,
                     GxEPD_WHITE);
    display.setFont(&FreeSans9pt7b);
    int16_t fx, fy;
    uint16_t fw, fh;
    display.getTextBounds(label, x, y, &fx, &fy, &fw, &fh);
    display.setCursor(x - fw / 2, y + gauge_radius - fh / 2);
    display.setTextColor(GxEPD_BLACK);
    display.print(label);
    char buffer[20];
    if (abs(raw_value) < 100) {
        sprintf(buffer, "%.2f", raw_value);
    } else {
        sprintf(buffer, "%.1f", raw_value);
    }
    String value_string(buffer);
    display.getTextBounds(value_string, x, y, &fx, &fy, &fw, &fh);
    display.setCursor(x - fw / 2, y + gauge_radius * 0.6 - fh / 2);
    display.setTextColor(GxEPD_BLACK);
    display.print(value_string);

    // Draw the needle.
    float angle = gauge_value * 10 / 16 * 2 * PI;
    float nx = sin(angle);
    float ny = cos(angle);
    display.drawLine(x, y, x + needle_radius * nx, y - needle_radius * ny,
                     GxEPD_BLACK);

    // Draw the pin in the center.
    display.fillCircle(x, y, pin_width, GxEPD_BLACK);
    display.drawPixel(x, y, GxEPD_WHITE);
}

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
        display.setCursor(200 - fw / 2, fh + 20);
        display.setTextColor(GxEPD_BLACK);
        display.print(label);

        DrawGauge(100, 100, "Test Label", 0.25, 0.0, 1.0);
    } while (display.nextPage());
    display.hibernate();
    Serial.println("Write to display complete.");
    digitalWrite(LED_BUILTIN, LOW);
    delay(180000);
}
