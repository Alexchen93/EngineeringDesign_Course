//SDA	GPIO 21	SS (Slave Select)
//SCK	GPIO 18	SPI Clock
//MOSI	GPIO 23	SPI 主輸出從輸入
//MISO	GPIO 19	SPI 主輸入從輸出
//IRQ	不接	中斷請求
//GND	GND	接地
//RST	GPIO 22	重置
//3.3V	3.3V	電源供應

#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN  22   // RC522 RST 接腳
#define SS_PIN   21   // RC522 SDA 接腳

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  SPI.begin();           // 啟動 SPI
  rfid.PCD_Init();       // 初始化 RC522
  Serial.println("RC522 準備好了，請刷卡...");
}

void loop() {
  // 檢查是否有新卡片靠近
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("感應到卡片 UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // 停止與這張卡片通訊
  rfid.PICC_HaltA();
}
