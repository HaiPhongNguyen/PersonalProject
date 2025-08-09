#include <WiFi.h>
#include "time.h"

// Cấu hình NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;     // Múi giờ Việt Nam GMT+7
const int   daylightOffset_sec = 0;       // Không dùng DST

// Gọi hàm này sau khi Wi-Fi đã kết nối
String getDateFromNTP() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Không lấy được thời gian từ NTP");
    return "";
  }

  // Trả về chuỗi dạng "yy/MM/dd,HH:mm"
  char buffer[20];
  sprintf(buffer, "%02d/%02d/%02d,%02d:%02d",
          timeinfo.tm_year % 100,
          timeinfo.tm_mon + 1,
          timeinfo.tm_mday,
          timeinfo.tm_hour,
          timeinfo.tm_min);
  return String(buffer);
}
int extractDay(String dateStr) {
  if (dateStr.length() >= 8 && dateStr.charAt(2) == '/' && dateStr.charAt(5) == '/') {
    return dateStr.substring(6, 8).toInt();
  }
  return -1;
}
