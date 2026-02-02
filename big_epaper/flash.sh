#!/bin/bash

# Determine the port based on the OS
if [[ "$OSTYPE" == "darwin"* ]]; then
    PORT="/dev/tty.wchusbserial54B00108461"
else
    PORT="/dev/ttyACM0"
fi

arduino-cli compile -v --fqbn esp32:esp32:adafruit_feather_esp32_v2 && arduino-cli upload --fqbn esp32:esp32:adafruit_feather_esp32_v2 --port "$PORT"
