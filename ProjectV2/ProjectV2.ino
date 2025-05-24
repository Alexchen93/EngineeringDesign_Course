#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "WebPage.h"   // 前端頁面

// ===== 設定 =====
const char* ssid = "ite-441";
const char* password = "ite12345";

#define RST_PIN      22    // RC522 Reset
#define SS_PIN       21    // RC522 SS
#define LED_PIN      2     // 掃描指示燈
#define SD_CS        5     // SD 卡 CS
#define HISTORY_FILE "/history.json"

MFRC522 mfrc522(SS_PIN, RST_PIN);
WebServer server(80);
DynamicJsonDocument cardsDoc(2048);
bool scanning = false;
String pendingUID = "";
bool pendingExists = false;
unsigned long scanStartTime = 0;

// 函式宣告
void handleRoot();
void handleStartScan();
void handleScanStatus();
void handleGetCards();
void handleAddCard();
void handleRenameCard();
void handleDeleteCard();
void handleRecommend();
void loadCards();
void saveCards();
bool cardExists(const String& uid, String &found);
void startScanMode();
void stopScanMode();
String getHistoryHtml();

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID 初始化完成");

  if(!SD.begin(SD_CS)) Serial.println("SD 卡初始化失敗");
  else Serial.println("SD 卡初始化成功");

  loadCards();

  WiFi.begin(ssid, password);
  Serial.print("連線中");
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi 已連線, IP: " + WiFi.localIP().toString());

  // 路由設定
  server.on("/", HTTP_GET, handleRoot);
  server.on("/startScan", HTTP_POST, handleStartScan);
  server.on("/scanStatus", HTTP_GET, handleScanStatus);
  server.on("/cards", HTTP_GET, handleGetCards);
  server.on("/cards", HTTP_POST, handleAddCard);
  server.on("/cards", HTTP_PUT, handleRenameCard);
  server.on("/cards", HTTP_DELETE, handleDeleteCard);
  server.on("/recommend", HTTP_GET, handleRecommend);

  server.begin();
  Serial.println("HTTP 伺服器已啟動");
}

void loop() {
  server.handleClient();

  // 掃描逾時
  if(scanning && millis() - scanStartTime >= 5000) {
    Serial.println("掃描逾時，關閉 RFID 掃描模式");
    stopScanMode();
  }

  // RFID 掃描
  if(scanning && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uidStr;
    for(byte i = 0; i < mfrc522.uid.size; i++) {
      if(mfrc522.uid.uidByte[i] < 0x10) uidStr += "0";
      uidStr += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidStr.toUpperCase();
    Serial.println("讀取到卡片 UID: " + uidStr);

    stopScanMode();
    String f;
    pendingExists = cardExists(uidStr, f);
    pendingUID = uidStr;
    mfrc522.PICC_HaltA();
  }
}

// 主頁面
void handleRoot() {
  String html = FPSTR(INDEX_HTML);
  html.replace("%HISTORY%", getHistoryHtml());
  server.send(200, "text/html", html);
}

// 開始掃描模式
void handleStartScan() {
  startScanMode();
  server.send(200, "text/plain", "開始掃描，請刷卡（5秒內有效）...");
}

// 查詢掃描狀態
void handleScanStatus() {
  DynamicJsonDocument d(256);
  d["scanning"] = scanning;
  d["pendingUID"] = pendingUID;
  d["pendingExists"] = pendingExists;
  String resp;
  serializeJson(d, resp);
  server.send(200, "application/json", resp);
  if(!pendingUID.isEmpty()) { pendingUID.clear(); pendingExists = false; }
}

// 取得卡片清單
void handleGetCards() {
  String json;
  serializeJson(cardsDoc, json);
  server.send(200, "application/json", json);
}

// 新增卡片
void handleAddCard() {
  if(!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }
  DynamicJsonDocument d(256);
  auto err = deserializeJson(d, server.arg("plain"));
  if(err) { server.send(400, "text/plain", "JSON Parse Error"); return; }
  String uid = d["uid"].as<String>();
  String tmp;
  if(cardExists(uid, tmp)) { server.send(409, "text/plain", "Card exists"); return; }
  JsonObject obj = cardsDoc.createNestedObject();
  obj["uid"] = uid;
  obj["features"] = d["features"];
  saveCards();
  server.send(200, "text/plain", "OK");
}

// 修改卡片資訊
void handleRenameCard() {
  if(!server.hasArg("uid") || !server.hasArg("plain")) { server.send(400, "text/plain", "Missing args"); return; }
  String uid = server.arg("uid");
  DynamicJsonDocument d(256);
  deserializeJson(d, server.arg("plain"));
  JsonArray arr = cardsDoc.as<JsonArray>();
  for(JsonVariant v: arr) {
    if(v["uid"] == uid) {
      JsonObject feat = v["features"].as<JsonObject>();
      JsonObject nf = d["features"].as<JsonObject>();
      if(nf.containsKey("style")) feat["style"] = nf["style"];
      if(nf.containsKey("color")) feat["color"] = nf["color"];
      if(nf.containsKey("position")) feat["position"] = nf["position"];
      saveCards();
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(404, "text/plain", "Not found");
}

// 刪除卡片
void handleDeleteCard() {
  if(!server.hasArg("uid")) { server.send(400, "text/plain", "Missing uid"); return; }
  String uid = server.arg("uid");
  JsonArray arr = cardsDoc.as<JsonArray>();
  for(size_t i = 0; i < arr.size(); i++) {
    if(arr[i]["uid"] == uid) { arr.remove(i); saveCards(); server.send(200, "text/plain", "OK"); return; }
  }
  server.send(404, "text/plain", "Not found");
}

// 推薦穿搭
void handleRecommend() {
  HTTPClient http;
  String apiUrl = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/F-C0032-001?Authorization=CWA-FFADC16C-8605-4FEA-B52A-02281BA42218&locationName=高雄市";
  http.begin(apiUrl);
  int httpCode = http.GET();
  String recommendation;
  if(httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument weatherDoc(4096);
    if(!deserializeJson(weatherDoc, payload)) {
      auto elements = weatherDoc["records"]["location"][0]["weatherElement"].as<JsonArray>();
      // 處理各要素
      JsonArray wxTime, maxTTime, minTTime, ciTime, popTime;
      for(auto el: elements) {
        String name = el["elementName"].as<String>();
        if(name == "Wx") wxTime = el["time"].as<JsonArray>();
        else if(name == "MaxT") maxTTime = el["time"].as<JsonArray>();
        else if(name == "MinT") minTTime = el["time"].as<JsonArray>();
        else if(name == "CI") ciTime = el["time"].as<JsonArray>();
        else if(name == "PoP") popTime = el["time"].as<JsonArray>();
      }
      // 建表格
      recommendation += "<table border='1'><tr><th>時間</th><th>現象</th><th>最高</th><th>最低</th><th>舒適度</th><th>降雨機率</th></tr>";
      for(size_t i=0; i<wxTime.size(); i++) {
        String timeRange = String(wxTime[i]["startTime"].as<const char*>()) + " ~ " + wxTime[i]["endTime"].as<const char*>();
        recommendation += "<tr>" +
          "<td>"+timeRange+"</td>" +
          "<td>"+wxTime[i]["parameter"]["parameterName"].as<String>()+"</td>" +
          "<td>"+maxTTime[i]["parameter"]["parameterName"].as<String>()+"</td>" +
          "<td>"+minTTime[i]["parameter"]["parameterName"].as<String>()+"</td>" +
          "<td>"+ciTime[i]["parameter"]["parameterName"].as<String>()+"</td>" +
          "<td>"+popTime[i]["parameter"]["parameterName"].as<String>()+"</td>" +
        "</tr>";
      }
      recommendation += "</table>";
      int maxTemp = maxTTime[0]["parameter"]["parameterName"].as<String>().toInt();
      if(maxTemp >= 28) recommendation += "<p>炎熱，建議短袖短褲</p>";
      else if(maxTemp >= 20) recommendation += "<p>舒適，可長袖薄外套</p>";
      else recommendation += "<p>較涼，建議保暖外套</p>";
    } else recommendation = "解析天氣資料錯誤";
  } else recommendation = "HTTP Error: " + String(httpCode);
  http.end();
  server.send(200, "text/html", recommendation);
}

// 讀取卡片資料
void loadCards() {
  if(!SD.exists(cardsDoc.capacity() ? cardsDoc.as<JsonArray>()[0].as<const char*>() : "/cards.json")) {
    cardsDoc.to<JsonArray>(); saveCards(); return;
  }
  File f = SD.open("/cards.json", FILE_READ);
  if(f) {
    deserializeJson(cardsDoc, f);
    f.close();
  }
}

// 儲存卡片資料
void saveCards() {
  SD.remove("/cards.json");
  File f = SD.open("/cards.json", FILE_WRITE);
  if(f) {
    serializeJson(cardsDoc, f);
    f.close();
  }
}

// 檢查是否已登記
bool cardExists(const String& uid, String &found) {
  JsonArray arr = cardsDoc.as<JsonArray>();
  for(auto v: arr) {
    if(v["uid"] == uid) {
      if(v["features"].is<JsonObject>())
        found = v["features"]["style"].as<String>();
      return true;
    }
  }
  return false;
}

// 掃描開始/停止
void startScanMode() {
  scanning = true; pendingUID.clear(); pendingExists = false; digitalWrite(LED_PIN, HIGH); scanStartTime = millis();
}
void stopScanMode() {
  scanning = false; digitalWrite(LED_PIN, LOW);
}

// 讀取歷史紀錄
String getHistoryHtml() {
  if(!SD.exists(HISTORY_FILE)) return "<p>無歷史紀錄</p>";
  File f = SD.open(HISTORY_FILE, FILE_READ);
  if(!f) return "<p>無法讀取歷史紀錄</p>";
  DynamicJsonDocument doc(4096);
  if(deserializeJson(doc, f)) { f.close(); return "<p>解析失敗</p>"; }
  f.close();
  JsonArray arr = doc.as<JsonArray>();
  if(arr.isNull() || arr.size()==0) return "<p>無歷史紀錄</p>";
  String html = "<table border='1'><tr>";
  for(auto kv: arr[0].as<JsonObject>()) html += "<th>"+String(kv.key().c_str())+"</th>";
  html += "</tr>";
  for(auto obj: arr) {
    html += "<tr>";
    for(auto kv: obj.as<JsonObject>()) html += "<td>"+String(kv.value().as<const char*>())+"</td>";
    html += "</tr>";
  }
  html += "</table>";
  return html;
}
