#include "graph.h"

#include <Adafruit_ThinkInk.h>
#include <Arduino.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Picopixel.h>

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

static void draw_sparkline(Adafruit_GFX *display, uint16_t line_color,
                           int16_t corner_x, int16_t corner_y, int16_t width,
                           int16_t height, size_t num_values, float values[]) {
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
            display->drawLine(prev_x, prev_y, x, y, line_color);
        }
        prev_x = x;
        prev_y = y;
    }
}

void draw_graph(Adafruit_GFX *display, const String &label, uint16_t line_color,
                int16_t corner_x, int16_t corner_y, int16_t width,
                int16_t height, size_t num_values, float values[]) {
    const int16_t padding = 5;

    int16_t fx, fy;
    uint16_t fw, fh;
    display->setFont(&FreeSans9pt7b);
    display->getTextBounds(label, corner_x, corner_y, &fx, &fy, &fw, &fh);
    display->setCursor(corner_x + fw / 2, corner_y + height - padding);
    display->setTextColor(EPD_BLACK);
    float last_val = values[num_values - 1];
    if (last_val < 2) {
        display->printf("%.4f", last_val);
    } else if (last_val < 100) {
        display->printf("%.3f", last_val);
    } else if (last_val < 1000) {
        display->printf("%.f", last_val);
    } else {
        display->printf("%.1fK", last_val / 1000);
    }
    draw_sparkline(display, line_color, corner_x + padding, corner_y + padding,
                   width - padding * 2, height - fh - padding * 3, num_values,
                   values);
}

void show_status(Adafruit_GFX *display, const String &time,
                 float battery_voltage, int16_t x, int16_t y) {
    display->setFont(&Picopixel);
    display->setCursor(x, y);

    if (battery_voltage > 3.4f) {
        display->setTextColor(EPD_BLACK);
        display->printf("%.2f V   %s", battery_voltage, time.c_str());
    } else {
        display->setTextColor(EPD_RED);
        display->printf("LOW BATTERY  %.2f V   %s", battery_voltage,
                        time.c_str());
    }
}

void show_battery_icon(Adafruit_GFX *display, float battery_voltage) {
    const uint16_t BATTERY_WIDTH = 8;
    const uint16_t BATTERY_HEIGHT = 12;
    const uint16_t BATTERY_KNOB_WIDTH = 4;
    const uint16_t BATTERY_KNOB_HEIGHT = 2;
    const uint16_t EDGE_OFFSET = 1;

    const float FILL_MAX = BATTERY_HEIGHT;
    const float VOLTAGE_MIN = 3.3f;
    const float VOLTAGE_MAX = 3.8f;
    const uint16_t fill_size =
        min(FILL_MAX, (battery_voltage - VOLTAGE_MIN) /
                          (VOLTAGE_MAX - VOLTAGE_MIN) * FILL_MAX);

    display->fillRect(
        display->width() - BATTERY_HEIGHT - EDGE_OFFSET - BATTERY_KNOB_HEIGHT,
        display->height() - BATTERY_WIDTH - EDGE_OFFSET, fill_size,
        BATTERY_WIDTH, EPD_RED);
    display->drawRect(
        display->width() - BATTERY_HEIGHT - EDGE_OFFSET - BATTERY_KNOB_HEIGHT,
        display->height() - BATTERY_WIDTH - EDGE_OFFSET, BATTERY_HEIGHT,
        BATTERY_WIDTH, EPD_BLACK);
    display->drawRect(display->width() - BATTERY_KNOB_HEIGHT - EDGE_OFFSET,
                      display->height() - BATTERY_WIDTH / 2 -
                          BATTERY_KNOB_WIDTH / 2 - EDGE_OFFSET,
                      BATTERY_KNOB_HEIGHT, BATTERY_KNOB_WIDTH, EPD_BLACK);
}