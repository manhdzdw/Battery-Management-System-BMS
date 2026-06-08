# BMS Main Firmware 6S2P

This firmware is used for the main ESP32 controller in the 6S2P Battery Management System.

## Main Functions
- Measure pack voltage and individual cell voltages
- Measure charge/discharge current using a low-side shunt resistor
- Monitor battery temperature using NTC sensors
- Estimate State of Charge (SOC)
- Control charge/discharge MOSFETs
- Activate passive cell balancing channels
- Display battery information on LCD I2C
- Send battery data through CAN bus
- Support RS485/Modbus RTU communication

## Hardware Used
- ESP32
- ADS1115 ADC module
- NTC temperature sensor
- Low-side shunt resistor
- MOSFET charge/discharge circuit
- Passive cell balancing circuit
- LCD I2C
- RS485 module
- MCP2515 CAN module

## Development Environment
- Arduino IDE
- C/C++
