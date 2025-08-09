#define BLYNK_TEMPLATE_ID "TMPL6iuwSgGub"
#define BLYNK_TEMPLATE_NAME "congtoso"
#define BLYNK_AUTH_TOKEN "Hs4VnFp8TzenqmZ7eoI1ZE0HUFAN9xMl"

#include <PZEM004Tv30.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include "SoftI2C_LCD.h"
#include "Sensor.h"
#include "KeyPad.h"
#include "Module_SIM.h"
#include "GetTime.h"

PZEM004Tv30 pzem(Serial2, 16, 17);
Preferences preferences;

#define Vol V0
#define Amp V1
#define Power V2
#define Energy V3
#define Money V4
#define CostPerWh V6
#define Temp      V7
#define OVERVOLTAGE_THRESHOLD 250.0   // Ví dụ: > 250V là quá áp
#define OVERCURRENT_THRESHOLD 40.0     // Ví dụ: > 5A là quá dòng

char auth[] = BLYNK_AUTH_TOKEN;
String CostPerHour;
double Cost = 2500.0;
double savedEnergy;

BlynkTimer timer;
WiFiManager wm;

unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 60000; // 1 phút
String PhoneNumber;
bool overVoltageSent = false;
bool overCurrentSent = false;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(CostPerWh);
}

BLYNK_WRITE(CostPerWh) {
  CostPerHour = param.asStr();
}

void ConvertStringToDouble() {
  if (CostPerHour.length() > 0) {
    Cost = CostPerHour.toDouble();
  } else {
    Cost = 2500.0;
  }
}

void Display() {
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy() + savedEnergy;

  float temp    = read_temperature();  
  double TienDien = energy * Cost;   // đồng
  double TienNghin = TienDien / 1000.0;

  preferences.putFloat("energy_data", (float)energy);

  char buffer[21];  // LCD 20x4: 20 ký tự mỗi dòng + 1 null terminator

  // Dòng 1: V & I
  snprintf(buffer, sizeof(buffer), "V:%.1f I:%.1fA", voltage, current);
  lcd_goto_XY(0, 0);
  lcd_send_string(buffer);

  // Dòng 2: P & E
  snprintf(buffer, sizeof(buffer), "P:%.0fW E:%.2fkWh", power, energy);
  lcd_goto_XY(1, 0);
  lcd_send_string(buffer);

  // Dòng 3: Nhiệt độ
  snprintf(buffer, sizeof(buffer), "Nhiet do:%.1fC", temp);
  lcd_goto_XY(2, 0);
  lcd_send_string(buffer);

  // Dòng 4: Tiền điện
  snprintf(buffer, sizeof(buffer), "Tien:%.1f N.dong", TienNghin);
  lcd_goto_XY(3, 0);
  lcd_send_string(buffer);

  // Serial debug
  Serial.print("V:"); Serial.print(voltage, 1);
  Serial.print(" I:"); Serial.print(current, 1); Serial.println("A");
  Serial.print("P:"); Serial.print(power, 0);
  Serial.print("W E:"); Serial.print(energy, 2); Serial.println("kWh");
  Serial.print("T:"); Serial.print(temp, 1); Serial.println("C");
  Serial.print("Tien:"); Serial.print(TienNghin, 2); Serial.println(" N.dong  ");

  // Blynk update
  if (Blynk.connected()) 
  {
    Blynk.virtualWrite(Vol, voltage);
    Blynk.virtualWrite(Amp, current);
    Blynk.virtualWrite(Power, power);
    Blynk.virtualWrite(Energy, energy);
    Blynk.virtualWrite(Money, TienDien);
    Blynk.virtualWrite(Temp, temp);
  }
}
int caseCount = 0; // đếm số lần đã gửi cảnh báo, reset về 0 khi ESP reset

void FireAlert()
{

  float temperature = read_temperature();
  int raw = digitalRead(25);
  Serial.print("raw: "); Serial.println(raw);

  if (temperature > 35 && temperature < 40 && PhoneNumber.length() > 0 && caseCount < 5)
  {
    case1(PhoneNumber);
    caseCount++;
    delay(1000); // tránh gửi nhiều lần liên tiếp
  }
  else if (temperature > 40 && raw == 1 && PhoneNumber.length() > 0  && caseCount < 5)
  {
    case2(PhoneNumber);
    caseCount++;
    delay(1000);
  }
}


void CheckOverload() {
  float voltage = pzem.voltage();
  float current = pzem.current();

  // Kiểm tra quá áp
  if (voltage > OVERVOLTAGE_THRESHOLD && !overVoltageSent) {
    String msg = "CANH BAO: Dien ap vuot nguong (" + String(voltage, 1) + "V)";
    sendSMS(PhoneNumber, msg);
    overVoltageSent = true;
    Serial.println(">>> Da gui canh bao qua ap.");
  } else if (voltage <= OVERVOLTAGE_THRESHOLD) {
    overVoltageSent = false;  // Reset cờ khi đã trở lại bình thường
  }

  // Kiểm tra quá dòng
  if (current > OVERCURRENT_THRESHOLD && !overCurrentSent) {
    String msg = "CANH BAO: Dong dien vuot nguong (" + String(current, 1) + "A)";
    sendSMS(PhoneNumber, msg);
    overCurrentSent = true;
    Serial.println(">>> Da gui canh bao qua dong.");
  } else if (current <= OVERCURRENT_THRESHOLD) {
    overCurrentSent = false;  // Reset cờ khi đã trở lại bình thường
  }
}

void startConfigPortal() {
  Serial.println("Mất WiFi! Bắt đầu config portal...");
  wm.startConfigPortal("ESP32_Config");
}

void setup() {
  init_sensors();
  pinMode(25, INPUT);
  Serial.begin(115200);
  delay(1000);

  sim_init();
  KeyPad_Init();
  lcd_init();
  lcd_send_string("Khoi dong...");

  preferences.begin("energy_data", false);
  savedEnergy = preferences.getFloat("energy_data", 0.0);

  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(180);
  bool res;
  res = wm.autoConnect("EnergyMeter");
  if(!res) {
    Serial.println("Không kết nối được WiFi, mở config portal...");
    // Nếu autoConnect thất bại, mở config portal
    wm.startConfigPortal("EnergyMeter");
  } else {
    Serial.println("Kết nối WiFi thành công!");
  }

  timer.setInterval(1000L, Display);
  timer.setInterval(1000L, FireAlert);
  timer.setInterval(2000L, CheckOverload);
}

void loop() {
  wm.process();

  if (WiFi.status() == WL_CONNECTED) {
    if (!Blynk.connected()) {
      Serial.println("Đang kết nối Blynk...");
      Blynk.config(auth);
      Blynk.connect();
    }
  }

  if (Blynk.connected()) {
    Blynk.run();
  }

  timer.run();
  ConvertStringToDouble();
  // Tự động kiểm tra WiFi mỗi 1 phút
  if (millis() - lastCheckTime > checkInterval) {
    lastCheckTime = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Mất WiFi, khởi tạo lại config portal...");
      wm.setConfigPortalBlocking(false);
      wm.startConfigPortal("EnergyMeter");
    }
  }
  String TempPhone;
  TempPhone = CheckAndStartPhoneInput_LCD();
  if(TempPhone.length() > 0)
  { 
    PhoneNumber = TempPhone;
      Serial.print("Phone Number: "); Serial.println(PhoneNumber);
      Serial.println("Gui tin nhan");
      sendSMS(PhoneNumber, "Sim da ket noi");
  }

  static unsigned long lastDayCheck = 0;
  if (millis() - lastDayCheck > 3600000) 
  {  // mỗi 1 giờ
    lastDayCheck = millis();

    // Lấy ngày hiện tại từ SIM
    String dateStr = getDateFromNTP(); // "24/06/27,16:05:34+00"
    int day = extractDay(dateStr);     // --> 27

    bool alreadySent = preferences.getBool("sent_today", false);

    if (day == 25 && !alreadySent) 
    {
      float energy  = pzem.energy() + savedEnergy;
      double money = energy * Cost;
      String msg = "Tien dien thang nay: " + String(money, 0) + " dong";
      sendSMS(PhoneNumber, msg);

      delay(3000); // chờ tin nhắn gửi xong

      // Reset điện năng
      pzem.resetEnergy();
      preferences.putFloat("energy_data", 0.0);
      savedEnergy = 0;

      preferences.putBool("sent_today", true);
      Serial.println(">> Da gui tin nhan va reset dien.");
    }

    // Nếu qua ngày khác thì reset cờ để tháng sau gửi lại
    if (day != 25) 
    {
      preferences.putBool("sent_today", false);
    }
  }
}
