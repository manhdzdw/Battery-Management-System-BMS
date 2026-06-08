# CAN Receiver Firmware

This firmware is used for the second ESP32 node to receive CAN bus data from the main BMS controller.

## Main Functions
- Receive CAN frames from the BMS ESP32
- Decode battery data such as pack voltage, current, SOC, cell voltages, and temperature
- Display received data on Serial Monitor

## Hardware Used
- ESP32
- MCP2515 CAN module

## Communication
- SPI between ESP32 and MCP2515
- CAN bus between two ESP32 nodes

## Development Environment
- Arduino IDE
- C/C++
