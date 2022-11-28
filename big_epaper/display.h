#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <GxEPD2_BW.h>

#define MAX_DISPLAY_BUFFER_SIZE 15000ul  // ~15k is a good compromise
#define MAX_HEIGHT(EPD)                                        \
    (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) \
         ? EPD::HEIGHT                                         \
         : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

extern GxEPD2_BW<GxEPD2_420, MAX_HEIGHT(GxEPD2_420)> display;
