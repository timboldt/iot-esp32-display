arduino-cli compile --fqbn esp32:esp32:adafruit_feather_esp32_v2
arduino-cli upload --fqbn esp32:esp32:adafruit_feather_esp32_v2 --port /dev/ttyACM0

# For some reason, `arduino-cli upload` is sending the wrong parameters, but this works (yikes!):
# esptool.py write_flash 0x1000 "/private/var/folders/yz/wkvw343x7tgd8m97tf8lq7zc0000gn/T/arduino-sketch-965DC5B1FAF9DA47009123A220E49C78/big_epaper.ino.bootloader.bin" 0x8000 "/private/var/folders/yz/wkvw343x7tgd8m97tf8lq7zc0000gn/T/arduino-sketch-965DC5B1FAF9DA47009123A220E49C78/big_epaper.ino.partitions.bin" 0xe000 "/Users/timboldt/Library/Arduino15/packages/esp32/hardware/esp32/2.0.5/tools/partitions/boot_app0.bin" 0x10000 "/private/var/folders/yz/wkvw343x7tgd8m97tf8lq7zc0000gn/T/arduino-sketch-965DC5B1FAF9DA47009123A220E49C78/big_epaper.ino.bin"

