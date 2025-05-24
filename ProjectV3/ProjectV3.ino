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
const char* cardsFile = "/clothes.json"; // 衣物資料檔案

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
void handleFilterCards();  // 新增篩選卡片功能
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

  if (!SD.begin(SD_CS)) Serial.println("SD 卡初始化失敗");
  else Serial.println("SD 卡初始化成功");

  loadCards();

  WiFi.begin(ssid, password);
  Serial.print("連線中");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
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
  server.on("/filter", HTTP_GET, handleFilterCards);  // 新增過濾選項

  server.begin();
  Serial.println("HTTP 伺服器已啟動");
}

void loop() {
  server.handleClient();

  // 掃描逾時
  if (scanning && millis() - scanStartTime >= 5000) {
    stopScanMode();
  }

  // RFID 掃描
  if (scanning && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    stopScanMode();
    String f;
    pendingExists = cardExists(uid, f);
    pendingUID = uid;
    mfrc522.PICC_HaltA();
  }
}

void handleRoot() {
  String html = FPSTR(INDEX_HTML);
  html.replace("%HISTORY%", getHistoryHtml());
  server.send(200, "text/html", html);
}

void handleStartScan() {
  startScanMode();
  server.send(200, "text/plain", "開始掃描，請刷卡（5秒內有效）...");
}

void handleScanStatus() {
  DynamicJsonDocument d(256);
  d["scanning"] = scanning;
  d["pendingUID"] = pendingUID;
  d["pendingExists"] = pendingExists;
  String resp;
  serializeJson(d, resp);
  server.send(200, "application/json", resp);
  if (!pendingUID.isEmpty()) {
    pendingUID.clear();
    pendingExists = false;
  }
}

void handleGetCards() {
  String json;
  serializeJson(cardsDoc, json);
  server.send(200, "application/json", json);
}

void handleAddCard() {
  if (!server.hasArg("plain")) { server.send(400, "text/plain", "Missing body"); return; }
  DynamicJsonDocument d(256);
  if (deserializeJson(d, server.arg("plain"))) { server.send(400, "text/plain", "JSON Parse Error"); return; }
  String uid = d["uid"].as<String>();
  String tmp;
  if (cardExists(uid, tmp)) { server.send(409, "text/plain", "Card exists"); return; }
  JsonObject o = cardsDoc.createNestedObject();
  o["uid"] = uid;
  o["features"] = d["features"];
  saveCards();
  server.send(200, "text/plain", "OK");
}

void handleRenameCard() {
  if (!server.hasArg("uid") || !server.hasArg("plain")) { server.send(400, "text/plain", "Missing args"); return; }
  String uid = server.arg("uid");
  DynamicJsonDocument d(256);
  deserializeJson(d, server.arg("plain"));
  for (JsonVariant v : cardsDoc.as<JsonArray>()) {
    if (v["uid"] == uid) {
      JsonObject feat = v["features"].as<JsonObject>();
      JsonObject nf   = d["features"].as<JsonObject>();
      if (nf.containsKey("style"))    feat["style"]    = nf["style"];
      if (nf.containsKey("color"))    feat["color"]    = nf["color"];
      if (nf.containsKey("position")) feat["position"] = nf["position"];
      saveCards();
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(404, "text/plain", "Not found");
}

void handleDeleteCard() {
  if (!server.hasArg("uid")) { server.send(400, "text/plain", "Missing uid"); return; }
  String uid = server.arg("uid");
  JsonArray arr = cardsDoc.as<JsonArray>();
  for (size_t i = 0; i < arr.size(); i++) {
    if (arr[i]["uid"] == uid) {
      arr.remove(i);
      saveCards();
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(404, "text/plain", "Not found");
}

void handleRecommend() {
  HTTPClient http;
  const char* url = "https://opendata.cwa.gov.tw/api/v1/rest/datastore/F-C0032-001?Authorization=CWA-FFADC16C-8605-4FEA-B52A-02281BA42218&locationName=高雄市";
  http.begin(url);
  int code = http.GET();
  String rec;
  if (code == 200) {
    String payload = http.getString();
    DynamicJsonDocument w(4096);
    if (deserializeJson(w, payload) == DeserializationError::Ok) {
      JsonArray elems = w["records"]["location"][0]["weatherElement"].as<JsonArray>();
      JsonArray wx, ma, mi, ci, po;
      for (JsonVariant e : elems) {
        String name = e["elementName"].as<String>();
        if (name == "Wx")   wx = e["time"].as<JsonArray>();
        else if (name == "MaxT") ma = e["time"].as<JsonArray>();
        else if (name == "MinT") mi = e["time"].as<JsonArray>();
        else if (name == "CI")   ci = e["time"].as<JsonArray>();
        else if (name == "PoP")  po = e["time"].as<JsonArray>();
      }
      rec += "<table border='1'><tr><th>時間</th><th>現象</th><th>最高</th><th>最低</th><th>舒適度</th><th>降雨機率</th></tr>";
      for (size_t i = 0; i < wx.size(); i++) {
        String tr = String(wx[i]["startTime"].as<const char*>()) + " ~ " + String(wx[i]["endTime"].as<const char*>());
        rec += "<tr><td>" + tr + "</td>";
        rec += "<td>" + wx[i]["parameter"]["parameterName"].as<String>() + "</td>";
        rec += "<td>" + String(ma[i]["parameter"]["parameterName"].as<int>()) + "</td>";
        rec += "<td>" + String(mi[i]["parameter"]["parameterName"].as<int>()) + "</td>";
        rec += "<td>" + ci[i]["parameter"]["parameterName"].as<String>() + "</td>";
        rec += "<td>" + String(po[i]["parameter"]["parameterName"].as<int>()) + "</td></tr>";
      }
      rec += "</table>";
      int t0 = ma[0]["parameter"]["parameterName"].as<int>();
      if (t0 >= 28) rec += "<p>炎熱，建議短袖短褲</p>";
      else if (t0 >= 20) rec += "<p>舒適，可長袖薄外套</p>";
      else rec += "<p>較涼，建議保暖外套</p>";
    } else {
      rec = "解析天氣資料錯誤";
    }
  } else {
    rec = "HTTP Error: " + String(code);
  }
  http.end();
  server.send(200, "text/html", rec);
}

void loadCards() {
  // 將原始資料轉換為網頁所需的格式
  // 將轉換後的資料複製回 cardsDoc
  cardsDoc.clear();
  cardsDoc.to<JsonArray>(); // Ensure cardsDoc is an empty array initially

  File f = SD.open(cardsFile, FILE_READ);
  if (!f) {
    Serial.println("無法讀取衣物資料檔");
    return; // Return early if file cannot be opened
  }

  DynamicJsonDocument tempDoc(8192); // Use a temporary document for parsing
  DeserializationError err = deserializeJson(tempDoc, f);
  f.close();

  if (err) {
    Serial.println("解析衣物資料失敗");
    Serial.println(err.c_str());
    return; // Return early if parsing fails
  }

  // If parsing successful and it's a JsonArray, process it
  if (tempDoc.is<JsonArray>()) {
    JsonArray srcArr = tempDoc.as<JsonArray>();
    for (JsonObject item : srcArr) {
      JsonObject newItem = cardsDoc.createNestedObject(); // Add to the global cardsDoc
      newItem["uid"] = item["UUID"].as<const char*>();

      JsonObject feat = newItem.createNestedObject("features");
      feat["style"] = item["款式"].as<const char*>();
      feat["color"] = item["顏色"].as<const char*>();
      feat["position"] = item["位置"].as<const char*>();
    }
  } else {
    Serial.println("衣物資料檔內容不是 JSON 陣列");
    // cardsDoc is already an empty array, so no need to do anything more
  }

  Serial.print("成功載入衣物數量: ");
  Serial.println(cardsDoc.as<JsonArray>().size());
}

void saveCards() {
  SD.remove(cardsFile);
  File f = SD.open(cardsFile, FILE_WRITE);
  if (f) {
    serializeJson(cardsDoc, f);
    f.close();
  }
}

bool cardExists(const String& u, String &found) {
  for (JsonVariant v : cardsDoc.as<JsonArray>()) {
    if (v["uid"] == u) {
      if (v["features"].is<JsonObject>()) found = v["features"]["style"].as<String>();
      return true;
    }
  }
  return false;
}

void startScanMode() {
  scanning = true;
  pendingUID.clear();
  pendingExists = false;
  digitalWrite(LED_PIN, HIGH);
  scanStartTime = millis();
}

void stopScanMode() {
  scanning = false;
  digitalWrite(LED_PIN, LOW);
}

String getHistoryHtml() {
  const char* fp = HISTORY_FILE;
  String html = "<h2>過去穿衣紀錄</h2>";
  if (!SD.exists(fp)) return html + "<p>無歷史紀錄</p>";
  File f = SD.open(fp, FILE_READ);
  if (!f) return html + "<p>讀取失敗</p>";
  DynamicJsonDocument d(4096);
  if (deserializeJson(d, f)) { f.close(); return html + "<p>解析失敗</p>"; }
  f.close();
  JsonArray a = d.as<JsonArray>();
  if (a.size() == 0) return html + "<p>無歷史紀錄</p>";
  html += "<table border='1'><tr>";
  const char* cols[] = {"Time","Wx","MaxT","MinT","CI","PoP","Top","Top-color","Top-type","Bottom","Bottom-color","Bottom-type"};
  for (auto &c : cols) html += String("<th>") + c + "</th>";
  html += "</tr>";
  for (JsonObject o : a) {
    html += "<tr>";
    for (auto &c : cols) {
      if (!strcmp(c, "MaxT") || !strcmp(c, "MinT") || !strcmp(c, "PoP")) {
        html += String("<td>") + o[c].as<int>() + "</td>";
      } else {
        html += String("<td>") + o[c].as<const char*>() + "</td>";
      }
    }
    html += "</tr>";
  }
  html += "</table>";
  return html;
}
void handleFilterCards() {
  // 檢查衣物資料檔案是否存在
  if (!SD.exists(cardsFile)) {
    Serial.println("找不到檔案，創建一個新的檔案");
    return;
  }

  // 打開檔案以讀取
  File f = SD.open(cardsFile, FILE_READ);
  if (!f) {
    Serial.println("無法打開檔案");
    return;
  }

  // 將檔案內容解析為 JSON
  DeserializationError error = deserializeJson(cardsDoc, f);
  if (error) {
    Serial.println("解析 JSON 失敗: " + String(error.c_str()));
  } else {
    Serial.println("成功讀取並解析衣物資料");
  }

  f.close();
}