#include <Arduino.h>

const int SENSOR1_PIN = 34;
const int SENSOR2_PIN = 35;

void setup() {
  Serial.begin(115200);           // 設定序列埠鮑率
  pinMode(SENSOR1_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);
}

void loop() {
  int val1 = analogRead(SENSOR1_PIN);
  int val2 = analogRead(SENSOR2_PIN);

  // 輸出格式：時間戳, sensor1, sensor2
  unsigned long ts = millis();
  Serial.print(ts);
  Serial.print(',');
  Serial.print(val1);
  Serial.print(',');
  Serial.println(val2);

  delay(1000);
}
