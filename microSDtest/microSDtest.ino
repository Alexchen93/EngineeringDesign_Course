#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS   5    // SD 卡 CS 腳位，可依實際接線修改
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

// 使用內建 VSPI
SPIClass sdSPI = SPIClass(VSPI);

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  File root = fs.open(dirname);
  if(!root){
    Serial.println("打開目錄失敗");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("這不是目錄");
    return;
  }
  File file = root.openNextFile();
  while(file){
    for(uint8_t i=0;i<levels;i++) Serial.print("  ");
    Serial.print("- ");
    Serial.print(file.name());
    if(file.isDirectory()){
      Serial.println("/");
      listDir(fs, file.name(), levels+1);
    } else {
      Serial.print("\t  ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file = root.openNextFile();
  }
}

bool testReadWrite(fs::FS &fs){
  const char * testDir = "/test_dir";
  const char * testFile = "/test_dir/test.txt";
  // 建立目錄
  if(!fs.exists(testDir)){
    if(!fs.mkdir(testDir)){
      Serial.println("建立測試目錄失敗");
      return false;
    }
  }
  // 寫入檔案
  File f = fs.open(testFile, FILE_WRITE);
  if(!f){
    Serial.println("打開測試檔案寫入失敗");
    return false;
  }
  f.println("microSD 卡測試內容");
  f.close();
  Serial.println("寫入測試檔案成功");

  // 讀取檔案
  f = fs.open(testFile);
  if(!f){
    Serial.println("打開測試檔案讀取失敗");
    return false;
  }
  Serial.println("讀取測試檔案內容：");
  while(f.available()){
    Serial.write(f.read());
  }
  Serial.println();
  f.close();

  // 刪除檔案
  if(fs.remove(testFile)){
    Serial.println("刪除測試檔案成功");
  } else {
    Serial.println("刪除測試檔案失敗");
  }
  return true;
}

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("===== microSD 卡診斷程式 =====");

  // 初始化 SPI
  sdSPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);

  Serial.println("嘗試初始化 SD 卡...");
  if(!SD.begin(SD_CS, sdSPI, 4000000)){
    Serial.println(">> SD 卡初始化失敗，請檢查接線、供電、格式");
    while(true){ delay(1000); }
  }
  Serial.println(">> SD 卡初始化成功");

  // 卡片類型
  uint8_t cardType = SD.cardType();
  Serial.print("卡片類型：");
  switch(cardType){
    case CARD_NONE: Serial.println("無卡片"); break;
    case CARD_MMC:  Serial.println("MMC"); break;
    case CARD_SD:   Serial.println("SDSC"); break;
    case CARD_SDHC: Serial.println("SDHC/SDXC"); break;
    default:        Serial.println("未知"); break;
  }

  // 卡片容量
  uint64_t size = SD.cardSize() / (1024 * 1024);
  Serial.printf("卡片容量：%llu MB\n", size);

  // 列出根目錄
  Serial.println("\n列出根目錄檔案：");
  listDir(SD, "/", 0);

  // 測試檔案讀寫與刪除
  Serial.println("\n進行讀寫測試：");
  if(testReadWrite(SD)){
    Serial.println(">> 讀寫測試完成");
  } else {
    Serial.println(">> 讀寫測試失敗");
  }

  Serial.println("\n=== 測試結束 ===");
}

void loop(){
  // 空迴圈，所有動作都在 setup() 中完成
}
