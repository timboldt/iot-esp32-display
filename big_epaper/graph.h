#pragma once

#include <Arduino.h>
#include <stdint.h>

void draw_graph(const String &label, int16_t corner_x, int16_t corner_y,
                int16_t width, int16_t height, size_t num_values,
                float values[]);
