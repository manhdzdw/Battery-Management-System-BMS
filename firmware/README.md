# Firmware

This folder contains the firmware source code for the ESP32-based Battery Management System.

## Firmware Structure

### 1. bms_main_6s2p
Main firmware for the BMS 6S2P system.

Main functions:
- Read individual cell voltages
- Measure pack current
- Monitor battery temperature
- Estimate SOC and remaining capacity
- Control charge/discharge MOSFETs
- Control passive cell balancing
- Display battery parameters on LCD I2C
- Send BMS data via CAN bus
- Support RS485/Modbus RTU communication

### 2. can_receiver
Firmware for the CAN receiver node.

Main functions:
- Receive CAN data sent from the BMS ESP32
- Decode battery parameters
- Display received data on Serial Monitor

## Platform
- ESP32
- Arduino IDE
- C/C++

## Communication Interfaces
- I2C
- UART / RS485
- SPI / MCP2515 CAN bus
