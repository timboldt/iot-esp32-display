#include "graph.h"

#include <Arduino.h>
#include <Fonts/FreeSans9pt7b.h>
#include <GxEPD2_BW.h>

static void get_min_max(size_t num_values, float values[], float *min_val,
                        float *max_val) {
    *min_val = 0.0;
    *max_val = 0.0;
    if (num_values > 0) {
        *min_val = values[0];
        *max_val = values[0];
        for (size_t i = 1; i < num_values; i++) {
            if (values[i] < *min_val) {
                *min_val = values[i];
            }
            if (values[i] > *max_val) {
                *max_val = values[i];
            }
        }
    }
    if (*min_val == *max_val) {
        float middle = *min_val;
        *min_val = middle - 1;
        *max_val = middle + 1;
    }
}

static void draw_sparkline(Adafruit_GFX *display, int16_t corner_x,
                           int16_t corner_y, int16_t width, int16_t height,
                           size_t num_values, float values[]) {
    float min_val, max_val;
    get_min_max(num_values, values, &min_val, &max_val);

    // Draw the line.
    int16_t prev_x = 0;
    int16_t prev_y = 0;
    for (size_t i = 0; i < num_values; i++) {
        int16_t x = i * width / num_values + corner_x;
        int16_t y =
            height -
            round((values[i] - min_val) / (max_val - min_val) * height) +
            corner_y;
        if (i > 0) {
            display->drawLine(prev_x, prev_y, x, y, GxEPD_BLACK);
        }
        prev_x = x;
        prev_y = y;
    }
}

void draw_graph(Adafruit_GFX *display, const String &label, int16_t corner_x,
                int16_t corner_y, int16_t width, int16_t height,
                size_t num_values, float values[]) {
    const int16_t padding = 5;

    int16_t fx, fy;
    uint16_t fw, fh;
    display->setFont(&FreeSans9pt7b);
    display->getTextBounds(label, corner_x, corner_y, &fx, &fy, &fw, &fh);
    display->setCursor(corner_x, corner_y + height - padding);
    display->setTextColor(GxEPD_BLACK);
    float last_val = values[num_values - 1];
    if (last_val < 2) {
        display->printf("%s: %.4f", label.c_str(), last_val);
    } else if (last_val < 100) {
        display->printf("%s: %.3f", label.c_str(), last_val);
    } else if (last_val < 1000) {
        display->printf("%s: %.f", label.c_str(), last_val);
    } else {
        display->printf("%s: %.1fK", label.c_str(), last_val / 1000);
    }
    draw_sparkline(display, corner_x + padding, corner_y + padding,
                   width - padding * 2, height - fh - padding * 3, num_values,
                   values);
}

void show_status(Adafruit_GFX *display, const String &time,
                 float battery_voltage, int16_t x, int16_t y) {
    display->setFont(&FreeSans9pt7b);
    display->setCursor(x, y);

    if (battery_voltage > 3.4f) {
        display->printf("%.2f V   %s", battery_voltage, time.c_str());
    } else {
        display->printf("LOW BATTERY  %.2f V   %s", battery_voltage, time.c_str());
    }
}

void show_battery_icon(Adafruit_GFX *display, float battery_voltage) {
    const uint16_t BATTERY_WIDTH = 10;
    const uint16_t BATTERY_HEIGHT = 15;
    const uint16_t BATTERY_KNOB_WIDTH = 5;
    const uint16_t BATTERY_KNOB_HEIGHT = 3;
    const uint16_t EDGE_OFFSET = 1;

    const float FILL_MAX = BATTERY_HEIGHT;
    const float VOLTAGE_MIN = 3.3f;
    const float VOLTAGE_MAX = 3.8f;
    const uint16_t fill_size =
        min(FILL_MAX, (battery_voltage - VOLTAGE_MIN) /
                          (VOLTAGE_MAX - VOLTAGE_MIN) * FILL_MAX);

    display->fillRect(display->width() - BATTERY_WIDTH - EDGE_OFFSET,
                      display->height() - fill_size - EDGE_OFFSET,
                      BATTERY_WIDTH, fill_size, GxEPD_BLACK);
    display->drawRect(display->width() - BATTERY_WIDTH - EDGE_OFFSET,
                      display->height() - FILL_MAX - EDGE_OFFSET, BATTERY_WIDTH,
                      BATTERY_HEIGHT, GxEPD_BLACK);
    display->drawRect(
        display->width() - BATTERY_WIDTH / 2 - BATTERY_KNOB_WIDTH / 2 -
            EDGE_OFFSET,
        display->height() - BATTERY_HEIGHT - BATTERY_KNOB_HEIGHT - EDGE_OFFSET,
        BATTERY_KNOB_WIDTH, BATTERY_KNOB_HEIGHT, GxEPD_BLACK);
}