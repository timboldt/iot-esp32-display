#pragma once

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <stdint.h>

void get_time(WiFiClientSecure* client, String* time);

size_t fetch_data(WiFiClientSecure* client, const String& feed_name,
                  size_t num_vals, float vals[], String* description);

bool send_data(WiFiClientSecure* client, const String& feed_name, float val);