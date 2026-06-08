# Firmware

This folder contains the firmware source code for the ESP32-based Battery Management System.

## Main Functions
- Read cell voltage data from ADS1115
- Measure pack current using a low-side shunt resistor
- Monitor battery temperature using NTC sensors
- Estimate SOC and remaining capacity
- Control charge/discharge MOSFETs
- Control passive cell balancing channels
- Display data on LCD I2C
- Transmit data via RS485/Modbus RTU
- Transmit data via CAN bus using MCP2515

## Platform
- ESP32
- Arduino IDE
- C/C++
