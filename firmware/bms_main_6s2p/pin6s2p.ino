
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <mcp_can.h>
#include <math.h>

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2;
LiquidCrystal_I2C lcd(0x27,16,2);
HardwareSerial RS485(2);

// =====================================================
#define CAN_CS_PIN   5
MCP_CAN CAN0(CAN_CS_PIN);

// =====================================================
#define CHARGE_FET      18
#define DISCHARGE_FET   2
#define BUTTON_PIN      15

#define NTC1_PIN        34
#define NTC2_PIN        35

int balPin[6]={26,27,25,12,32,33};

// =====================================================
float CELL_MAX  = 4.2;
float CELL_MIN  = 2.6;
float PACK_MAX  = 25.2;
float CURR_MAX  = 2.60;
float TEMP_MAX  = 60;

float BAL_START = 3.80;
float BAL_DIFF  = 0.03;

// =====================================================
float cell[6];
float packV=0;
float currentA=0;
float temp1=0,temp2=0;
int soc=0;

float CAPACITY_mAh = 6000.0;
float remain_mAh = 6000.0;
unsigned long tAh=0;

// =====================================================
int chargeState = LOW;
int dischargeState = HIGH;

// Biến cờ bảo vệ
bool protectionActive = false;
unsigned long lastProtectionMsg = 0;

byte page=0;
unsigned long tRead=0;
unsigned long tLCD=0;
unsigned long tBtn=0;
unsigned long tAutoPage=0;
unsigned long tRS485=0;
unsigned long tCAN=0;

bool lastButtonState = HIGH;

// =====================================================
uint16_t regData[30];

// =====================================================
float readADS(Adafruit_ADS1115 &ads,int ch)
{
  long sum=0;
  for(int i=0;i<10;i++)
  {
    sum += ads.readADC_SingleEnded(ch);
    delay(2);
  }
  float adc=sum/10.0;
  return adc*0.1875/1000.0;
}

// =====================================================
void readCells()
{
  float v = readADS(ads2,2);
  float a0 = readADS(ads1,0)*6.3;
  float a1 = (readADS(ads1,1)-(1.4*v))*2.0;
  float a2 = (readADS(ads1,2)-(1.2*v))*3.0;
  float a3 = (readADS(ads1,3)-(1.2*v))*4.0;
  float a4 = (readADS(ads2,0)-(1.2*v))*5.3;
  float a5 = (readADS(ads2,1)-(1.15*v))*6.1;

  cell[0]=a0;
  cell[1]=(a1-a0);
  cell[2]=(a2-a1);
  cell[3]=(a3-a2);
  cell[4]=(a4-a3);
  cell[5]=(a5-a4);

  packV=a5;
}

// =====================================================
void readCurrent()
{
  float v = readADS(ads2,2);

  // tính dòng
  currentA = v / 0.1;
  currentA = fabs(currentA);
  if(currentA < 0.07)
    currentA = 0;
}
// =====================================================
int readADCavg(int pin)
{
  long sum=0;
  for(int i=0;i<20;i++)
  {
    sum += analogRead(pin);
    delay(2);
  }
  return sum/20;
}

float ntcToTemp(float Rntc)
{
  float T = 1.0 / ( log(Rntc/10000.0)/3950.0 + (1.0/298.15) );
  return T - 273.15;
}

float readNTC1()
{
  int adc = readADCavg(NTC1_PIN);
  float v = adc * 3.3 / 4095.0;
  if(v<0.03) v=0.03;
  if(v>3.27) v=3.27;
  float Rntc = (v * 10000.0) / (3.3 - v);
  return ntcToTemp(Rntc);
}

float readNTC2()
{
  int adc = readADCavg(NTC2_PIN);
  float v = adc * 3.3 / 4095.0;
  if(v<0.03) v=0.03;
  if(v>3.20) v=3.20;
  float Rntc = (v * 10000.0) / (5.0 - v);
  return ntcToTemp(Rntc);
}

void readTemp()
{
  static float old1=25;
  static float old2=25;

  temp1 = readNTC1();
  temp2 = readNTC2();

  temp1 = old1*0.7 + temp1*0.3;
  temp2 = old2*0.7 + temp2*0.3;

  old1=temp1;
  old2=temp2;
}

void capacityTask()
{
  float absI = fabs(currentA);

  // =================================================
  // CÓ DÒNG ĐIỆN
  // =================================================
  if(absI > 0.05)
  {
    unsigned long now = millis();

    if(tAh == 0)
    {
      tAh = now;
      return;
    }

    float dtHour = (now - tAh) / 3600000.0;
    tAh = now;

    float delta_mAh = absI * 1000.0 * dtHour;

    // XẢ
    if(dischargeState == HIGH && chargeState == LOW)
    {
      remain_mAh -= delta_mAh;
    }

    // SẠC
    else if(chargeState == HIGH && dischargeState == LOW)
    {
      remain_mAh += delta_mAh;
    }

    // giới hạn
    if(remain_mAh < 0) remain_mAh = 0;
    if(remain_mAh > CAPACITY_mAh) remain_mAh = CAPACITY_mAh;

    soc = (remain_mAh * 100.0) / CAPACITY_mAh;

    if(soc > 100) soc = 100;
    if(soc < 0) soc = 0;
  }

  // =================================================
  // KHÔNG CÓ DÒNG
  // =================================================
  else
  {
    tAh = 0;

    float vCell = packV / 6.0;
    soc = ((vCell - 2.60) / (3.90 - 2.60)) * 100.0;

    if(soc > 100) soc = 100;
    if(soc < 0) soc = 0;
    remain_mAh = (CAPACITY_mAh * soc) / 100.0;
  }
}
// =====================================================
void balanceTask()
{
  float minV=cell[0];
  for(int i=1;i<6;i++)
    if(cell[i]<minV) minV=cell[i];

  for(int i=0;i<6;i++)
  {
    if(cell[i]>BAL_START && (cell[i]-minV)>BAL_DIFF)
      digitalWrite(balPin[i],1);
    else
      digitalWrite(balPin[i],0);
  }
}

// =====================================================
void showModeMessage(String a,String b)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(a);
  lcd.setCursor(0,1);
  lcd.print(b);
  delay(1500);
}

// =====================================================
void handleButton()
{

  if(protectionActive) return;
  
  bool currentState=digitalRead(BUTTON_PIN);

  if(lastButtonState==HIGH && currentState==LOW)
  {
    if(millis()-tBtn>300)
    {
      tBtn=millis();

      if(dischargeState)
      {
        dischargeState=LOW;
        chargeState=HIGH;
        showModeMessage("CHARGE","CH=ON DS=OFF");
      }
      else if(chargeState)
      {
        chargeState=LOW;
        dischargeState=LOW;
        showModeMessage("OFF","CH=OFF DS=OFF");
      }
      else
      {
        chargeState=LOW;
        dischargeState=HIGH;
        showModeMessage("DISCHARGE","CH=OFF DS=ON");
      }

      digitalWrite(CHARGE_FET,chargeState);
      digitalWrite(DISCHARGE_FET,dischargeState);
    }
  }

  lastButtonState=currentState;
}

void protectVoltage()
{
  bool overVoltage = false;
  bool underVoltage = false;

  for(int i=0;i<6;i++)
  {
    if(cell[i] >= CELL_MAX)
      overVoltage = true;

    if(cell[i] <= CELL_MIN)
      underVoltage = true;
  }
  if(overVoltage && underVoltage)
  {
    chargeState = LOW;
    dischargeState = LOW;

    digitalWrite(CHARGE_FET, LOW);
    digitalWrite(DISCHARGE_FET, LOW);

    protectionActive = true;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("CELL FAULT !");
    lcd.setCursor(0,1);
    lcd.print("SYSTEM OFF");
  }

  else if(overVoltage || packV >= PACK_MAX)
  {
    dischargeState = HIGH;
    chargeState = LOW;

    digitalWrite(DISCHARGE_FET, HIGH);
    digitalWrite(CHARGE_FET, LOW);

    protectionActive = false;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("OVER VOLTAGE");
    lcd.setCursor(0,1);
    lcd.print("DISCHARGE ON");
  }

  else if(underVoltage)
  {
    chargeState = HIGH;
    dischargeState = LOW;

    digitalWrite(CHARGE_FET, HIGH);
    digitalWrite(DISCHARGE_FET, LOW);

    protectionActive = false;

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("UNDER VOLTAGE");
    lcd.setCursor(0,1);
    lcd.print("CHARGE ON");
  }

  else
  {
    protectionActive = false;
  }
}
void protectTemp()
{
  // QUÁ NHIỆT
  if(temp1 >= TEMP_MAX || temp2 >= TEMP_MAX)
  {
    chargeState = LOW;
    dischargeState = LOW;

    digitalWrite(CHARGE_FET, LOW);
    digitalWrite(DISCHARGE_FET, LOW);

    protectionActive = true;

    if(millis() - lastProtectionMsg > 2000)
    {
      lastProtectionMsg = millis();

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("OVER TEMP !");
      lcd.setCursor(0,1);
      lcd.print("SYSTEM OFF");
    }
  }

  // NHIỆT ĐỘ BÌNH THƯỜNG LẠI
  else
  {
    protectionActive = false;
  }
}

// =====================================================
void protectCurrent()
{
  // =========================
  // QUÁ DÒNG
  // =========================
  if(abs(currentA) >= CURR_MAX)
  {
    chargeState = LOW;
    dischargeState = LOW;

    digitalWrite(CHARGE_FET, LOW);
    digitalWrite(DISCHARGE_FET, LOW);

    protectionActive = true;

    if(millis() - lastProtectionMsg > 2000)
    {
      lastProtectionMsg = millis();

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("OVER CURRENT!");
      lcd.setCursor(0,1);
      lcd.print("SYSTEM OFF");
    }
  }
  // DÒNG BÌNH THƯỜNG
  else
  {
    protectionActive = false;
  }
}

// =====================================================
void lcdTask()
{
  if(millis()-tLCD<1200) return;
  tLCD=millis();
  
  // Nếu đang hiển thị thông báo bảo vệ thì không cập nhật page
  if(protectionActive && (millis() - lastProtectionMsg < 2000)) return;
  
  lcd.clear();

  if(page==0)
  {
    lcd.setCursor(0,0);
    lcd.print("Pack:");
    lcd.print(packV,2);

    lcd.setCursor(0,1);
    lcd.print("I:");
    lcd.print(currentA,2);
    lcd.print("A ");
    lcd.print(soc);
    lcd.print("%");
  }

  if(page==1)
  {
    lcd.setCursor(0,0);
    lcd.print("C1:");
    lcd.print(cell[0],2);
    lcd.print(" C2:");
    lcd.print(cell[1],2);

    lcd.setCursor(0,1);
    lcd.print("C3:");
    lcd.print(cell[2],2);
    lcd.print(" C4:");
    lcd.print(cell[3],2);
  }

  if(page==2)
  {
    lcd.setCursor(0,0);
    lcd.print("C5:");
    lcd.print(cell[4],2);
    lcd.print(" C6:");
    lcd.print(cell[5],2);
    
    lcd.setCursor(0,1);
    if(currentA > 0.05 || currentA < -0.05)
    {
      lcd.print("Cap:");
      lcd.print(remain_mAh,0);
      lcd.print("/");
      lcd.print(CAPACITY_mAh,0);
      lcd.print("mAh");
    }
    else
    {
      lcd.print("SOC:");
      lcd.print(soc);
      lcd.print("%");
    }
  }

  if(page==3)
  {
    lcd.setCursor(0,0);
    lcd.print("T1:");
    lcd.print(temp1,1);

    lcd.setCursor(8,0);
    lcd.print("T2:");
    lcd.print(temp2,1);

    lcd.setCursor(0,1);

    if(chargeState) lcd.print("MODE:CHARGE");
    else if(dischargeState) lcd.print("MODE:DISCH");
    else lcd.print("MODE:OFF");
  }
}

// =====================================================
void autoPageTask()
{
  if(millis()-tAutoPage>3000)
  {
    tAutoPage=millis();
    page++;
    if(page>3) page=0;
  }
}

// =====================================================
void rs485TextSend()
{
  if(millis()-tRS485<1000) return;
  tRS485=millis();

  RS485.print("PACK=");
  RS485.print(packV,2);
  RS485.print(",I=");
  RS485.print(currentA,2);
  RS485.print(",SOC=");
  RS485.print(soc);
  RS485.print(",CAP=");
  RS485.print(remain_mAh,0);
  RS485.print("mAh");
  RS485.print(",C1=");
  RS485.print(cell[0],3);
  RS485.print(",C2=");
  RS485.print(cell[1],3);
  RS485.print(",C3=");
  RS485.print(cell[2],3);
  RS485.print(",C4=");
  RS485.print(cell[3],3);
  RS485.print(",C5=");
  RS485.print(cell[4],3);
  RS485.print(",C6=");
  RS485.print(cell[5],3);
  RS485.print(",T1=");
  RS485.print(temp1,1);
  RS485.print(",T2=");
  RS485.print(temp2,1);
  RS485.println();
}

// =====================================================
uint16_t crc16(uint8_t *buf,int len)
{
  uint16_t crc=0xFFFF;
  for(int pos=0;pos<len;pos++)
  {
    crc ^= buf[pos];
    for(int i=8;i!=0;i--)
    {
      if(crc & 0x0001)
      {
        crc >>=1;
        crc ^=0xA001;
      }
      else crc >>=1;
    }
  }
  return crc;
}

// =====================================================
void modbusUpdate()
{
  regData[0]=packV*100;
  regData[1]=currentA*100;
  regData[2]=soc;
  regData[3]=temp1*10;
  regData[4]=temp2*10;
  regData[5]=remain_mAh;
  regData[6]=cell[0]*1000;
  regData[7]=cell[1]*1000;
  regData[8]=cell[2]*1000;
  regData[9]=cell[3]*1000;
  regData[10]=cell[4]*1000;
  regData[11]=cell[5]*1000;
}

// =====================================================
void modbusTask()
{
  if(RS485.available()<8) return;

  uint8_t rx[8];
  for(int i=0;i<8;i++) rx[i]=RS485.read();

  if(rx[0]!=1) return;
  if(rx[1]!=3) return;

  uint16_t start=(rx[2]<<8)|rx[3];
  uint16_t qty=(rx[4]<<8)|rx[5];

  uint8_t tx[64];
  tx[0]=1;
  tx[1]=3;
  tx[2]=qty*2;

  int idx=3;

  for(int i=0;i<qty;i++)
  {
    uint16_t val=regData[start+i];
    tx[idx++]=highByte(val);
    tx[idx++]=lowByte(val);
  }

  uint16_t crc=crc16(tx,idx);
  tx[idx++]=lowByte(crc);
  tx[idx++]=highByte(crc);

  RS485.write(tx,idx);
}

// =====================================================
void canSend()
{
  if (millis() - tCAN < 200) return;
  tCAN = millis();

  byte txBuf[8];
  
  uint16_t pv = packV * 100;
  int16_t ca = currentA * 100;
  
  txBuf[0] = highByte(pv);
  txBuf[1] = lowByte(pv);
  txBuf[2] = highByte(ca);
  txBuf[3] = lowByte(ca);
  txBuf[4] = soc;
  txBuf[5] = (byte)temp1;
  txBuf[6] = (byte)temp2;
  txBuf[7] = (byte)(remain_mAh / 10);
  
  CAN0.sendMsgBuf(0x100, 0, 8, txBuf);
  delay(10);
  
  txBuf[0] = highByte((uint16_t)(cell[0] * 1000));
  txBuf[1] = lowByte((uint16_t)(cell[0] * 1000));
  txBuf[2] = highByte((uint16_t)(cell[1] * 1000));
  txBuf[3] = lowByte((uint16_t)(cell[1] * 1000));
  txBuf[4] = highByte((uint16_t)(cell[2] * 1000));
  txBuf[5] = lowByte((uint16_t)(cell[2] * 1000));
  txBuf[6] = highByte((uint16_t)(cell[3] * 1000));
  txBuf[7] = lowByte((uint16_t)(cell[3] * 1000));
  
  CAN0.sendMsgBuf(0x101, 0, 8, txBuf);
  delay(10);
  
  txBuf[0] = highByte((uint16_t)(cell[4] * 1000));
  txBuf[1] = lowByte((uint16_t)(cell[4] * 1000));
  txBuf[2] = highByte((uint16_t)(cell[5] * 1000));
  txBuf[3] = lowByte((uint16_t)(cell[5] * 1000));
  txBuf[4] = chargeState;
  txBuf[5] = dischargeState;
  txBuf[6] = 0;
  txBuf[7] = 0;
  
  CAN0.sendMsgBuf(0x102, 0, 8, txBuf);
}

// =====================================================
void setup()
{
  Serial.begin(115200);

  Wire.begin(21,22);

  lcd.init();
  lcd.backlight();

  ads1.begin(0x48);
  ads2.begin(0x49);

  ads1.setGain(GAIN_TWOTHIRDS);
  ads2.setGain(GAIN_TWOTHIRDS);

  pinMode(CHARGE_FET,OUTPUT);
  pinMode(DISCHARGE_FET,OUTPUT);
  pinMode(BUTTON_PIN,INPUT_PULLUP);

  for(int i=0;i<6;i++)
  {
    pinMode(balPin[i],OUTPUT);
    digitalWrite(balPin[i],0);
  }

  analogReadResolution(12);

  RS485.begin(9600,SERIAL_8N1,17,16);

  SPI.begin(13,19,23,5);
  
  if(CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK) {
    CAN0.setMode(MCP_NORMAL);
  }

  digitalWrite(CHARGE_FET,chargeState);
  digitalWrite(DISCHARGE_FET,dischargeState);

  lcd.clear();
  lcd.print("BMS READY");
  delay(1500);
}

// =====================================================
void loop()
{
  canSend();

  if(millis()-tRead>500)
  {
    tRead=millis();

    readCells();
    readCurrent();
    readTemp();

    protectVoltage();
    protectTemp();
    protectCurrent();
    
    capacityTask();
    balanceTask();
    modbusUpdate();
  }

  handleButton();
  autoPageTask();
  lcdTask();

  rs485TextSend();
  modbusTask();
}