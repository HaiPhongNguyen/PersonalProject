#define BLYNK_TEMPLATE_ID "TMPL6Nvd8Yvtu"
#define BLYNK_TEMPLATE_NAME "DogRobot"
#define BLYNK_AUTH_TOKEN "b_hx3TRvhDZ9k3khB_Wl-8yTAi1satwJ"

#include <Servo_ESP32.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>

// Chân kết nối Servo
#define PH_DUOI_KHOP1_PIN   33
#define PH_DUOI_KHOP2_PIN   32
#define PH_TREN_KHOP1_PIN   18
#define PH_TREN_KHOP2_PIN   4
#define TRAI_DUOI_KHOP1_PIN 27
#define TRAI_DUOI_KHOP2_PIN 26
#define TRAI_TREN_KHOP1_PIN 19
#define TRAI_TREN_KHOP2_PIN 5

// Khai báo các đối tượng Servo
Servo_ESP32 ph_duoi_k1, ph_duoi_k2;
Servo_ESP32 ph_tren_k1, ph_tren_k2;
Servo_ESP32 trai_duoi_k1, trai_duoi_k2;
Servo_ESP32 trai_tren_k1, trai_tren_k2;

// Kết nối WiFi và Blynk
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Tang8";
char pass[] = "123456789@";
BlynkTimer timer;

// Các nút điều khiển trên Blynk
// Các nút điều khiển Blynk
#define LEN   V0
#define XUONG V1
#define TRAI  V2
#define PHAI  V3
#define DUNG  V4
#define NGOI  V5

// Nhận tín hiệu Blynk
uint8_t len = 0, xuong = 0, trai = 0, phai = 0, dung = 0, ngoi = 0;
BLYNK_WRITE(LEN)   { len = param.asInt(); }
BLYNK_WRITE(XUONG) { xuong = param.asInt(); }
BLYNK_WRITE(TRAI)  { trai = param.asInt(); }
BLYNK_WRITE(PHAI)  { phai = param.asInt(); }
BLYNK_WRITE(DUNG)  { dung = param.asInt(); }
BLYNK_WRITE(NGOI)  { ngoi = param.asInt(); }

// Đồng bộ khi kết nối lại Blynk
BLYNK_CONNECTED() {
  Blynk.syncVirtual(LEN);
  Blynk.syncVirtual(XUONG);
  Blynk.syncVirtual(TRAI);
  Blynk.syncVirtual(PHAI);
  Blynk.syncVirtual(DUNG);
  Blynk.syncVirtual(NGOI);
}


// Tư thế đứng mặc định
void tuTheDung() {
  ph_duoi_k1.write(30);
  ph_duoi_k2.write(36);
  trai_duoi_k1.write(120);
  trai_duoi_k2.write(83);
  ph_tren_k1.write(50);
  ph_tren_k2.write(30);
  trai_tren_k1.write(120);
  trai_tren_k2.write(115);
  Serial.println("=> Robot ở tư thế ĐỨNG");
}

// Xử lý lệnh Serial để test servo riêng lẻ
void docSerialVaDieuKhienServo() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();  // Xoá khoảng trắng

    if (input.length() < 5 || input.charAt(3) != '_') {
      Serial.println("⚠️ Lệnh sai. Định dạng: tu1_90");
      return;
    }

    char ben = input.charAt(0);    // 't' (trái) hoặc 'p' (phải)
    char viTri = input.charAt(1);  // 'u' (trên) hoặc 'd' (dưới)
    char khop = input.charAt(2);   // '1' hoặc '2'
    int goc = input.substring(4).toInt();

    if ((ben != 't' && ben != 'p') || 
        (viTri != 'u' && viTri != 'd') || 
        (khop != '1' && khop != '2') || 
        goc < 0 || goc > 240) {
      Serial.println("⚠️ Tham số sai. Định dạng đúng: tu1_90");
      return;
    }

    Servo_ESP32* servo = nullptr;

    if (ben == 'p') {
      servo = (viTri == 'u') ? ((khop == '1') ? &ph_tren_k1 : &ph_tren_k2) :
                               ((khop == '1') ? &ph_duoi_k1 : &ph_duoi_k2);
    } else {
      servo = (viTri == 'u') ? ((khop == '1') ? &trai_tren_k1 : &trai_tren_k2) :
                               ((khop == '1') ? &trai_duoi_k1 : &trai_duoi_k2);
    }

    if (servo) {
      servo->write(goc);
      Serial.printf("✅ Servo [%c%c%c] = %d độ\n", ben, viTri, khop, goc);
    } else {
      Serial.println("❌ Không tìm thấy servo phù hợp.");
    }
  }
}
void chucNangLen() {

  // BƯỚC 4: Hạ chân B xuống
  delay(50);
  trai_tren_k1.write(120); delay(25);
  trai_tren_k2.write(115); delay(25);
  ph_duoi_k1.write(30); delay(25);
  ph_duoi_k2.write(36); delay(25);

  // BƯỚC 1: Trong khi chân B hạ xuống, chân A đưa ra.
  trai_duoi_k2.write(90); delay(35);  
  trai_duoi_k1.write(100);  delay(25);
  trai_duoi_k2.write(65);  delay(50);

  ph_tren_k2.write(5);     delay(35);
  ph_tren_k1.write(70);    delay(25);
  ph_tren_k2.write(40);    delay(50);

  delay(50); 

  // BƯỚC 3: Trong khi **chân B** bắt đầu đưa ra, chân A hạ xuống.
  trai_tren_k2.write(140); delay(35);
  trai_tren_k1.write(100);  delay(25);
  trai_tren_k2.write(115); delay(50);

  ph_duoi_k2.write(15);     delay(35);
  ph_duoi_k1.write(50);     delay(25);
  ph_duoi_k2.write(34);     delay(50);

  delay(50);

  // BƯỚC 2: Hạ chân A xuống
  ph_tren_k1.write(50); delay(25);
  ph_tren_k2.write(30); delay(25);
  trai_duoi_k1.write(120); delay(25);
  trai_duoi_k2.write(83);  delay(25);
}
void SangTrai()
{
  // ph_duoi_k2.write(0);     delay(35);
  // ph_duoi_k1.write(65);     delay(35);
  // ph_duoi_k2.write(52);     delay(100);
  // delay(100);
  // ph_tren_k2.write(10);     delay(35);
  // ph_tren_k1.write(70);    delay(35);
  // ph_tren_k2.write(62);    delay(110);
  ph_duoi_k2.write(0);     delay(50);
  ph_duoi_k1.write(50);     delay(25);
  ph_duoi_k2.write(34);     delay(50);
  delay(100);
  ph_tren_k2.write(5);     delay(35);
  ph_tren_k1.write(70);    delay(25);
  ph_tren_k2.write(40);    delay(50);
  delay(100);
  tuTheDung();
  delay(100);
}
void SangPhai()
{
  trai_duoi_k2.write(120); delay(35);  
  trai_duoi_k1.write(88);  delay(25);
  trai_duoi_k2.write(55);  delay(50);
  delay(100);
  trai_tren_k2.write(135); delay(35);
  trai_tren_k1.write(90);  delay(25);
  trai_tren_k2.write(102); delay(50);
  delay(100);
  tuTheDung(); // Trở về tư thế đứng
  delay(100);
}
void DiLui() {
    // BƯỚC 4: Hạ chân B xuống
  trai_tren_k1.write(120); delay(25);
  trai_tren_k2.write(115); delay(25);
  ph_duoi_k1.write(30); delay(25);
  ph_duoi_k2.write(36); delay(25);

  // BƯỚC 1: Trong khi chân B hạ xuống, chân A đưa ra.
  trai_duoi_k2.write(130); delay(40);
  trai_duoi_k1.write(130);  delay(25);
  trai_duoi_k2.write(82);  delay(50);

  ph_tren_k2.write(10);     delay(15);
  ph_tren_k1.write(40);    delay(15);
  ph_tren_k2.write(30);    delay(50);

  delay(100);

  // BƯỚC 3: Trong khi **chân B** bắt đầu đưa ra, chân A hạ xuống.
  ph_duoi_k2.write(10);     delay(25);
  ph_duoi_k1.write(15);    delay(40);
  ph_duoi_k2.write(34);    delay(50);

  trai_tren_k2.write(135); delay(15);
  trai_tren_k1.write(130);  delay(15);
  trai_tren_k2.write(115);  delay(50);
    
  delay(100);

  // BƯỚC 2: Hạ chân A xuống
  ph_tren_k1.write(50); delay(25);
  ph_tren_k2.write(30); delay(50);
  trai_duoi_k1.write(120); delay(25);
  trai_duoi_k2.write(83);  delay(50);
}


void setup() {
  Serial.begin(115200);

  // Gắn chân servo
  ph_duoi_k1.attach(PH_DUOI_KHOP1_PIN);
  ph_duoi_k2.attach(PH_DUOI_KHOP2_PIN);
  ph_tren_k1.attach(PH_TREN_KHOP1_PIN);
  ph_tren_k2.attach(PH_TREN_KHOP2_PIN);
  trai_duoi_k1.attach(TRAI_DUOI_KHOP1_PIN);
  trai_duoi_k2.attach(TRAI_DUOI_KHOP2_PIN);
  trai_tren_k1.attach(TRAI_TREN_KHOP1_PIN);
  trai_tren_k2.attach(TRAI_TREN_KHOP2_PIN);

  tuTheDung(); // Khởi động ở tư thế đứng
  // delay(10000);

  Blynk.begin(auth, ssid, pass);
}

void loop() {
  Blynk.run();
  timer.run();
  if(len == 1)         {  chucNangLen();   } 
  else if(trai == 1)   {  SangTrai();      }
  else if(phai == 1)   {  SangPhai();      }
  else if(xuong == 1)  {  DiLui();         }
  else {
    docSerialVaDieuKhienServo(); // Cho phép nhập Serial để test từng servo
    tuTheDung();
  }
}
