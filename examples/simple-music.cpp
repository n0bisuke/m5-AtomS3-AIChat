#include <M5Unified.h>
#include <Wire.h>
#include <M5EchoBase.h>

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
    echobase.init(16000, PIN_I2C_SDA, PIN_I2C_SCL, PIN_I2S_DIN, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_BCLK, Wire);
    echobase.setSpeakerVolume(70);

    // --- Speaker (EchoBase NS4168) Config ---
    {
        auto spk_cfg = M5.Speaker.config();
        spk_cfg.pin_bck      = PIN_I2S_BCLK;
        spk_cfg.pin_ws       = PIN_I2S_WS;
        spk_cfg.pin_data_out = PIN_I2S_DOUT;
        spk_cfg.i2s_port     = I2S_NUM_0; // S3はI2S_NUM_0推奨
        spk_cfg.sample_rate  = 16000;     // サンプリングレート
        M5.Speaker.config(spk_cfg);
    }

    // スピーカーの初期化開始（マイクは使わない）
    M5.Speaker.begin();

    // 音量設定 (0-255) ハウリング注意！
    M5.Speaker.setVolume(128); 

    // LCD表示は動作確認用なので、ミニマム版では無効化
    // M5.Display.setTextSize(2);
    // M5.Display.println("Speaker Test");
    // M5.Display.println("Hold Btn to Tone");
    // scan_i2c();
}

void playMusic() {
    // FF Victory Fanfare (Cメジャー基準、クラシック版)
        static const uint16_t melody[] = {
            523, 523, 523, 523,         // C5 C5 C5 C5 (ドドドド)
            415, 466, 523,              // G#4 A#4 C5
            466, 523,                   // A#4 C5
            392, 349, 392, 349,         // G4 F4 G4 F4
            466, 466, 440, 466, 440,    // A#4 A#4 A4 A#4 A4
            392, 349, 330, 349, 311,    // G4 F4 E4 F4 D#4
            392, 349, 392, 349,         // G4 F4 G4 F4
            466, 466, 440, 466, 440,    // A#4 A#4 A4 A#4 A4
            392, 349, 392,              // G4 F4 G4
            466, 523                    // A#4 C5 (最後盛り上げ)
        };
        static const uint16_t note_ms[] = {
            150, 150, 150, 150,         // 速い4音
            150, 150, 300,              // 
            150, 400,                   // 
            150, 150, 150, 150,         // 
            150, 150, 150, 150, 150,    // 
            150, 150, 150, 150, 300,    // 
            150, 150, 150, 150,         // 
            150, 150, 150, 150, 150,    // 
            150, 150, 300,              // 
            150, 600                    // 最後長め
        };
        const size_t count = sizeof(melody) / sizeof(melody[0]);
        for (size_t i = 0; i < count; ++i) {
            M5.Speaker.tone(melody[i], note_ms[i]);
            delay(note_ms[i] + 20);  // 少し間を空けて自然に
        }
}

void loop() {
    M5.update();

    if (M5.BtnA.wasPressed()) {
        playMusic();
        M5.Speaker.stop();
    }
}
