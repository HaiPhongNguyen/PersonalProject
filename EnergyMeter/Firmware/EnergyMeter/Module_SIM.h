#include <SoftwareSerial.h>

// Định nghĩa chân giao tiếp SIM
#define SIM_RX 27
#define SIM_TX 26

// Khởi tạo SoftwareSerial
SoftwareSerial sim(SIM_RX, SIM_TX);

// Hàm khởi tạo SIM
void sim_init() {
  sim.begin(115200);
  
  delay(3000);
  Serial.println("Khoi dong giao tiep SIM hoan tat");
}
void sendAT(String cmd, int waitTime = 2000) {
  sim.println(cmd);
  Serial.print(">>> "); Serial.println(cmd);

  long int time = millis();
  while ((millis() - time) < waitTime) {
    while (sim.available()) {
      char c = sim.read();
      Serial.write(c);
    }
  }
  Serial.println();
}
// Gửi SMS
void sendSMS(String number, String message) {
  sim.println("AT+CMGF=1");
  delay(500);

  sim.print("AT+CMGS=\"");
  sim.print(number);
  sim.println("\"");
  delay(500);

  sim.print(message);
  delay(500);

  sim.write(26);  // Ctrl+Z
  delay(3000);

  Serial.println("Da gui SMS den " + number);
}

// Gọi điện
void makeCall(String number) {
  sim.print("ATD");
  sim.print(number);
  sim.println(";");
  Serial.println("Dang goi toi " + number);

  delay(15000);  // Gọi 15 giây

  sim.println("ATH");  // Cúp máy
  Serial.println("Da cup may");
}

// Trường hợp 1: chỉ gửi SMS
void case1(String phoneNumber) {
  String message = "Canh bao nhiet do cao";
  sendSMS(phoneNumber, message);
}

// Trường hợp 2: gửi SMS + gọi
void case2(String phoneNumber) {
  String message = "Canh bao dam chay da xay ra";
  sendSMS(phoneNumber, message);

  delay(1000);
  makeCall(phoneNumber);
}
