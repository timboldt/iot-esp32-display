#include "display.h"

#define EPD_BUSY 14
#define EPD_RESET 32
#define EPD_DC 33
#define EPD_CS 15

// Waveshare 4.2" B/W EPD.
GxEPD2_BW<GxEPD2_420, MAX_HEIGHT(GxEPD2_420)> display(GxEPD2_420(EPD_CS, EPD_DC,
                                                                 EPD_RESET,
                                                                 EPD_BUSY));
