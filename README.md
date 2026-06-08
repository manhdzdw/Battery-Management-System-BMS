# Battery Management System for BTS Backup Battery Pack

## Overview
This project focuses on designing and implementing a Battery Management System (BMS) for a 6S2P Lithium-ion backup battery pack used in BTS power systems.

The system is designed to monitor battery parameters, protect the battery pack from abnormal conditions, support passive cell balancing, and transmit data through industrial communication interfaces.

## Main Features
- Monitor pack voltage and individual cell voltages
- Measure charge/discharge current
- Monitor battery temperature
- Estimate State of Charge (SOC)
- Implement overvoltage, undervoltage, and overtemperature protection
- Support passive cell balancing
- Display battery parameters on LCD I2C
- Transmit data via RS485/Modbus RTU and CAN bus

## Hardware
- ESP32 microcontroller
- ADS1115 ADC module
- Low-side shunt resistor
- NTC temperature sensor
- MOSFET charge/discharge control circuit
- Passive cell balancing circuit
- RS485 module
- MCP2515 CAN module
- LCD I2C

## Software and Tools
- C/C++
- Arduino IDE
- Altium Designer
- Hercules Terminal
- Modbus Poll

## Communication
- RS485 / Modbus RTU
- CAN bus using MCP2515

## Project Status
Prototype completed and tested with a 6S2P Lithium-ion battery pack.
