// ProjectV5_Cleaned.ino
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include "WebPage.h"
#include "DataJson.h"

// ====== WiFi & Web App ======
const char* ssid = "ite-441";
const char* password = "ite12345";

#define RST_PIN   22
#define SS_PIN    21
#define LED_PIN   2
// #define SD_CS     5     // SD 卡 CS  // 已不再使用SD卡，註解掉

const char* CWA_API = "CWA-FFADC16C-8605-4FEA-B52A-02281BA42218";

MFRC522 mfrc522(SS_PIN, RST_PIN);
WebServer server(80);

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
void handleDeleteCard();
void handleUpdateCard();
void startScanMode();
void stopScanMode();
int mapWx(const String& wx);
float calcScore(int wx,int ma,int mi,int ci,int po,int rx,int rma,int rmi,int rci,int rpo);

void writeFileIfEmpty(const char* path, const char* content) {
  if (!SPIFFS.exists(path)) {
    File f = SPIFFS.open(path, FILE_WRITE);
    if (f) { f.print(content); f.close(); Serial.printf("%s 初始化為預設資料\n", path); }
  } else {
    File f = SPIFFS.open(path, FILE_READ);
    if (f) {
      size_t sz = f.size();
      String c = "";
      while (f.available()) c += char(f.read());
      f.close();
      if (sz == 0 || c.length() == 0 || c == "[]") {
        File fw = SPIFFS.open(path, FILE_WRITE);
        if (fw) { fw.print(content); fw.close(); Serial.printf("%s 內容為空，自動初始化為預設資料\n", path); }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  SPI.begin();
  mfrc522.PCD_Init();

  // 初始化 SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS 掛載失敗");
  } else {
    Serial.println("SPIFFS 初始化成功");
    writeFileIfEmpty("/clothes.json", CLOTHES_JSON);
    writeFileIfEmpty("/history.json", HISTORY_JSON);
  }

  // 檢查 SPIFFS 是否有 clothes.json 和 history.json，並輸出內容
  if (SPIFFS.exists("/clothes.json")) {
    Serial.println("[檢查] clothes.json 已存在於 SPIFFS");
    File f = SPIFFS.open("/clothes.json", FILE_READ);
    if (f) {
      Serial.print("[內容] clothes.json: ");
      while (f.available()) Serial.write(f.read());
      Serial.println();
      f.close();
    }
  } else {
    Serial.println("[檢查] clothes.json 不存在於 SPIFFS");
  }
  if (SPIFFS.exists("/history.json")) {
    Serial.println("[檢查] history.json 已存在於 SPIFFS");
    File f = SPIFFS.open("/history.json", FILE_READ);
    if (f) {
      Serial.print("[內容] history.json: ");
      while (f.available()) Serial.write(f.read());
      Serial.println();
      f.close();
    }
  } else {
    Serial.println("[檢查] history.json 不存在於 SPIFFS");
  }

  // WiFi 連線
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi 已連線，IP 位址：");
  Serial.println(WiFi.localIP());

  // 路由設定
  server.on("/", HTTP_GET, handleRoot);
  server.on("/startScan", HTTP_POST, handleStartScan);
  server.on("/scanStatus", HTTP_GET, handleScanStatus);
  server.on("/cards", HTTP_GET, handleGetCards);
  server.on("/cards", HTTP_POST, handleAddCard); // SPIFFS write
  server.on("/cards", HTTP_DELETE, handleDeleteCard);
  server.on("/cards", HTTP_PUT, handleUpdateCard); // SPIFFS update
  server.on("/recommend", HTTP_GET, handleRecommend);
  server.on("/similarHistory", HTTP_GET, handleSimilarHistory);
  server.begin();
}

void loop() {
  server.handleClient();

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
    // TODO: implement SD-based lookup
    pendingExists = false;
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
  Serial.println("嘗試開啟 clothes.json");
  File file = SPIFFS.open("/clothes.json");
  if (!file) {
    Serial.println("clothes.json 開啟失敗");
    server.send(500, "application/json", "[]");
    return;
  }
  Serial.println("clothes.json 開啟成功");
  String content;
  while (file.available()) content += char(file.read());
  file.close();
  Serial.println("檔案內容：" + content);

  // 將每筆資料 key 轉換為前端需要的格式
  DynamicJsonDocument srcDoc(8192);
  DynamicJsonDocument outDoc(8192);
  DeserializationError err = deserializeJson(srcDoc, content);
  if (err) {
    server.send(500, "application/json", "[]");
    return;
  }
  JsonArray srcArr = srcDoc.as<JsonArray>();
  JsonArray outArr = outDoc.to<JsonArray>();
  for (JsonObject obj : srcArr) {
    JsonObject o = outArr.createNestedObject();
    o["cloth"] = obj["衣服"];
    o["model"] = obj["款式"];
    JsonObject features = o.createNestedObject("features");
    features["color"] = obj["顏色"];
    features["style"] = obj["樣式"];
    features["position"] = obj["位置"];
    o["uid"] = obj["UUID"];
  }
  String outJson;
  serializeJson(outDoc, outJson);
  server.send(200, "application/json", outJson);
}

void handleAddCard() {
  // 讀取現有 clothes.json
  File file = SPIFFS.open("/clothes.json", FILE_READ);
  DynamicJsonDocument doc(4096);
  if (file) {
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
      Serial.println("JSON 解析失敗，建立新陣列");
      doc.to<JsonArray>();
    }
  } else {
    doc.to<JsonArray>();
  }

  // 取得前端傳來的 JSON
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "缺少 JSON 資料");
    return;
  }

  DynamicJsonDocument input(512);
  DeserializationError err = deserializeJson(input, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "JSON 格式錯誤");
    return;
  }

  // 新增到陣列
  JsonArray arr = doc.as<JsonArray>();
  arr.add(input);

  // 寫回 SPIFFS
  file = SPIFFS.open("/clothes.json", FILE_WRITE);
  if (!file) {
    server.send(500, "text/plain", "無法寫入 clothes.json");
    return;
  }
  serializeJson(doc, file);
  file.close();
  server.send(200, "text/plain", "新增成功");
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

    float avgTemp = (curMax + curMin) / 2.0;
    String statsUpdate = "<script>document.getElementById('avgTemp').textContent = '" + String(avgTemp, 1) + "°C';</script>";

    String html = statsUpdate + "<table><tr><th>時間</th><th>天氣現象</th><th>最高溫度</th><th>最低溫度</th>"
                  "<th>舒適度</th><th>降雨機率</th></tr>";
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
  File file = SPIFFS.open("/history.json");
  if (!file) {
    server.send(500, "application/json", "[]");
    return;
  }
  String content;
  while (file.available()) content += char(file.read());
  file.close();
  server.send(200, "application/json", content);
}

void handleDeleteCard() {
  // TODO: 實作從 SD 卡刪除指定卡片資料
  server.send(200, "text/plain", "Delete OK");
}

void handleUpdateCard() {
  // 讀取現有 clothes.json
  File file = SPIFFS.open("/clothes.json", FILE_READ);
  DynamicJsonDocument doc(4096);
  if (file) {
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
      server.send(500, "text/plain", "無法解析現有的 JSON 資料");
      return;
    }
  } else {
    server.send(500, "text/plain", "無法開啟 clothes.json");
    return;
  }

  // 取得前端傳來的 JSON
  DynamicJsonDocument input(512);
  DeserializationError err = deserializeJson(input, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "JSON 格式錯誤");
    return;
  }

  // 新增更多除錯訊息
  Serial.println("[DEBUG] 嘗試更新資料");
  Serial.println("[DEBUG] 接收到的 JSON 資料：");
  serializeJson(input, Serial);
  Serial.println();

  // 檢查 clothes.json 的結構
  if (!doc.is<JsonArray>()) {
    Serial.println("[ERROR] clothes.json 的結構不是 JSON 陣列");
    server.send(500, "text/plain", "clothes.json 的結構錯誤");
    return;
  }

  // 嘗試更新指定的項目
  JsonArray arr = doc.as<JsonArray>();
  bool updated = false;
  for (JsonObject obj : arr) {
    Serial.print("[DEBUG] 檢查 UUID: ");
    Serial.println(obj["UUID"].as<String>());
    if (obj["UUID"] == input["UUID"]) {
      Serial.println("[DEBUG] 找到匹配的 UUID，開始更新...");
      for (JsonPair kv : input.as<JsonObject>()) {
        obj[kv.key()] = kv.value();
      }
      updated = true;
      break;
    }
  }

  if (!updated) {
    Serial.println("[ERROR] 找不到匹配的 UUID");
    server.send(404, "text/plain", "找不到指定的項目");
    return;
  }

  // 確認更新後的 JSON 結構
  Serial.println("[DEBUG] 更新後的 JSON 資料：");
  serializeJson(doc, Serial);
  Serial.println();

  // 寫回 SPIFFS
  file = SPIFFS.open("/clothes.json", FILE_WRITE);
  if (!file) {
    server.send(500, "text/plain", "無法寫入 clothes.json");
    return;
  }
  serializeJson(doc, file);
  file.close();
  server.send(200, "text/plain", "更新成功");
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
