#define BLYNK_TEMPLATE_ID "TMPL6PPun2nSl"
#define BLYNK_TEMPLATE_NAME "VuonTm"
#define BLYNK_AUTH_TOKEN "0Deuz7VtsZRel6Qss7R12LxNAJv3kDoy"

#include <Wire.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define DHTTYPE DHT11
#define DHT_PIN 13
#define ENA 26
#define IN1 16
#define IN2 4
#define IN3 2
#define IN4 15
#define ENB 27
#define FAN 25
#define LIGHT 14
#define DAD 39
#define WF 12
#define DS1307 0x68

const int bt[4] = {17, 18, 5, 19};

LiquidCrystal_I2C lcd(0x27, 20, 4);
DHT dht(DHT_PIN, DHTTYPE);
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t ReadSensorDataTask_Handle;
TaskHandle_t DataDisplayTask_Handle;
TaskHandle_t ActuatorControlTask_Handle;
TaskHandle_t ConfigTask_Handle;

QueueHandle_t buttonQueue;
QueueHandle_t sensorDataQueue;
QueueHandle_t timeQueue;

volatile uint32_t pulseCount = 0;
volatile bool interruptDisable = false;
bool autoMode = 0;
volatile TickType_t lastButtonPressTick[4] = {0, 0, 0, 0};

typedef struct {
  float temperature;
  float humidity;
  int soilMoisture;
  float waterFlow;
  int light;
} SensorData_t;

typedef struct {
  int second;
  int minute;
  int hour;
  int day;
  int wday;
  int month;
  int year;
} Time_t;

int bcd2dec(byte num) {
  return ((num / 16 * 10) + (num % 16));
}

void IRAM_ATTR handleButton0() {
  TickType_t now = xTaskGetTickCountFromISR();
  if (!interruptDisable && (now - lastButtonPressTick[0] > pdMS_TO_TICKS(200))) {
    lastButtonPressTick[0] = now;
    uint8_t btn = 0;
    xQueueSendFromISR(buttonQueue, &btn, NULL);
  }
}

void IRAM_ATTR handleButton1() {
  TickType_t now = xTaskGetTickCountFromISR();
  if (!interruptDisable && (now - lastButtonPressTick[1] > pdMS_TO_TICKS(200))) {
    lastButtonPressTick[1] = now;
    uint8_t btn = 1;
    xQueueSendFromISR(buttonQueue, &btn, NULL);
  }
}

void IRAM_ATTR handleButton2() {
  TickType_t now = xTaskGetTickCountFromISR();
  if (!interruptDisable && (now - lastButtonPressTick[2] > pdMS_TO_TICKS(200))) {
    lastButtonPressTick[2] = now;
    uint8_t btn = 2;
    xQueueSendFromISR(buttonQueue, &btn, NULL);
  }
}

void IRAM_ATTR handleButton3() {
  TickType_t now = xTaskGetTickCountFromISR();
  if (!interruptDisable && (now - lastButtonPressTick[3] > pdMS_TO_TICKS(200))) {
    lastButtonPressTick[3] = now;
    uint8_t btn = 3;
    xQueueSendFromISR(buttonQueue, &btn, NULL);
  }
}

void IRAM_ATTR flowSensorISR() {
  portENTER_CRITICAL_ISR(&mux);
  pulseCount++;
  portEXIT_CRITICAL_ISR(&mux);
}

#define Tempblk  V0
#define Airblk   V1
#define Soilblk V2
#define WFblk  V3
#define Modeblk V4
#define Fanblk V5
#define Bom1blk V6
#define Bom2blk V7
#define Denblk  V8


char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Tang8";
char pass[] = "123456789@";
BlynkTimer timer;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(Tempblk);
  Blynk.syncVirtual(Airblk);
  Blynk.syncVirtual(Soilblk);
  Blynk.syncVirtual(WFblk);
  Blynk.syncVirtual(Modeblk);
}
uint8_t bom1, bom2, mode, fan , den;
BLYNK_WRITE(Bom1blk)
{
  bom1=param.asInt();
}
BLYNK_WRITE(Bom2blk)
{
  bom2=param.asInt();
}
BLYNK_WRITE(Modeblk)
{
  autoMode=param.asInt();
}
BLYNK_WRITE(Denblk)
{
  den=param.asInt();
}
BLYNK_WRITE(Fanblk)
{
  fan=param.asInt();
}

int configValues[6] = {25, 60, 30, 14, 1, 24}; // Giá trị mặc định cho các tham số cấu hình
void ConfigTask(void *pvParameters) {
  Blynk.begin(auth,ssid,pass);
  const char* labels[] = {
    "Nhiet do: ", "Do am kk: ", "Do am dat: ",
    "May bom:",
    "Den tu:", "Den den:"
  };
  int configDone = 0;
  int currentConfig = 0;
  unsigned long lastAdjustTime = 0;
  bool increaseHeld = false, decreaseHeld = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("---- CAU HINH ----");
  vTaskDelay(pdMS_TO_TICKS(1000));

  while (!configDone) {
    lcd.setCursor(0, 1);
    lcd.print("Cau hinh: ");
    lcd.print(labels[currentConfig]);

    lcd.setCursor(0, 2);
    lcd.print("Gia tri: ");
    lcd.print(configValues[currentConfig]);
    lcd.print("      ");

    uint8_t btn;
    if (xQueueReceive(buttonQueue, &btn, pdMS_TO_TICKS(200)) == pdTRUE) {
      switch (btn) {
        case 0:
          currentConfig = (currentConfig + 1) % 6;
          Serial.print(labels[currentConfig]);
          delay(1000);
          break;
        case 1:
          configValues[currentConfig]++;
          increaseHeld = true;
          lastAdjustTime = millis();
          Serial.print(labels[currentConfig]); Serial.println(configValues[currentConfig]);
          break;
        case 2:
          configValues[currentConfig]--;
          Serial.print(labels[currentConfig]); Serial.println(configValues[currentConfig]);
          decreaseHeld = true;
          lastAdjustTime = millis();
          break;
        case 3:
          configDone = true;
          break;
      }
    }

    // Tăng giảm nhanh khi giữ nút
    if (increaseHeld && millis() - lastAdjustTime > 300) {
      configValues[currentConfig]++;
      lastAdjustTime = millis();
    } else if (decreaseHeld && millis() - lastAdjustTime > 300) {
      configValues[currentConfig]--;
      lastAdjustTime = millis();
    }

    // Reset trạng thái giữ nếu không còn giữ nữa
    if (digitalRead(bt[1]) == HIGH) increaseHeld = false;
    if (digitalRead(bt[2]) == HIGH) decreaseHeld = false;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cau hinh xong!");
  

  xTaskCreatePinnedToCore(ReadSensorTask, "ReadSensor", 8192, NULL, 4, &ReadSensorDataTask_Handle, 1);
  xTaskCreatePinnedToCore(DataDisplayTask, "Display", 4096, NULL, 3, &DataDisplayTask_Handle, 1);
  xTaskCreatePinnedToCore(ActuatorControlTask, "Actuator", 4096, NULL, 2, &ActuatorControlTask_Handle, 1);
  vTaskDelete(NULL); // Xoá Task sau khi hoàn tất
}

void ReadSensorTask(void *pvParameters) {
  SensorData_t data;
  Time_t date;
  uint32_t localPulse = 0;

  while (1) {
    portENTER_CRITICAL(&mux);
    localPulse = pulseCount;
    pulseCount = 0;
    portEXIT_CRITICAL(&mux);

    Wire.beginTransmission(DS1307);
    Wire.write((byte)0x00);
    Wire.endTransmission();
    Wire.requestFrom(DS1307, 7);
    date.second = bcd2dec(Wire.read() & 0x7f);
    date.minute = bcd2dec(Wire.read());
    date.hour = bcd2dec(Wire.read() & 0x3f);
    date.wday = bcd2dec(Wire.read());
    date.day = bcd2dec(Wire.read());
    date.month = bcd2dec(Wire.read());
    date.year = bcd2dec(Wire.read()) + 2000;

    data.temperature = dht.readTemperature();
    data.humidity = dht.readHumidity();
    data.soilMoisture = 100 - analogRead(DAD) * 100 / 4096;
    data.waterFlow = localPulse * 2.25;
    data.light = digitalRead(36);

    xQueueSend(sensorDataQueue, &data, 0);
    xQueueSend(timeQueue, &date, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void DataDisplayTask(void *pvParameters) {
  SensorData_t sensor;
  Time_t date;

  while (1) {
      Blynk.run();
  timer.run();
    if (xQueueReceive(sensorDataQueue, &sensor, 0) == pdTRUE &&
        xQueueReceive(timeQueue, &date, 0) == pdTRUE) {

      if (autoMode) 
      { 
        Serial.print("mode: ");Serial.println(autoMode);
        Blynk.virtualWrite(Modeblk, autoMode);
        if(sensor.humidity < configValues[1] && sensor.temperature > configValues[0])
        {
          digitalWrite(FAN, 1);
          Blynk.virtualWrite(Fanblk, 1);
        }
        else
        {
          digitalWrite(FAN,0);
          Blynk.virtualWrite(Fanblk, 0);
        }
        if(sensor.light == 1 && configValues[4] <= date.hour && date.hour <= configValues[5])
        {
          digitalWrite(LIGHT, 1);
          Blynk.virtualWrite(Denblk, 1);
        }
        else
        {
          digitalWrite(LIGHT, 0);
           Blynk.virtualWrite(Denblk, 0);
        }
        if (sensor.soilMoisture < configValues[2]) {
          digitalWrite(IN1, 1);
          digitalWrite(IN2, 0);
          digitalWrite(ENA, 1);
           Blynk.virtualWrite(Bom1blk, 1);
        } else {
          digitalWrite(ENA, 0);
           Blynk.virtualWrite(Bom1blk, 0);
        }
        if(date.day == configValues[3]  && 5 <= date.hour && date.hour <= 7)
        {
          digitalWrite(IN3, 1);
          digitalWrite(IN4, 0);
          digitalWrite(ENB, 1);
           Blynk.virtualWrite(Bom2blk, 1);
        }
        else
        {
          digitalWrite(ENB, 0);
           Blynk.virtualWrite(Bom2blk, 0);
        }
      }
      else
      {
        Serial.print("Quat: ");Serial.print(fan) ;
        Serial.print("Den: ");Serial.print(den);
        Serial.print("Bom1: ");Serial.print(bom1);
        Serial.print("Bom2: ");Serial.println(bom2);
        Blynk.virtualWrite(Bom1blk, bom1);
        Blynk.virtualWrite(Fanblk, fan);
        Blynk.virtualWrite(Denblk, den);
        Blynk.virtualWrite(Bom2blk, bom2);
        digitalWrite(FAN, fan);
        digitalWrite(LIGHT,den);
        if(bom1 == 1)
        {
          digitalWrite(IN1, 1);
          digitalWrite(IN2, 0);
          digitalWrite(ENA, 1);
        }
        else
        {
          digitalWrite(ENA,0);
        }
        if(bom2 == 1)
        {
          digitalWrite(IN3, 1);
          digitalWrite(IN4, 0);
          digitalWrite(ENB, 1);
        }
        else
        {
          digitalWrite(ENB, 0);
        }
      }
      Blynk.virtualWrite(Tempblk, sensor.temperature);
      Blynk.virtualWrite(Airblk, sensor.humidity);
      Blynk.virtualWrite(Soilblk, sensor.soilMoisture);
      Blynk.virtualWrite(WFblk, sensor.waterFlow);

      lcd.setCursor(0, 0);
      lcd.print("Nhiet do: "); lcd.print(sensor.temperature);
      lcd.setCursor(0, 1);
      lcd.print("Do am kk: "); lcd.print(sensor.humidity);
      lcd.setCursor(0, 2);
      lcd.print("DAD: "); lcd.print(sensor.soilMoisture);
      lcd.setCursor(0, 3);
      lcd.printf("%02d:%02d:%02d %02d/%02d/%d", date.hour, date.minute, date.second, date.day, date.month, date.year);
      Serial.print("LCD Display -> Temp: ");
      Serial.print(sensor.temperature);
      Serial.print(" C, Humidity: ");
      Serial.print(sensor.humidity);
      Serial.print(" %, Soil Moisture: ");
      Serial.print(sensor.soilMoisture);
      Serial.print(" %, Water Flow: ");
      Serial.print(sensor.waterFlow);
      Serial.print(" L/min, Light: ");
      Serial.print(sensor.light);
      Serial.print(" Time: ");
      Serial.print(date.hour); Serial.print(":"); Serial.print(date.minute); Serial.print(":"); Serial.print(date.second);
      Serial.print(" Date: ");
      Serial.print(date.day); Serial.print("/"); Serial.print(date.month); Serial.print("/"); Serial.println(date.year);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void ActuatorControlTask(void *pvParameters) {
  SensorData_t sensor;

  while (1) {
      uint8_t btn;
      if (xQueueReceive(buttonQueue, &btn, 0)) {
        interruptDisable = true;

          switch (btn) {
            case 3: 
            autoMode = !autoMode; 
            Blynk.virtualWrite(Modeblk,autoMode);
            delay(1000);
            Serial.print("autoMode:"); Serial.println(autoMode);
            break;
          }
        interruptDisable = false;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Blynk.begin(auth, ssid, pass);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(36,INPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(DAD, INPUT);
  pinMode(WF, INPUT);

  for (int i = 0; i < 4; i++) pinMode(bt[i], INPUT_PULLUP);
attachInterrupt(digitalPinToInterrupt(bt[0]), handleButton0, FALLING);
attachInterrupt(digitalPinToInterrupt(bt[1]), handleButton1, FALLING);
attachInterrupt(digitalPinToInterrupt(bt[2]), handleButton2, FALLING);
attachInterrupt(digitalPinToInterrupt(bt[3]), handleButton3, FALLING);


  lcd.init(); lcd.backlight();
  dht.begin();

  buttonQueue = xQueueCreate(5, sizeof(uint8_t));
  sensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
  timeQueue = xQueueCreate(1, sizeof(Time_t));
  // xTaskCreatePinnedToCore(coreTwo,"coreTwo",10000,NULL,0,&Core2,0);
  xTaskCreatePinnedToCore(ConfigTask, "ConfigTask", 8192, NULL, 4, &ConfigTask_Handle, 1);

}

void loop() {}
