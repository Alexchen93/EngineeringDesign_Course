#include <SPI.h>
#include <MFRC522.h>

// ESP32 接腳設定，可依實際接線修改
#define SS_PIN   21  // SDA pin
#define RST_PIN  22  // RST pin

MFRC522 rfid(SS_PIN, RST_PIN);

const int MAX_CARDS = 50;         // 最多辨識張數
String detectedUIDs[MAX_CARDS];   // 已偵測 UID 儲存陣列
int detectedCount = 0;            // 已偵測張數

void setup() {
  Serial.begin(115200);
  SPI.begin();                // 初始化 SPI
  rfid.PCD_Init();            // 初始化 RC522
  Serial.println("RFID Reader Ready");
}

void loop() {
  // 是否有新卡片靠近？
  if (!rfid.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  // 讀取卡片序號
  if (!rfid.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }

  // 將 UID 轉成十六進制字串
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  // 檢查是否已記錄過
  bool isDuplicate = false;
  for (int i = 0; i < detectedCount; i++) {
    if (detectedUIDs[i] == uid) {
      isDuplicate = true;
      break;
    }
  }

  // 若為新卡且未超過上限，則儲存並輸出
  if (!isDuplicate) {
    if (detectedCount < MAX_CARDS) {
      detectedUIDs[detectedCount] = uid;
      detectedCount++;
      Serial.print("Card #");
      Serial.print(detectedCount);
      Serial.print(": UID = ");
      Serial.println(uid);
    } else {
      Serial.println(">>> 已達最大記錄張數 50 張，停止記錄新卡");
    }
  }

  // 停止對這張卡的加密通訊，準備下一次偵測
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(100);
}
