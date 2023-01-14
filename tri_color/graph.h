#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <stdint.h>

void draw_graph(Adafruit_GFX *display, const String &label, int16_t corner_x, int16_t corner_y,
                int16_t width, int16_t height, size_t num_values,
                float values[]);
