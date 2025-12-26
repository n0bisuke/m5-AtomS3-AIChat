#include <M5Unified.h>
#include <Wire.h>
#include <M5EchoBase.h>

// AtomS3 + EchoBase pin map
static constexpr uint8_t PIN_I2S_BCLK = 8;
static constexpr uint8_t PIN_I2S_WS   = 6;
static constexpr uint8_t PIN_I2S_DOUT = 5;
static constexpr uint8_t PIN_I2S_DIN  = 7;
static constexpr uint8_t PIN_I2C_SDA  = 38;
static constexpr uint8_t PIN_I2C_SCL  = 39;

M5EchoBase echobase(I2S_NUM_0);

static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t FRAME_SAMPLES = 256;
static constexpr size_t FRAME_BYTES = FRAME_SAMPLES * 2 * 2; // stereo 16bit
static constexpr int32_t CLAP_THRESHOLD = 9600; // 80%ライン相当（12000基準）
static constexpr uint32_t DOUBLE_WINDOW_MS = 800;
static constexpr uint32_t COOLDOWN_MS = 1500;
static constexpr uint32_t HOLD_MS = 500;
static constexpr uint8_t ABOVE_FRAMES = 2; //判定用連続超えフレーム数
static constexpr uint8_t BELOW_FRAMES = 2; //判定用連続下回りフレーム数

void setup() {
    auto cfg = M5.config();
    
    M5.begin(cfg);
    Serial.begin(115200);
    randomSeed(millis());

    M5.Display.setTextSize(2);
    M5.Display.setCursor(0, 0);
    M5.Display.println("Clap Debug");
    M5.Display.println("See Serial");

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    echobase.init(SAMPLE_RATE, PIN_I2C_SDA, PIN_I2C_SCL, PIN_I2S_DIN, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_BCLK, Wire);
    echobase.setMicGain(ES8311_MIC_GAIN_6DB);
}

void loop() {
    M5.update();
    int16_t samples[FRAME_BYTES / 2];
    static uint32_t last_trigger_ms = 0;
    static uint32_t hold_until_ms = 0;
    static uint32_t flash_until_ms = 0;
    static uint16_t flash_color = 0;
    static uint32_t last_log_ms = 0;
    static uint32_t last_above_ms = 0;
    static uint32_t prev_above_ms = 0;
    static uint8_t above_count = 0;
    static uint8_t below_count = 0;
    static bool above_latched = false;
    bool above_event = false;
    const uint32_t now = millis();
    if (echobase.record(reinterpret_cast<uint8_t *>(samples), FRAME_BYTES)) {
        int32_t peak = 0;
        for (size_t i = 0; i < FRAME_SAMPLES; ++i) {
            int32_t v = samples[i * 2]; // left ch
            if (v < 0) v = -v;
            if (v > peak) peak = v;
        }

        if (now - last_log_ms >= 200) {
            last_log_ms = now;
            Serial.printf("peak,%ld\n", static_cast<long>(peak));
        }

        // ボリュームバー表示
        const int32_t bar_max = 120;
        int32_t bar = (peak * bar_max) / 12000;
        if (bar > bar_max) bar = bar_max;
        if (bar < 0) bar = 0;
        M5.Display.fillRect(0, 50, 128, 30, BLACK);
        M5.Display.drawRect(4, 60, 120, 12, DARKGREY);
        M5.Display.fillRect(4, 60, bar, 12, GREEN);
        // 閾値ライン表示
        int32_t thr = (CLAP_THRESHOLD * bar_max) / 12000;
        if (thr < 0) thr = 0;
        if (thr > bar_max) thr = bar_max;
        M5.Display.drawFastVLine(4 + thr, 60, 12, RED);
        if (flash_until_ms != 0 && now < flash_until_ms) {
            M5.Display.fillScreen(flash_color);
        } else {
            if (flash_until_ms != 0 && now >= flash_until_ms) {
                flash_until_ms = 0;
            }
            M5.Display.setCursor(0, 0);
            M5.Display.fillRect(0, 0, 128, 40, BLACK);
            if (hold_until_ms != 0 && now < hold_until_ms) {
                M5.Display.println("double_clap!");
            } else {
                if (hold_until_ms != 0 && now >= hold_until_ms) {
                    hold_until_ms = 0;
                }
                M5.Display.printf("Peak:%ld\n", static_cast<long>(peak));
            }
        }

        // 閾値を一定フレーム以上超えたら「超えイベント」扱い
        if (peak > CLAP_THRESHOLD) {
            above_count++;
            below_count = 0;
        } else {
            below_count++;
            if (below_count >= BELOW_FRAMES) {
                above_count = 0;
                above_latched = false;
            }
        }

        if (!above_latched && above_count >= ABOVE_FRAMES) {
            above_latched = true;
            prev_above_ms = last_above_ms;
            last_above_ms = now;
            above_event = true;
        }

        // 一定フレーム期間内に2回超えたらダブルクラップ（新しい超えイベント時のみ判定）
        if (above_event && last_above_ms != 0 && prev_above_ms != 0) {
            const uint32_t gap = last_above_ms - prev_above_ms;
            if (gap <= DOUBLE_WINDOW_MS && (now - last_trigger_ms) > COOLDOWN_MS) {
                last_trigger_ms = now;
                Serial.println("double_clap");
                hold_until_ms = now + HOLD_MS;
                flash_until_ms = now + HOLD_MS;
                flash_color = M5.Display.color565(random(0, 256), random(0, 256), random(0, 256));
                // 連続誤発火防止で履歴をリセット
                prev_above_ms = 0;
                last_above_ms = 0;
            }
        }
    } else {
        Serial.println("record_failed");
        delay(200);
    }
}
