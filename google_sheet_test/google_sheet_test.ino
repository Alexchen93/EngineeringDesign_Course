#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ===== 設定 =====
const char* ssid     = "C^3";
const char* password = "icecream";
// 測試用：history 或 clothes
const char* SHEETS_URL = "https://script.google.com/macros/s/AKfycbwszH0VHaHgFjo6g7aSfoE9WZpTg-YgFvKQ7Vr7-WZvy8FErlMadb7vHFgLtjuD35jOkQ/exec?sheet=history";

void setup() {
  Serial.begin(115200);
  delay(100);

  // 1. 連線 WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // 2. 發 GET 請求
  Serial.println("\n>> GET " + String(SHEETS_URL));
  HTTPClient http;
  http.begin(SHEETS_URL);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  Serial.printf("HTTP GET code: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Received payload:");
    Serial.println(payload);

    // 3. 解析 JSON
    DynamicJsonDocument doc(8192);
    auto err = deserializeJson(doc, payload);
    if (err == DeserializationError::Ok) {
      JsonArray arr = doc.as<JsonArray>();

      // 印出表頭
      JsonArray header = arr[0].as<JsonArray>();
      Serial.println("\nColumns:");
      for (JsonVariant h : header) {
        Serial.print("  - ");
        Serial.println(h.as<const char*>());
      }

      // 印出第一筆資料
      if (arr.size() > 1) {
        JsonArray firstRow = arr[1].as<JsonArray>();
        Serial.println("\nFirst row values:");
        Serial.print("Time:     "); Serial.println(firstRow[0].as<const char*>());
        Serial.print("Wx:       "); Serial.println(firstRow[1].as<const char*>());
        Serial.print("MaxT:     "); Serial.println(firstRow[2].as<int>());
        Serial.print("MinT:     "); Serial.println(firstRow[3].as<int>());
        // …你可以依序再印 CI、PoP、Top、Bottom 等
      }
    } else {
      Serial.print("JSON parse failed: ");
      Serial.println(err.c_str());
    }
  } else {
    Serial.println("Failed to get JSON.");
  }

  http.end();
  Serial.println("\n=== Test Complete ===");
}

void loop() {
  // 不做任何事情
}
