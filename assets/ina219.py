import time
import board
from adafruit_ina219 import ADCResolution, BusVoltageRange, INA219

i2c_bus = board.I2C()
ina219 = INA219(i2c_bus)

print("ina219 test")
print("Config register:")
print("  bus_voltage_range:    0x%1X" % ina219.bus_voltage_range)
print("  gain:                 0x%1X" % ina219.gain)
print("  bus_adc_resolution:   0x%1X" % ina219.bus_adc_resolution)
print("  shunt_adc_resolution: 0x%1X" % ina219.shunt_adc_resolution)
print("  mode:                 0x%1X" % ina219.mode)
print("")

ina219.bus_adc_resolution = ADCResolution.ADCRES_12BIT_1S
ina219.shunt_adc_resolution = ADCResolution.ADCRES_12BIT_1S
ina219.bus_voltage_range = BusVoltageRange.RANGE_16V
ina219.set_calibration_16V_400mA()

# measure and display loop
while True:
    # bus_voltage = ina219.bus_voltage  # voltage on V- (load side)
    # shunt_voltage = ina219.shunt_voltage  # voltage between V+ and V- across the shunt
    current = ina219.current  # current in mA
    # power = ina219.power  # power in watts

    print(time.time(), current)

    # Check internal calculations haven't overflowed (doesn't detect ADC overflows)
    if ina219.overflow:
        print("Internal Math Overflow Detected!")
        print("")

    time.sleep(0.1)
