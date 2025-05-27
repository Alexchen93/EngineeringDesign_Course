// ProjectV5_modified_fixed.ino
// 修改：從 Goc:\Users\41117\Downloads\V7\WebPage.txtogle Sheets 轉為 SD 卡存儲
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <HTTPClient.h>  // 保留 HTTPClient 引用，用於天氣 API
#include "WebPage.h"

// ====== WiFi & Web App ======
const char* ssid = "ite-441";
const char* password = "ite12345";

// ====== SD 卡設定 ======
#define SD_CS_PIN 5  // SD 卡模組的 CS 腳位，請根據實際連接調整

// ====== RFID 設定 ======
#define RST_PIN   22
#define SS_PIN    21
#define LED_PIN   2

MFRC522 mfrc522(SS_PIN, RST_PIN);
WebServer server(80);
const char* CWA_API = "CWA-FFADC16C-8605-4FEA-B52A-02281BA42218";

// ====== 檔案路徑設定 ======
const char* CLOTHES_FILE = "/clothes.json";
const char* HISTORY_FILE = "/history.json";

// **預設為空陣列，避免 JSON 解析錯誤**
String clothesCache = "[]";
String historyCache = "[]";

unsigned long lastCache = 0;
const unsigned long CACHE_INTERVAL = 300000;

bool scanning = false;
String pendingUID;
bool pendingExists = false;
unsigned long scanStart = 0;

int curWx, curMax, curMin, curCI, curPop;

// Forward declarations
void handleRoot();
void handleStartScan();
void handleScanStatus();
void handleGetCards();
void handleAddCard();
void handleRecommend();
void handleSimilarHistory();
void handleUpdateCard();
void handleDeleteCard();
void refreshCache();
void startScanMode();
void stopScanMode();
bool cardExists(const String&, String&);
int mapWx(const String& wx);
float calcScore(int wx,int ma,int mi,int ci,int po,int rx,int rma,int rmi,int rci,int rpo);
bool initSDCard();
bool readJSONFile(const char* filename, String &output);
bool writeJSONFile(const char* filename, const String &jsonString);

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  SPI.begin();
  mfrc522.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi 已連線，IP 位址：");
  Serial.println(WiFi.localIP());

  // 初始化 SD 卡
  if (!initSDCard()) {
    Serial.println("SD 卡初始化失敗！");
  } else {
    Serial.println("SD 卡初始化成功！");
  }

  // 第一次讀取 SD 卡資料
  refreshCache();
  lastCache = millis();

  // 路由設定
  server.on("/", HTTP_GET, handleRoot);
  server.on("/startScan", HTTP_POST, handleStartScan);
  server.on("/scanStatus", HTTP_GET, handleScanStatus);
  server.on("/cards", HTTP_GET, handleGetCards);
  server.on("/cards", HTTP_POST, handleAddCard);
  server.on("/cards", HTTP_PUT, handleUpdateCard);
  server.on("/cards", HTTP_DELETE, handleDeleteCard);
  server.on("/recommend", HTTP_GET, handleRecommend);
  server.on("/similarHistory", HTTP_GET, handleSimilarHistory);
  server.begin();
}

void loop() {
  server.handleClient();

  // 定時更新快取
  if (millis() - lastCache > CACHE_INTERVAL) {
    refreshCache();
    lastCache = millis();
  }

  // 偵測掃描逾時
  if (scanning && millis() - scanStart > 5000) stopScanMode();

  // 有新卡片被刷
  if (scanning && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid;
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += mfrc522.uid.uidByte[i] < 0x10 ? "0" : "";
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    stopScanMode();
    pendingExists = cardExists(uid, pendingUID);
    pendingUID = uid;
    mfrc522.PICC_HaltA();
  }
}

// SD 卡初始化函數
bool initSDCard() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD 卡掛載失敗");
    return false;
  }
  
  // 檢查必要的檔案是否存在，若不存在則創建
  if (!SD.exists(CLOTHES_FILE)) {
    writeJSONFile(CLOTHES_FILE, "[]");
    Serial.println("創建空的衣物檔案");
  }
  
  if (!SD.exists(HISTORY_FILE)) {
    writeJSONFile(HISTORY_FILE, "[]");
    Serial.println("創建空的歷史檔案");
  }
  
  return true;
}

// 從 SD 卡讀取 JSON 檔案
bool readJSONFile(const char* filename, String &output) {
  File file = SD.open(filename, FILE_READ);
  if (!file) {
    Serial.println("無法開啟檔案進行讀取: " + String(filename));
    return false;
  }
  
  output = "";
  while (file.available()) {
    output += (char)file.read();
  }
  
  file.close();
  return true;
}

// 寫入 JSON 檔案到 SD 卡
bool writeJSONFile(const char* filename, const String &jsonString) {
  // 先刪除舊檔案（如果存在）
  if (SD.exists(filename)) {
    SD.remove(filename);
  }
  
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("無法開啟檔案進行寫入: " + String(filename));
    return false;
  }
  
  if (file.print(jsonString)) {
    Serial.println("檔案寫入成功: " + String(filename));
    file.close();
    return true;
  } else {
    Serial.println("檔案寫入失敗: " + String(filename));
    file.close();
    return false;
  }
}

void handleRoot() {
  server.send(200, "text/html", FPSTR(INDEX_HTML));
}

void handleStartScan() {
  startScanMode();
  server.send(200, "text/plain", "開始掃描，請刷卡...");
}

void handleScanStatus() {
  DynamicJsonDocument d(256);
  d["scanning"] = scanning;
  d["pendingUID"] = pendingUID;
  d["pendingExists"] = pendingExists;
  String resp;
  serializeJson(d, resp);
  server.send(200, "application/json", resp);
  pendingUID.clear(); 
  pendingExists = false;
}

void handleGetCards() {
  // 如果還沒抓到任何資料，就回傳空陣列
  String toSend = clothesCache;
  if (toSend.length() == 0) {
    toSend = "[]";
  }
  server.send(200, "application/json", toSend);
}

void handleAddCard() {
  String body = server.arg("plain");
  
  // 解析傳入的 JSON 資料
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, body);
  
  if (error) {
    server.send(400, "text/plain", "JSON 解析錯誤");
    return;
  }
  
  // 讀取現有衣物資料
  DynamicJsonDocument clothesDoc(32768);
  if (readJSONFile(CLOTHES_FILE, clothesCache)) {
    deserializeJson(clothesDoc, clothesCache);
  } else {
    // 修正：不能直接賦值 JsonArray，而是創建空的 JsonArray
    clothesDoc.clear();
    clothesDoc.to<JsonArray>();
  }
  
  // 將新衣物加入陣列
  JsonArray clothesArray = clothesDoc.as<JsonArray>();
  clothesArray.add(doc);
  
  // 將更新後的資料寫回檔案
  String updatedJson;
  serializeJson(clothesDoc, updatedJson);
  
  if (writeJSONFile(CLOTHES_FILE, updatedJson)) {
    // 更新快取
    clothesCache = updatedJson;
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(500, "text/plain", "寫入檔案失敗");
  }
}

void handleUpdateCard() {
  String body = server.arg("plain");
  String uid = server.hasArg("uid") ? server.arg("uid") : "";
  
  if (uid.isEmpty()) {
    server.send(400, "text/plain", "缺少 UID 參數");
    return;
  }
  
  // 解析傳入的 JSON 資料
  DynamicJsonDocument updateDoc(1024);
  DeserializationError error = deserializeJson(updateDoc, body);
  
  if (error) {
    server.send(400, "text/plain", "JSON 解析錯誤");
    return;
  }
  
  // 讀取現有衣物資料
  DynamicJsonDocument clothesDoc(32768);
  if (readJSONFile(CLOTHES_FILE, clothesCache)) {
    deserializeJson(clothesDoc, clothesCache);
  } else {
    server.send(500, "text/plain", "讀取衣物資料失敗");
    return;
  }
  
  // 尋找並更新指定 UID 的衣物
  bool found = false;
  JsonArray clothesArray = clothesDoc.as<JsonArray>();
  
  for (JsonVariant item : clothesArray) {
    if (item["uid"] == uid) {
      // 更新衣物資料
      for (JsonPair kv : updateDoc.as<JsonObject>()) {
        item[kv.key()] = kv.value();
      }
      found = true;
      break;
    }
  }
  
  if (!found) {
    server.send(404, "text/plain", "找不到指定 UID 的衣物");
    return;
  }
  
  // 將更新後的資料寫回檔案
  String updatedJson;
  serializeJson(clothesDoc, updatedJson);
  
  if (writeJSONFile(CLOTHES_FILE, updatedJson)) {
    // 更新快取
    clothesCache = updatedJson;
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(500, "text/plain", "寫入檔案失敗");
  }
}

void handleDeleteCard() {
  String uid = server.hasArg("uid") ? server.arg("uid") : "";
  
  if (uid.isEmpty()) {
    server.send(400, "text/plain", "缺少 UID 參數");
    return;
  }
  
  // 讀取現有衣物資料
  DynamicJsonDocument clothesDoc(32768);
  if (readJSONFile(CLOTHES_FILE, clothesCache)) {
    deserializeJson(clothesDoc, clothesCache);
  } else {
    server.send(500, "text/plain", "讀取衣物資料失敗");
    return;
  }
  
  // 尋找並刪除指定 UID 的衣物
  bool found = false;
  JsonArray clothesArray = clothesDoc.as<JsonArray>();
  
  for (size_t i = 0; i < clothesArray.size(); i++) {
    if (clothesArray[i]["uid"] == uid) {
      clothesArray.remove(i);
      found = true;
      break;
    }
  }
  
  if (!found) {
    server.send(404, "text/plain", "找不到指定 UID 的衣物");
    return;
  }
  
  // 將更新後的資料寫回檔案
  String updatedJson;
  serializeJson(clothesDoc, updatedJson);
  
  if (writeJSONFile(CLOTHES_FILE, updatedJson)) {
    // 更新快取
    clothesCache = updatedJson;
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(500, "text/plain", "寫入檔案失敗");
  }
}

void handleRecommend() {
  HTTPClient http;
  String url = String("https://opendata.cwa.gov.tw/api/v1/rest/datastore/F-C0032-001?Authorization=") 
               + CWA_API + "&locationName=高雄市";
  http.begin(url);
  int code = http.GET();
  if (code == 200) {
    DynamicJsonDocument doc(4096);
    String payload = http.getString();
    deserializeJson(doc, payload);

    JsonArray elems = doc["records"]["location"][0]["weatherElement"].as<JsonArray>();
    JsonArray wxE, maE, miE, ciE, poE;
    for (JsonVariant e : elems) {
      String n = e["elementName"].as<String>();
      if (n == "Wx")       wxE = e["time"].as<JsonArray>();
      else if (n == "MaxT") maE = e["time"].as<JsonArray>();
      else if (n == "MinT") miE = e["time"].as<JsonArray>();
      else if (n == "CI")   ciE = e["time"].as<JsonArray>();
      else if (n == "PoP")  poE = e["time"].as<JsonArray>();
    }

    curWx  = mapWx(wxE[0]["parameter"]["parameterName"].as<String>());
    curMax = maE[0]["parameter"]["parameterName"].as<int>();
    curMin = miE[0]["parameter"]["parameterName"].as<int>();
    curCI  = ciE[0]["parameter"]["parameterName"].as<String>().toInt();
    curPop = poE[0]["parameter"]["parameterName"].as<int>();

    // 計算平均溫度
    float avgTemp = (curMax + curMin) / 2.0;

    // 更新統計資訊
    String statsUpdate = "<script>document.getElementById('avgTemp').textContent = '" + String(avgTemp, 1) + "°C';</script>";

    String html = statsUpdate + "<table><tr><th>時間</th><th>天氣現象</th><th>最高溫度</th><th>最低溫度</th>"
                  "<th>舒適度</th><th>降雨機率</th></tr>";
    
    // 加入迴圈來顯示所有時段的天氣資訊
    for (int i = 0; i < wxE.size(); i++) {
      html += "<tr><td>" + String(wxE[i]["startTime"].as<const char*>())
            + " ~ " + String(wxE[i]["endTime"].as<const char*>()) + "</td>";
      html += "<td>" + wxE[i]["parameter"]["parameterName"].as<String>() + "</td>";
      html += "<td>" + String(maE[i]["parameter"]["parameterName"].as<int>()) + "°C</td>";
      html += "<td>" + String(miE[i]["parameter"]["parameterName"].as<int>()) + "°C</td>";
      html += "<td>" + ciE[i]["parameter"]["parameterName"].as<String>() + "</td>";
      html += "<td>" + String(poE[i]["parameter"]["parameterName"].as<int>()) + "%</td></tr>";
    }
    
    html += "</table>";
    server.send(200, "text/html", html);
  } else {
    server.send(code, "text/plain", "Error fetching weather");
  }
  http.end();
}

void handleSimilarHistory() {
  // 直接回傳歷史資料快取
  server.send(200, "application/json", historyCache);
}

void refreshCache() {
  // 從 SD 卡讀取衣物資料
  if (!readJSONFile(CLOTHES_FILE, clothesCache)) {
    Serial.println("讀取衣物資料失敗，使用空陣列");
    clothesCache = "[]";
  } else {
    Serial.println("成功讀取衣物資料");
  }
  
  // 從 SD 卡讀取歷史資料
  if (!readJSONFile(HISTORY_FILE, historyCache)) {
    Serial.println("讀取歷史資料失敗，使用空陣列");
    historyCache = "[]";
  } else {
    Serial.println("成功讀取歷史資料");
  }
}

void startScanMode() {
  scanning = true;
  scanStart = millis();
  digitalWrite(LED_PIN, HIGH);
}

void stopScanMode() {
  scanning = false;
  digitalWrite(LED_PIN, LOW);
}

bool cardExists(const String& uid, String& existingUID) {
  // 檢查卡片是否已存在於衣物資料中
  DynamicJsonDocument doc(32768);
  deserializeJson(doc, clothesCache);
  JsonArray array = doc.as<JsonArray>();
  
  for (JsonVariant item : array) {
    if (item["uid"] == uid) {
      existingUID = uid;
      return true;
    }
  }
  
  return false;
}

int mapWx(const String& wx) {
  if (wx == "晴") return 0;
  if (wx == "多雲") return 1;
  if (wx == "陰天" || wx == "陰時多雲") return 2;
  return 9;
}

float calcScore(int wx,int ma,int mi,int ci,int po,int rx,int rma,int rmi,int rci,int rpo) {
  const float wT = 1.0, wCI = 0.5, wPoP = 0.3, wWx = 2.0;
  return wT * fabs(ma - rma)
       + wT * fabs(mi - rmi)
       + wCI * fabs(ci - rci)
       + wPoP * fabs(po - rpo)
       + wWx * (wx != rx ? 1 : 0);
}
