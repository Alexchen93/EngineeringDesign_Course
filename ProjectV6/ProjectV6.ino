// ProjectV5_Cleaned.ino
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include "WebPage.h"

// ====== WiFi & Web App ======
const char* ssid = "SSID";
const char* password = "Password";

#define RST_PIN   22
#define SS_PIN    21
#define LED_PIN   2
#define SD_CS     5     // SD 卡 CS

const char* CWA_API = "API";

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
void startScanMode();
void stopScanMode();
int mapWx(const String& wx);
float calcScore(int wx,int ma,int mi,int ci,int po,int rx,int rma,int rmi,int rci,int rpo);

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
  server.send(200, "application/json", content);
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
