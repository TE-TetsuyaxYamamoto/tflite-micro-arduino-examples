// Arduino Nano 33 BLE Sense Rev2 - iBeacon 送信スケッチ
// ライブラリ: ArduinoBLE

#include <ArduinoBLE.h>

// iBeacon 設定
const uint8_t BEACON_UUID[16] = {
  0x25, 0x11, 0x14, 0x44, 0xB4, 0x51, 0x47, 0xA4,
  0x9F, 0xC5, 0xD7, 0x4C, 0x42, 0x3F, 0xE1, 0xB1
};
const uint16_t BEACON_MAJOR = 1;
const uint16_t BEACON_MINOR = 1;
const int8_t   MEASURED_POWER = -59;  // 1m 地点での RSSI (dBm)

void setup() {
  Serial.begin(115200);
  // USB シリアル待ち (最大 10 秒)
  unsigned long start = millis();
  while (!Serial && millis() - start < 10000) {}

  Serial.println("Starting iBeacon...");
  if (!BLE.begin()) {
    Serial.println("BLE の初期化に失敗しました");
    while (1) {}
  }

  // Apple company ID (0x004C) + iBeacon プレフィックス(0x02 0x15) + データ
  uint8_t mfrData[25];
  mfrData[0] = 0x4C;  // company ID (little endian)
  mfrData[1] = 0x00;
  mfrData[2] = 0x02;  // iBeacon type
  mfrData[3] = 0x15;  // データ長 (21)
  memcpy(&mfrData[4], BEACON_UUID, 16);
  mfrData[20] = BEACON_MAJOR >> 8;
  mfrData[21] = BEACON_MAJOR & 0xFF;
  mfrData[22] = BEACON_MINOR >> 8;
  mfrData[23] = BEACON_MINOR & 0xFF;
  mfrData[24] = (uint8_t)MEASURED_POWER;

  BLE.setManufacturerData(mfrData, sizeof(mfrData));
  BLE.setConnectable(false);
  BLE.setAdvertisingInterval(160);  // 160 * 0.625ms = 100ms

  BLE.advertise();
  Serial.println("iBeacon 送信中...");
  Serial.print("BLE MAC アドレス: ");
  Serial.println(BLE.address());
}

void loop() {
  BLE.poll();

  // シリアルモニターを後から開いても確認できるよう定期出力
  static unsigned long lastLog = 0;
  if (millis() - lastLog >= 5000) {
    lastLog = millis();
    Serial.print("iBeacon 送信中 MAC: ");
    Serial.println(BLE.address());
  }
}
