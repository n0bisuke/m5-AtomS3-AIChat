#include <M5Unified.h>
#include <Wire.h>
#include <M5EchoBase.h>
#include <esp_heap_caps.h>

// --- AtomS3 + EchoBase Pin Mapping ---
// AtomS3はESP32-S3ベースのため、従来のATOM Matrix/LiteとGPIO番号が異なります。
// EchoBaseの機能に割り当てられたピンをS3用にマッピングします。
static constexpr const uint8_t PIN_I2S_BCLK = 8;  // Bit Clock
static constexpr const uint8_t PIN_I2S_WS   = 6;  // Word Select (LRCK)
static constexpr const uint8_t PIN_I2S_DOUT = 5;  // Data Out (Speaker)
static constexpr const uint8_t PIN_I2S_DIN  = 7;  // Data In (Mic)
static constexpr const uint8_t PIN_I2C_SDA  = 38;
static constexpr const uint8_t PIN_I2C_SCL  = 39;

M5EchoBase echobase(I2S_NUM_0);

static constexpr uint32_t SAMPLE_RATE = 8000;
static constexpr uint32_t RECORD_SECONDS = 8;
// 16kHz * 16bit * stereo(2ch) * seconds
static constexpr size_t RECORD_SIZE = SAMPLE_RATE * 2 * 2 * RECORD_SECONDS;
static uint8_t *record_buffer = nullptr;

// I2Cスキャンは動作確認用なので、ミニマム版では無効化
// static void scan_i2c() {
//     M5.Display.println("I2C scan...");
//     uint8_t found = 0;
//     for (uint8_t addr = 1; addr < 0x7F; ++addr) {
//         Wire.beginTransmission(addr);
//         if (Wire.endTransmission() == 0) {
//             ++found;
//             M5.Display.printf("0x%02X\n", addr);
//             Serial.printf("I2C: 0x%02X\n", addr);
//         }
//     }
//     if (!found) {
//         M5.Display.println("No I2C dev");
//         Serial.println("No I2C devices found");
//     }
// }

void setup() {
    auto cfg = M5.config();
    
    // AtomS3の内部スピーカーを使わないように設定（EchoBaseを使うため）
    // ※M5Unifiedのバージョンによっては、外部I2S設定時に自動無効化されますが念のため。
    cfg.internal_spk = false; 
    cfg.internal_mic = false;

    M5.begin(cfg);

    // EchoBaseのオーディオCODEC(ES8311)初期化用I2C
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    // EchoBaseのES8311を初期化（公式サンプル準拠）
    echobase.init(SAMPLE_RATE, PIN_I2C_SDA, PIN_I2C_SCL, PIN_I2S_DIN, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_BCLK, Wire);
    echobase.setSpeakerVolume(70);

    // M5UnifiedのSpeaker/Micは使わず、EchoBaseのI2Sドライバのみを使う

    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Press Btn");
    M5.Display.println("Rec 8s @8k");

    record_buffer = static_cast<uint8_t *>(heap_caps_malloc(RECORD_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (record_buffer == nullptr) {
        record_buffer = static_cast<uint8_t *>(malloc(RECORD_SIZE));
    }
    if (record_buffer == nullptr) {
        M5.Display.println("No Mem");
        while (true) {
            delay(1000);
        }
    }
}

void loop() {
    M5.update();

    static bool busy = false;
    if (busy) {
        return;
    }

    // ボタン押下で「3秒録音 → 再生」を1回実行
    if (M5.BtnA.wasPressed()) {
        busy = true;
        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.printf("Recording %us\n", RECORD_SECONDS);

        echobase.setMute(true);
        delay(10);
        echobase.record(record_buffer, RECORD_SIZE);
        delay(100);

        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Playing...");

        echobase.setMute(false);
        delay(10);
        echobase.play(record_buffer, RECORD_SIZE);
        delay(100);

        M5.Display.fillScreen(BLACK);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Press Btn");
        M5.Display.println("Rec 8s @8k");
        busy = false;
    }
}
