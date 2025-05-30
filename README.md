# Smart Wardrobe Management System (智慧衣櫃管理系統)

## 目錄

1. [專案概述](#專案概述)
2. [功能特色](#功能特色)
3. [檔案結構](#檔案結構)
4. [硬體需求](#硬體需求)
5. [軟體需求](#軟體需求)
6. [安裝與使用](#安裝與使用)
7. [程式碼說明](#程式碼說明)

   * [RFIDtest](#1-rfidtest-讀卡測試)
   * [microSDtest](#2-microsdtest-儲存測試)
   * [RFID\_SD\_test](#3-rfid_sd_test-結合讀卡與儲存)
   * [V7 (ProjectV6/V7)](#4-v7-web-介面版本)
   * [APIRead.py (天氣與穿搭建議)](#5-apireadpy-天氣資料讀取)
8. [後續擴充](#後續擴充)
---

## 專案概述

本專案以 ESP32-CAM 開發板與 RC522 RFID 模組為基礎，打造一套智慧衣櫃管理系統。使用者可：

* 刷 RFID 標籤登錄衣物
* 透過網頁介面檢視、新增、編輯、刪除衣物資訊
* 整合即時天氣並給出穿搭建議
* 追蹤歷史穿搭紀錄並輸出報告

---

## 功能特色

* **RFID 自動識別**：快速登錄衣物與個人資訊對應
* **MicroSD/FLASH 資料存取**：將衣物清單、穿搭紀錄存檔
* **WebServer 介面**：手機/電腦皆可透過 Wi-Fi 操作衣櫃管理
* **即時天氣 API**：自動抓取天氣資料並提供穿搭建議
* **歷史分析**：統計最常穿著的衣物，生成簡易分析報表

---

## 檔案結構

```
EngineeringDesign_Course/
├─ .gitattributes
├─ README.md             # 本檔案
├─ APIRead.py            # Python 天氣 API 讀取腳本
├─ RFIDtest/             # 範例 1：單純 RFID 讀卡測試
│  └─ RFIDtest.ino
├─ microSDtest/          # 範例 2：MicroSD 卡讀寫測試
│  └─ microSDtest.ino
├─ RFID_SD_test/         # 範例 3：RFID + SD 卡結合測試
│  └─ RFID_SD_test.ino
└─ V7/                   # 最終整合版 (含 Web 介面與 JSON 資料)
   ├─ ProjectV6_modified_fixed.ino
   ├─ WebPage.h          # HTML/CSS/JS 模板
   └─ DataModel.h        # (若有) 資料結構定義
```

---

## 硬體需求

* ESP32-CAM 開發板
* RC522 RFID 讀卡模組
* MicroSD 卡模組（可選）
* 5V 電源或 USB 供電
* (選用) LED + 220Ω 電阻 作為狀態指示燈

### 接線對照

| 模組               | 接腳     | ESP32-CAM 腳位 |
| ---------------- | ------ | ------------ |
| RC522 VCC        | 3.3V   | 3.3V         |
| RC522 RST        | RST    | GPIO22       |
| RC522 SDA        | SS     | GPIO21       |
| RC522 SCK        | SCK    | GPIO18       |
| RC522 MOSI       | MOSI   | GPIO23       |
| RC522 MISO       | MISO   | GPIO19       |
| SD CS            | CS     | GPIO5        |
| SD SCK/MOSI/MISO | 共用 SPI | 同上           |
| LED +            | 正極     | GPIO2 (220Ω) |
| LED –            | 負極     | GND          |

> **註**：SD 卡與 RC522 共用 SPI Bus，只需在程式中切換 CS 腳位。

---

## 軟體需求

* Arduino IDE ≥ 1.8.13

* ESP32 開發板套件

* 安裝 Arduino 函式庫：

  * **WiFi**, **WebServer**
  * **SPI**, **MFRC522**
  * **ArduinoJson**
  * **SD** 或 **SPIFFS**（視儲存方式而定）

* Python 3.x（若要執行 `APIRead.py`）

  * 安裝套件：`requests`、`python-dotenv` (存放 API Key)

---

## 安裝與使用

1. **下載專案**

   ```bash
   git clone https://github.com/Alexchen93/EngineeringDesign_Course.git
   cd EngineeringDesign_Course
   ```
2. **開啟範例程式**

   * RFID測試：`RFIDtest/RFIDtest.ino`
   * SD卡測試：`microSDtest/microSDtest.ino`
   * RFID+SD 結合：`RFID_SD_test/RFID_SD_test.ino`
   * 最終版 Web 介面：`V7/ProjectV6_modified_fixed.ino`
3. **設定 Wi-Fi SSID/密碼**
   修改 `ProjectV6_modified_fixed.ino` 內的 `ssid`、`password` 參數。
4. **上傳程式至 ESP32-CAM**
5. **在瀏覽器開啟**

   * 連線至 ESP32-CAM AP 或同網段路由
   * 輸入裝置 IP（序列監控列印或路由器介面查詢）
6. **操作介面**

   * 刷卡登錄／選擇衣物
   * 新增、編輯、刪除條目
   * 查看天氣與穿搭建議
   * 匯出歷史報表

---

## 程式碼說明

### 1. RFIDtest (讀卡測試)

* **目的**：示範如何使用 MFRC522 函式庫讀取 RFID 卡片 UID。
* **核心流程**：

  1. 初始化 SPI 與 MFRC522
  2. 持續檢測卡片
  3. 顯示 UID 串列
  4. 加入簡易過濾避免重複讀取

### 2. microSDtest (儲存測試)

* **目的**：示範如何使用 SD 函式庫在 MicroSD 卡上建立、寫入與讀取文字檔。
* **核心流程**：

  1. 掛載 SD 卡
  2. 建立 `test.txt` 並寫入範例文字
  3. 讀取並在序列埠輸出內容

### 3. RFID\_SD\_test (結合讀卡與儲存)

* **目的**：整合 RFID 讀卡與 SD 卡存檔功能，將每次刷卡事件以 CSV 格式記錄。
* **特色**：

  * 每次刷卡紀錄 → `records.csv`
  * 包含 UID、時間戳、（可擴充）使用者名稱

### 4. V7 (Web 介面版本)

* **目的**：最終整合版，提供完整 Web UI，並使用 ArduinoJson 管理資料。
* **主要檔案**：

  * `ProjectV6_modified_fixed.ino`：

    * WebServer 路由定義（`/list`, `/add`, `/edit`, `/delete`）
    * JSON 讀寫到 SD 或 SPIFFS
  * `WebPage.h`：

    * HTML/CSS/JavaScript 樣板
    * AJAX 呼叫各路由更新畫面
  * `DataModel.h`（若有）：定義 ClothingItem 結構與 JSON 序列化
* **使用方式**：在瀏覽器即可對衣櫃進行 CRUD 操作，並同時抓取天氣 API 供前端顯示。

### 5. APIRead.py (天氣資料讀取)

* **目的**：在本機端或伺服器上執行，抓取 OpenWeatherMap API 的當日天氣，並產生簡易穿搭建議清單。
* **主要步驟**：

  1. 從 `.env` 讀取 `API_KEY` 與 `LOCATION`
  2. 呼叫 `https://api.openweathermap.org/data/2.5/weather`
  3. 解析溫度、天氣描述
  4. 根據溫度區間輸出建議，如「建議穿長袖」或「可以穿短袖」
* **執行方式**：

  ```bash
  pip install requests python-dotenv
  python APIRead.py
  ```

---

## 後續擴充

* **使用者認證**：整合簡易帳號密碼保護 Web 介面
* **資料庫**：將 SD 卡資料遷移到 SQLite 或 Firebase
* **行動 App**：開發 iOS/Android App，透過 RESTful API 遠端控制
* **穿搭演算法**：AI 分析使用者歷史穿搭，提供個性化推薦


---

> 若有任何問題或建議，歡迎透過 GitHub 提出 Issue 或 Pull Request。
