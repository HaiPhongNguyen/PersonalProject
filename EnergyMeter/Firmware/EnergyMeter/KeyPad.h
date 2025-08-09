#include <SoftWire.h>

#define KEYPAD_I2C_ADDR  0x25

SoftWire kp(32, 33);
char TxBuffer[16];
char RxBuffer[16];

const char keyMap[16] = {
  'D', 'C', 'B', 'A',
  '#', '9', '6', '3',
  '0', '8', '5', '2',
  '*', '7', '4', '1'
};

void KeyPad_Init() {
  kp.setTxBuffer(TxBuffer, sizeof(TxBuffer));
  kp.setRxBuffer(RxBuffer, sizeof(RxBuffer));
  kp.begin();
}

uint8_t KeyPad_Read(uint8_t mask) {
  kp.beginTransmission(KEYPAD_I2C_ADDR);
  kp.write(mask);
  if (kp.endTransmission() != 0) {
    return 0xFF;  // Lỗi
  }
  kp.requestFrom(KEYPAD_I2C_ADDR, (uint8_t)1);
  return kp.read();
}

char KeyPad_GetKey() {
  uint8_t rowVal = KeyPad_Read(0xF0);
  if (rowVal == 0xF0) return 0;  // Không nhấn gì

  uint8_t colVal = KeyPad_Read(0x0F);
  if (colVal == 0x0F) return 0;  // Không nhấn gì

  int row = -1, col = -1;
  switch (rowVal) {
    case 0xE0: row = 0; break;
    case 0xD0: row = 1; break;
    case 0xB0: row = 2; break;
    case 0x70: row = 3; break;
    default: return 0;  // Lỗi hoặc nhiều phím nhấn cùng lúc
  }

  switch (colVal) {
    case 0x0E: col = 0; break;
    case 0x0D: col = 1; break;
    case 0x0B: col = 2; break;
    case 0x07: col = 3; break;
    default: return 0;  // Lỗi hoặc nhiều phím nhấn cùng lúc
  }

  return keyMap[row * 4 + col];
}
String ReadPhoneNumberBlocking() {
  String phoneNumber = "";
  bool reading = true;

  Serial.println("Start reading phone number...");
  
  while (reading) {
    char key = KeyPad_GetKey();
    if (key) {
      Serial.print("Key pressed: ");
      Serial.println(key);

      if (key == '*') {
        phoneNumber = "";  // Reset số điện thoại
        Serial.println("Reset phone number input.");
      } 
      else if (key == '#') {
        Serial.println("End of phone number entry.");
        reading = false;
      } 
      else if (key >= '0' && key <= '9') {
        phoneNumber += key;
        Serial.print("Current phone: ");
        Serial.println(phoneNumber);
      } 
      else {
        Serial.println("Invalid key, ignored.");
      }

      delay(300);  // debounce
    }
  }

  return phoneNumber;
}

String CheckAndStartPhoneInput() {
  static bool waitingForStart = true;
  char key = KeyPad_GetKey();
  String number;
  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);
    delay(300);  // debounce

    if (waitingForStart && key == '*') {
      // Bắt đầu nhập số điện thoạis
      number = ReadPhoneNumberBlocking();
      Serial.print("Phone number entered: ");
      Serial.println(number);
    }
  }
  return number;
}
String ReadPhoneNumberBlocking_LCD() {
  String phoneNumber = "";
  bool reading = true;

  lcd_clear_display();
  lcd_goto_XY(0, 0);
  lcd_send_string("Nhap SDT:");

  while (reading) {
    char key = KeyPad_GetKey();
    if (key) {
      Serial.print("Key pressed: ");
      Serial.println(key);
      delay(300);  // debounce

      if (key == '*') {
        phoneNumber = "";
        lcd_clear_display();
        lcd_goto_XY(0, 0);
        lcd_send_string("Nhap SDT:");
        Serial.println("Reset phone number input.");
      }
      else if (key == '#') {
        reading = false;
        Serial.println("End of phone number entry.");
      }
      else if (key >= '0' && key <= '9') {
        if (phoneNumber.length() < 16) {  // Giới hạn max 16 số
          phoneNumber += key;
          lcd_goto_XY(1, 0);
          lcd_send_string((char *)phoneNumber.c_str());
          Serial.print("Current phone: ");
          Serial.println(phoneNumber);
        } else {
          Serial.println("Max length reached");
        }
      }
      else {
        Serial.println("Invalid key, ignored.");
      }
    }
  }

  return phoneNumber;
}

String CheckAndStartPhoneInput_LCD() {
  char key = KeyPad_GetKey();

  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);
    delay(100);

    if (key == '*') {
      String number = ReadPhoneNumberBlocking_LCD();
      lcd_clear_display();
      lcd_goto_XY(0, 0);
      lcd_send_string("SDT OK:");
      lcd_goto_XY(1, 0);
      lcd_send_string((char *)number.c_str());
      return number;
    }
  }

  return "";
}

