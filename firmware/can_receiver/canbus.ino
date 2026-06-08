/*
=========================================================
ESP32 CAN RECEIVER - CHỈ HIỂN THỊ TRÊN TERMINAL
=========================================================
*/

#include <SPI.h>
#include <mcp_can.h>

// =====================================================
// MCP2515
#define CAN_CS_PIN   5
MCP_CAN CAN0(CAN_CS_PIN);

// =====================================================
// Biến lưu dữ liệu nhận được
float packVoltage = 0;
float current = 0;
int soc = 0;
float temperature1 = 0;
float temperature2 = 0;
float capacity_mAh = 0;

float cellVoltage[6] = {0, 0, 0, 0, 0, 0};
int chargeState = 0;
int dischargeState = 0;

// =====================================================
unsigned long lastPrintTime = 0;

// =====================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n==========================================");
  Serial.println("ESP32 CAN RECEIVER - BMS DATA MONITOR");
  Serial.println("==========================================\n");
  
  // Khởi tạo CAN
  SPI.begin(13, 19, 23, 5);  // SCK=13, MISO=19, MOSI=23, SS=5
  
  if(CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("[OK] MCP2515 Initialized Successfully!");
    CAN0.setMode(MCP_NORMAL);
    Serial.println("[OK] CAN Mode: NORMAL");
    Serial.println("[OK] Waiting for CAN data...\n");
  } else {
    Serial.println("[ERROR] MCP2515 Initialization Failed!");
    Serial.println("[ERROR] Check wiring: CS=5, SCK=13, MISO=19, MOSI=23");
  }
  
  Serial.println("==========================================\n");
}

// =====================================================
void processCanData(unsigned long id, byte len, byte *buf) {
  if(id == 0x100) {  // Frame PACK BASIC DATA
    packVoltage = (buf[0] << 8 | buf[1]) / 100.0;
    current = (buf[2] << 8 | buf[3]) / 100.0;
    soc = buf[4];
    temperature1 = buf[5];
    temperature2 = buf[6];
    capacity_mAh = buf[7] * 10;
  }
  
  else if(id == 0x101) {  // Frame CELL VOLTAGES 1-4
    cellVoltage[0] = (buf[0] << 8 | buf[1]) / 1000.0;
    cellVoltage[1] = (buf[2] << 8 | buf[3]) / 1000.0;
    cellVoltage[2] = (buf[4] << 8 | buf[5]) / 1000.0;
    cellVoltage[3] = (buf[6] << 8 | buf[7]) / 1000.0;
  }
  
  else if(id == 0x102) {  // Frame CELL VOLTAGES 5-6 & STATUS
    cellVoltage[4] = (buf[0] << 8 | buf[1]) / 1000.0;
    cellVoltage[5] = (buf[2] << 8 | buf[3]) / 1000.0;
    chargeState = buf[4];
    dischargeState = buf[5];
  }
}

// =====================================================
void printData() {
  Serial.println("========== BMS DATA ==========");
  Serial.print("Pack Voltage: ");
  Serial.print(packVoltage);
  Serial.println(" V");
  
  Serial.print("Current: ");
  Serial.print(current);
  Serial.println(" A");
  
  Serial.print("SOC: ");
  Serial.print(soc);
  Serial.println(" %");
  
  Serial.print("Capacity Remaining: ");
  Serial.print(capacity_mAh);
  Serial.println(" mAh");
  
  Serial.print("Temperature 1: ");
  Serial.print(temperature1);
  Serial.println(" *C");
  
  Serial.print("Temperature 2: ");
  Serial.print(temperature2);
  Serial.println(" *C");
  
  Serial.println("\n-- Cell Voltages --");
  for(int i = 0; i < 6; i++) {
    Serial.print("Cell");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(cellVoltage[i], 3);
    Serial.println(" V");
  }
  
  Serial.println("\n-- FET Status --");
  Serial.print("Charge FET: ");
  Serial.println(chargeState ? "ON" : "OFF");
  Serial.print("Discharge FET: ");
  Serial.println(dischargeState ? "ON" : "OFF");
  
  Serial.println("=============================\n");
}

// =====================================================
void loop() {
  unsigned long id;
  byte len;
  byte buf[8];
  
  // Đọc dữ liệu CAN
  if(CAN0.readMsgBuf(&id, &len, buf) == CAN_OK) {
    processCanData(id, len, buf);
  }
  
  // In dữ liệu ra Terminal mỗi 1 giây
  if(millis() - lastPrintTime >= 1000) {
    lastPrintTime = millis();
    
    if(packVoltage > 0) {  // Chỉ in khi đã có dữ liệu
      printData();
    } else {
      Serial.print(".");
    }
  }
}