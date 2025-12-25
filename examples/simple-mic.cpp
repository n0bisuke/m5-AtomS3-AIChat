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

    // --- Mic (EchoBase SPM1423) Config ---
    {
        auto mic_cfg = M5.Mic.config();
        mic_cfg.pin_bck     = PIN_I2S_BCLK;
        mic_cfg.pin_ws      = PIN_I2S_WS;
        mic_cfg.pin_data_in = PIN_I2S_DIN;
        mic_cfg.i2s_port    = I2S_NUM_0;
        mic_cfg.stereo      = false;
        mic_cfg.use_adc     = false;
        M5.Mic.config(mic_cfg);
    }
    M5.Mic.begin();

    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Hold Btn");
    M5.Display.println("Mic Test");
}

void loop() {
    M5.update();

    // ボタンを押している間だけマイクレベルを表示
    if (M5.BtnA.isHolding() || M5.BtnA.isPressed()) {
        int16_t sound_data[256];
        if (M5.Mic.record(sound_data, 256, 16000)) {
            int32_t peak = 0;
            for (int i = 0; i < 256; ++i) {
                int32_t v = sound_data[i];
                if (v < 0) v = -v;
                if (v > peak) peak = v;
            }
            // 0..32767 を 0..60 にスケール
            int32_t radius = (peak * 60) / 32767;
            M5.Display.fillScreen(BLACK);
            M5.Display.drawCircle(64, 64, 60, DARKGREY);
            M5.Display.fillCircle(64, 64, radius, GREEN);
        }
    } else {
        M5.Display.fillScreen(BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(0, 0);
        M5.Display.println("Hold Btn");
        M5.Display.println("Mic Test");
    }
}