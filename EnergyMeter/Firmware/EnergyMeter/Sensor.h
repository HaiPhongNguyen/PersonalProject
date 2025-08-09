#include "DHT.h"

#define DHTPIN 13
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// Gọi hàm này 1 lần để khởi động cảm biến
void init_sensors() {
  dht.begin();
}

// Đọc nhiệt độ (°C)
float read_temperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    return NAN;
  }
  return t;
}

