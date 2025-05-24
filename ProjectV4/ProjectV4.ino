// ProjectV5.ino
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "WebPage.h"

// ====== WiFi & Web App ======
const char* ssid = "43-2N3F";
const char* password = "073817336";
const char* SHEETS_URL = "https://script.google.com/macros/s/AKfycbwszH0VHaHgFjo6g7aSfoE9WZpTg-YgFvKQ7Vr7-WZvy8FErlMadb7vHFgLtjuD35jOkQ/exec";

#define RST_PIN   22
#define SS_PIN    21
#define LED_PIN   2

MFRC522 mfrc522(SS_PIN, RST_PIN);
WebServer server(80);
const char* CWA_API = "CWA-FFADC16C-8605-4FEA-B52A-02281BA42218";

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

  // 第一次抓取遠端資料
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
  HTTPClient http;
  http.begin(SHEETS_URL);
  http.addHeader("Content-Type","application/json");
  int code = http.POST(body);
  String resp = http.getString();
  server.send(code, "text/plain", resp);
  http.end();
}

void handleUpdateCard() {
  String body = server.arg("plain");
  String uid = server.hasArg("uid") ? server.arg("uid") : "";
  
  if (uid.isEmpty()) {
    server.send(400, "text/plain", "Missing UID parameter");
    return;
  }

  HTTPClient http;
  http.begin(SHEETS_URL);
  http.addHeader("Content-Type", "application/json");
  
  // 構建更新請求
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, body);
  doc["action"] = "update";
  doc["uid"] = uid;
  
  String updatedBody;
  serializeJson(doc, updatedBody);
  Serial.println("Updating card with body: " + updatedBody);  // 除錯用
  
  int code = http.POST(updatedBody);
  String resp = http.getString();
  Serial.println("Update response: " + resp);  // 除錯用
  
  if (code == 200) {
    // 更新成功後重新獲取衣物資料
    refreshCache();
    server.send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    server.send(code, "text/plain", resp);
  }
  http.end();
}

void handleDeleteCard() {
  server.send(200, "text/plain", "Delete OK");
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
  DynamicJsonDocument doc(20000);
  if (deserializeJson(doc, historyCache) != DeserializationError::Ok) {
    server.send(500, "text/plain", "JSON 解析錯誤");
    return;
  }

  // 直接回傳原始的歷史資料，讓前端處理篩選
  server.send(200, "application/json", historyCache);
}

void refreshCache() {  //抓取資料存取history和clothes資料
  HTTPClient http;

  // ---------- 1. history 範例 ----------
  http.begin(String(SHEETS_URL) + "?sheet=history");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (http.GET() == HTTP_CODE_OK) {
    String raw = http.getString();
    Serial.println("🔄 history raw: " + raw);

    DynamicJsonDocument dHist(32768);
    if (deserializeJson(dHist, raw) == DeserializationError::Ok) {
      // 頂層即是陣列
      JsonArray arr = dHist.as<JsonArray>();
      historyCache = "";
      serializeJson(arr, historyCache);
      Serial.println("✅ historyCache parsed");
    } else {
      Serial.println("❌ history JSON parse error");
    }
  } else {
    Serial.printf("❌ history GET failed, code=%d\n", http.GET());
  }
  http.end();

  // ---------- 2. clothes ----------
  http.begin(String(SHEETS_URL) + "?sheet=clothes");
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (http.GET() == HTTP_CODE_OK) {
    String raw = http.getString();
    Serial.println("🔄 clothes raw: " + raw);

    DynamicJsonDocument dClothes(32768), outDoc(32768);
    if (deserializeJson(dClothes, raw) == DeserializationError::Ok) {
      JsonArray arr = dClothes.as<JsonArray>();     // <- 直接當 Array 解析
      JsonArray outArr = outDoc.to<JsonArray>();

      // 跳過 header 從 i=1 開始
      for (size_t i = 1; i < arr.size(); i++) {
        JsonArray row = arr[i].as<JsonArray>();
        if (row.size() >= 6) {
          JsonObject obj = outArr.createNestedObject();
          obj["cloth"] = row[0].as<const char*>();    // 衣服名稱
          obj["model"] = row[1].as<const char*>();     // 款式
          obj["uid"]   = row[5].as<const char*>();    // UUID
          JsonObject feat = obj.createNestedObject("features");
          feat["style"]    = row[3].as<const char*>(); // 樣式 (第 3 欄)
          feat["color"]    = row[2].as<const char*>(); // 顏色 (第 2 欄)
          feat["position"] = row[4].as<const char*>(); // 位置 (第 4 欄)
        } else {
          Serial.printf("⚠️ clothes row %d invalid, size=%d\n", i, row.size());
        }
      }

      clothesCache = "";
      serializeJson(outDoc, clothesCache);
      Serial.println("✅ clothesCache: " + clothesCache);
    } else {
      Serial.println("❌ clothes JSON parse error");
    }
  } else {
    Serial.printf("❌ clothes GET failed, code=%d\n", http.GET());
  }
  http.end();
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

bool cardExists(const String&, String&) {
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
