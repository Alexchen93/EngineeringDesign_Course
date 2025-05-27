#include <SPI.h>
#include <MFRC522.h>

// 依實際接線設定 SS（SDA）與 RST 腳位
#define SS_PIN   21
#define RST_PIN  22

MFRC522 mfrc522(SS_PIN, RST_PIN);

const int MAX_CARDS = 50;      // 最多記錄的卡片數量
String uidList[MAX_CARDS];     // 用來儲存已掃到的 UID
int uidCount = 0;              // 已記錄的卡片數量

void setup() {
  Serial.begin(115200);
  SPI.begin();                  // 初始化 SPI 線路
  mfrc522.PCD_Init();           // 初始化 MFRC522
  Serial.println("RFID Reader Ready. Waiting for cards...");
}

void loop() {
  // 檢查是否有新卡片靠近
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  // 嘗試讀取卡片序號
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // 組出十六進位的 UID 字串
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();  // 轉成大寫，方便比較

  // 檢查是否為新卡
  bool isNew = true;
  for (int i = 0; i < uidCount; i++) {
    if (uidList[i] == uid) {
      isNew = false;
      break;
    }
  }

  if (isNew) {
    if (uidCount < MAX_CARDS) {
      uidList[uidCount++] = uid;
      Serial.print("New card [");
      Serial.print(uidCount);
      Serial.print("/");
      Serial.print(MAX_CARDS);
      Serial.print("]: ");
      Serial.println(uid);
    } else {
      Serial.println(">>> 已經掃描到 50 張不同的卡片，程式停止偵測。");
      // 避免繼續偵測，進入無限迴圈
      while (true) {
        delay(1000);
      }
    }
  }
  // 如果是重複 UID，就不輸出

  // 停止對這張卡的偵測，並稍作延遲，避免連續多次讀取同張卡
  mfrc522.PICC_HaltA();
  delay(500);
}
