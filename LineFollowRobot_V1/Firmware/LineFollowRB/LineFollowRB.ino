#define BLYNK_TEMPLATE_ID "TMPL6sv4EUUKx"
#define BLYNK_TEMPLATE_NAME "smartdoor"
#define BLYNK_AUTH_TOKEN "9QNPdXlYXu2TSKjXZW5bTiFlllEdWp4X"

#include <Servo_ESP32.h>
#include <BlynkSimpleEsp32.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define ON_OFF   V0
#define PID_BLK  V1

#define LD    2
#define LE    16
#define LS    4
#define RD    5
#define RS    17
#define RE    18
#define S1    36
#define S2    34
#define S3    35
#define S4    32
#define S5    33

static const int servoPin = 13; //printed G14 on the board
Servo_ESP32 servo1;
const int PWM_CHANNEL = 0;    // ESP32 has 16 channels which can generate 16 independent waveforms
const int PWM_FREQ = 1900;     // Recall that Arduino Uno is ~490 Hz. Official ESP32 example uses 5,000Hz
const int PWM_RESOLUTION = 8; // We'll use same resolution as Uno (8 bits, 0-255) but ESP32 can go up to 16 bits 
int PWM_FREQ_L;
int PWM_FREQ_R;
// The max duty cycle value based on PWM resolution (will be 255 if resolution is 8 bits)
const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1); 
const int LCHANNEL = 0U;
const int RCHANNEL = 2U;
const int DELAY_MS = 4;  // delay between fade increments
const int trig = 21;     // chân trig của HC-SR04
const int echo = 19;     // chân echo của HC-SR04

int error; int previous_error = 0;
int P; int Kp = 200;
int I; int Ki =0;
int D; int Kd =0;
int PID_value;

char auth[] = BLYNK_AUTH_TOKEN;                // Gán mã token của app blynk vào biến auth
char ssid[] = "Tang8";          // Khai báo tên wifi kết nối và gán vào biến ssid
char pass[] = "123456789@";                      // Khai báo mật khẩu wifi và gán vào biến pass
BlynkTimer timer;

BLYNK_CONNECTED() {
  Blynk.syncVirtual(ON_OFF);
  Blynk.syncVirtual(PID_BLK);
}
uint8_t bt_state_blk;
BLYNK_WRITE(ON_OFF)
{
  bt_state_blk=param.asInt();
}
uint8_t s[5];
void Read_sensor()
{
  s[0] = digitalRead(S1);
  s[1] = digitalRead(S2);
  s[2] = digitalRead(S3);
  s[3] = digitalRead(S4);
  s[4] = digitalRead(S5);
  Serial.print(s[0] ); Serial.print(s[1] ); Serial.print(s[2] ); Serial.print(s[3] ); Serial.print(s[4] ); 
  Serial.println();
  if(s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 1 && s[4] == 1 && previous_error > 0) {error = 5;}
  if(s[0] == 0 && s[1] == 1 && s[2] == 1 && s[3] == 1 && s[4] == 1) {error = 4;}
  if(s[0] == 0 && s[1] == 0 && s[2] == 1 && s[3] == 1 && s[4] == 1) {error = 3;}
  if(s[0] == 1 && s[1] == 0 && s[2] == 1 && s[3] == 1 && s[4] == 1) {error = 2;}
  if(s[0] == 1 && s[1] == 0 && s[2] == 0 && s[3] == 1 && s[4] == 1) {error = 1;}
  if(s[0] == 1 && s[1] == 1 && s[2] == 0 && s[3] == 1 && s[4] == 1) {error = 0;}
  if(s[0] == 1 && s[1] == 1 && s[2] == 0 && s[3] == 0 && s[4] == 1) {error = -1;}
  if(s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 0 && s[4] == 1) {error = -2;}
  if(s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 0 && s[4] == 0) {error = -3;}
  if(s[0] == 1 && s[1] == 1 && s[2] == 1 && s[3] == 1 && s[4] == 0) {error = -4;}

}

void pid_calculation(){
 P = error;
 I = I + error;
 D = D - previous_error;
 PID_value = Kp*P + Ki*I + Kd*D;
 previous_error = error;
//Serial.print("PID: ");Serial.println(PID_value);
}

int initial_motor_speed = 50;
int left_motor_speed; 
int right_motor_speed;

void speedControl(){
  digitalWrite(RE, 0);
  digitalWrite(LE, 0);
  digitalWrite(RD, 0);
  digitalWrite(LD, 0);
  PWM_FREQ_L = PWM_FREQ - PID_value ;
  PWM_FREQ_R = PWM_FREQ + PID_value ;
  if(PWM_FREQ_L < 800) PWM_FREQ_L = 800;
  if(PWM_FREQ_L > 3000) PWM_FREQ_L = 3000;
  if(PWM_FREQ_R < 800) PWM_FREQ_R = 800;
  if(PWM_FREQ_R > 3000) PWM_FREQ_R = 3000;

  ledcSetup(LCHANNEL, PWM_FREQ_L, PWM_RESOLUTION);
  ledcSetup(RCHANNEL, PWM_FREQ_R, PWM_RESOLUTION);

  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 120);
  Serial.print(PWM_FREQ_L); Serial.print("   "); Serial.println(PWM_FREQ_R);
}
int measure_distance()
{
  unsigned long duration; // biến đo thời gian
    int distance;           // biến lưu khoảng cách
    
    /* Phát xung từ chân trig */
    digitalWrite(trig,0);   // tắt chân trig
    delayMicroseconds(2);
    digitalWrite(trig,1);   // phát xung từ chân trig
    delayMicroseconds(5);   // xung có độ dài 5 microSeconds
    digitalWrite(trig,0);   // tắt chân trig
    
    /* Tính toán thời gian */
    // Đo độ rộng xung HIGH ở chân echo. 
    duration = pulseIn(echo,HIGH);  
    // Tính khoảng cách đến vật.
    distance = int(duration/2/29.412);
  return distance;
}
void turn_right()
{
  /* Motor right stop motor left full speed */
  digitalWrite(RE, 0);
  digitalWrite(LE, 0);
  digitalWrite(RD, 0);
  digitalWrite(LD, 0);
  ledcSetup(LCHANNEL, PWM_FREQ, 1200);
  ledcSetup(RCHANNEL, PWM_FREQ_R, 1200);

  digitalWrite(RD, 0);
  digitalWrite(LD, 0);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 0);
  delay(1000);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 120);
  delay(500);
  ledcWrite(LCHANNEL, 0);
  ledcWrite(RCHANNEL, 120);
  delay(1200);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 120);
  delay(500);
  digitalWrite(RE, 1);
  digitalWrite(LE, 1);
}
void turn_left()
{
  /* Motor right stop motor left full speed */
  digitalWrite(RE, 0);
  digitalWrite(LE, 0);
  digitalWrite(RD, 0);
  digitalWrite(LD, 0);
  ledcSetup(LCHANNEL, PWM_FREQ, 1200);
  ledcSetup(RCHANNEL, PWM_FREQ_R, 1200);
  digitalWrite(RD, 0);
  digitalWrite(LD, 0);
  ledcWrite(LCHANNEL, 0);
  ledcWrite(RCHANNEL, 120);
  delay(1000);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 120);
  delay(500);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 0);
  delay(1200);
  ledcWrite(LCHANNEL, 120);
  ledcWrite(RCHANNEL, 120);
  delay(300);
  digitalWrite(RE, 1);
  digitalWrite(LE, 1);
}
int Distance_C = 0, Distance_L = 0, Distance_R = 0;
uint8_t checking()
{
  servo1.write(90);
  Distance_C =  measure_distance();
  Serial.print("Kc trung tam: "); Serial.println(Distance_C);
  if(Distance_C < 21 && Distance_C > 3)
  {
    digitalWrite(RE, 1);
    digitalWrite(LE, 1);
    for(uint8_t angle = 90; angle < 180; angle+=5)
    {
      servo1.write(angle);
      delay(10);
    }
    delay(800);
    Distance_L = measure_distance();


    for(uint8_t angle = 180; angle > 0; angle-=5)
    {
      servo1.write(angle);
      delay(10);
    }
    delay(800);
    Distance_R = measure_distance();
    Serial.print("Kc ben trai: "); Serial.println(Distance_L);
    Serial.print("Kc ben phai: "); Serial.println(Distance_R);
    for(uint8_t angle = 0; angle < 90; angle+=5)
    {
      servo1.write(angle);
      delay(10);
    }
    if(Distance_L < 21 && Distance_R > 21 && Distance_L > 3 && Distance_R > 3)
    {
      // re phai
      return 1;
    }
    else if(Distance_L > 20 && Distance_R < 21 && Distance_L > 3 && Distance_R > 3)
    {
      // re trai
      return 2;
    }
    else if(Distance_L > 21 && Distance_R > 21 && Distance_L > 3 && Distance_R > 3)
    {
      // vat can o giua
      return 3;
    }
    else
    {
      return 4;
      // vat ca 3 phia nghi chay
    }
  }
  return 0;
}

void setup() {
  // put your setup code here, to run once:
  pinMode(LS,OUTPUT);
  pinMode(LE,OUTPUT);
  pinMode(LD,OUTPUT);
  pinMode(RS,OUTPUT);
  pinMode(RE,OUTPUT);
  pinMode(RD,OUTPUT);
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
  pinMode(S3, INPUT);
  pinMode(S4, INPUT);
  pinMode(S5, INPUT);
  pinMode(trig,OUTPUT);   // chân trig sẽ phát tín hiệu
  pinMode(echo,INPUT);    // chân echo sẽ nhận tín hiệu
  Blynk.begin(auth, ssid, pass); // Chạy blynk với các thông tin ta đã cung cấp
  // Sets up a channel (0-15), a PWM duty cycle frequency, and a PWM resolution (1 - 16 bits) 
  // ledcSetup(uint8_t channel, double freq, uint8_t resolution_bits);
  ledcSetup(LCHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(RCHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // ledcAttachPin(uint8_t pin, uint8_t channel);
  ledcAttachPin(RS, LCHANNEL);
  ledcAttachPin(LS, RCHANNEL);
  servo1.attach(servoPin, 5, 0, 180, 544, 2400);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Blynk.run();
  timer.run();
  uint8_t isTurn = 0;
  if(bt_state_blk == 0)
  { 
        servo1.write(90);
      digitalWrite(LE, 1);
      digitalWrite(RE, 1);
      PID_value = 0;
      isTurn = 0;
      Distance_C = 0;
      Distance_L = 0;
      Distance_R = 0;
  }
  else
  {

    
    Distance_C = 0;
    Distance_L = 0;
    Distance_R = 0;
    isTurn = checking();
    if(isTurn == 1)
    {
      turn_right();
      digitalWrite(LE, 1);
      digitalWrite(RE, 1);
      isTurn = 0;
      Distance_C = 0;
      Distance_L = 0;
      Distance_R = 0;
      PID_value = 0;
    }
    else if(isTurn == 2 || isTurn == 3)
    {
      turn_left();
      digitalWrite(LE, 1);
      digitalWrite(RE, 1);
      isTurn = 0;
      Distance_C = 0;
      Distance_L = 0;
      Distance_R = 0;     
      PID_value = 0;
    }
    else if(isTurn == 4)
    {
      digitalWrite(LE, 1);
      digitalWrite(RE, 1);
      isTurn = 0;
      Distance_C = 0;
      Distance_L = 0;
      Distance_R = 0;
      PID_value = 0;
    }
    else if(isTurn == 0)
    { 
      digitalWrite(LE, 0);
      digitalWrite(RE, 0);
      Read_sensor();
      pid_calculation();
      speedControl();
      Blynk.virtualWrite(PID_BLK , PID_value);
    }
    
    
  }
}
