# ProjectV6: 智慧衣櫃管理系統 (Smart Wardrobe Management System)

## 專案概述
ProjectV6 是一套基於 ESP32-CAM 與 RC522 RFID 的智慧衣櫃管理系統。使用者透過刷卡即可登錄衣物，並在網頁介面中進行衣物瀏覽、新增、編輯、刪除，此外還整合了天氣資訊與穿搭建議，以及歷史穿搭分析功能。



---

## 硬體需求

- **ESP32-CAM** 開發板  
- **RC522 RFID** 讀卡模組  
- **MicroSD** 卡（選用，可儲存更多資料）  
- **5V** 電源或 USB 供電  
- **外接 LED**（選用，用於狀態指示）

### 接線說明

| 模組             | 模組腳位 | ESP32-CAM 腳位  |
|------------------|----------|-----------------|
| **ESP32-CAM 電源** | 5V       | 外部 5V         |
|                  | GND      | GND             |
| **RC522**        | VCC      | 3.3V            |
|                  | RST      | GPIO22          |
|                  | SDA/SS   | GPIO21          |
|                  | SCK      | GPIO18          |
|                  | MOSI     | GPIO23          |
|                  | MISO     | GPIO19          |
|                  | GND      | GND             |
| **SD 卡模組**    | CS       | GPIO5           |
|                  | SCK      | GPIO18          |
|                  | MOSI     | GPIO23          |
|                  | MISO     | GPIO19          |
|                  | VCC      | 3.3V            |
|                  | GND      | GND             |
| **狀態 LED**     | +        | GPIO2 (透過 220Ω) |
|                  | –        | GND             |

> *註：SD 卡與 RC522 共用同一 SPI 線（SCK/MOSI/MISO），僅各自獨立 CS 腳位。*

---

## 軟體需求
- Arduino IDE (版本 ≥ 1.8.13)  
- ESP32 開發板套件  
- Arduino 函式庫：
  - WiFi.h / WebServer.h  
  - SPI.h / MFRC522.h  
  - ArduinoJson.h  
  - （若使用 SPIFFS/SD，請加裝對應庫）
