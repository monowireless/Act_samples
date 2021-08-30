#include "dioint.h"
#include "dioint_bhv.hpp"

int iCounter = 0;

/*** the setup procedure (called on boot) */
void setup() {
    // BRD_APPTWELITE は twe_twelite.board には登録しない。
    // ※ピン番号のエリアスだけ使う。
    // auto&& brd = the_twelite.board.use<BRD_APPTWELITE>();

    // ビヘイビア<MyApp>の登録
    the_twelite.app.use<MyApp>();
    
    // 出力設定
    pinMode(BRD_APPTWELITE::PIN_DI1, OUTPUT);
    digitalWrite(BRD_APPTWELITE::PIN_DI1, LOW); // TURN DO1 ON

    // 入力設定
    pinMode(BRD_APPTWELITE::PIN_DI1, PIN_MODE::INPUT_PULLUP);
    pinMode(BRD_APPTWELITE::PIN_DI2, PIN_MODE::INPUT_PULLUP);

    // 割り込みの設定
    attachIntDio(BRD_APPTWELITE::PIN_DI1, PIN_INT_MODE::FALLING);
    attachIntDio(BRD_APPTWELITE::PIN_DI2, PIN_INT_MODE::RISING);

    // CPU クロックの設定 (32Mhz)
    the_twelite << TWENET::cpu_base_clock(3);

    Serial << "--- test sample ---" << crlf;
}

// 初回 loop() 直前に1回だけ呼ばれる
void begin() {    
}

/*** loop procedure (called every event) */
void loop() {
    // １秒おきに . を出力
    if (TickTimer.available()) {
        iCounter++;
        if (iCounter == 1000) {
            iCounter = 0;
            Serial << '.';
        }
    }
}
