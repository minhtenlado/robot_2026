#include <Arduino.h>

void setup() {
  // Khởi tạo cổng Serial với tốc độ 115200
  Serial.begin(115200);
  
  // Khởi tạo chân LED
  pinMode(LED_BUILTIN, OUTPUT);

  // Đợi một chút để cổng Serial trên máy tính kịp nhận Pico W
  delay(2000); 
  Serial.println("====================================");
  Serial.println("🚀 Pico W đã khởi động thành công!");
  Serial.println("Sẵn sàng cho dự án Robot của Đô!");
  Serial.println("====================================");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  
  Serial.println("Đèn LED: [BẬT]  - Tín hiệu: HIGH");
  delay(500);                       
  
  digitalWrite(LED_BUILTIN, LOW);   
  Serial.println("Đèn LED: [TẮT]  - Tín hiệu: LOW");
  delay(500);                       
}