#include <SPI.h>
#include <SD.h>
#include <MFRC522.h>

// ======= 腳位設定 =======
#define RST_PIN   22    // RC522 Reset
#define SS_PIN    21    // RC522 SPI SS
#define SD_CS     5     // SD 卡片選擇

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);

  // 確保 CS 釋放
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);

  // 初始化 SPI 及 RC522
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID 初始化完成");

  // 初始化 SD 卡
  if (!SD.begin(SD_CS)) {*
    Serial.println("SD 卡初始化失敗！");
  } else {
    Serial.println("SD 卡初始化成功");
  }
}

void loop() {
  // 確保 SD 卡不佔用 SPI
  digitalWrite(SD_CS, HIGH);

  // 掃描 RFID 卡片
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // 讀取 UID
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.println("讀取到卡片 UID: " + uid);

    // 將 UID 寫入 SD 卡
    File file = SD.open("/test.txt", FILE_APPEND);
    if (file) {
      file.println(uid);
      file.close();
      Serial.println("已將 UID 記錄至 SD 卡");
    } else {
      Serial.println("開啟檔案失敗");
    }

    mfrc522.PICC_HaltA();
    delay(1000); // 避免重複讀取
  }
}
