#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <stdint.h>
#include <vector>

class Config {
   public:
    int16_t days;
    int16_t rows;
    int16_t cols;
    std::vector<String> feeds;
};

void get_time(WiFiClientSecure* client, String* time);

size_t fetch_data(WiFiClientSecure* client, const String& symbol,
                  size_t hours, size_t num_vals, float vals[],
                  String* description);
