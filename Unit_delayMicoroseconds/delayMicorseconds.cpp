#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
}

/*** the loop procedure (called every event) */
void loop() {
    static int ct;
    if (TickTimer.available()) {
        ct++;

        if ((ct & 0xFF) == 0xFF) {
            uint32_t t0, t1;
            uint32_t msCt = 10 * ((ct >> 8) & 63);

            t0 = u32AHI_TickTimerRead();
            delayMicroseconds(msCt);
            t1 = u32AHI_TickTimerRead();
        
            // 16ct = 1us (TickTimer runs at 16MHz)
            Serial << format("%04d -> %d", msCt, int(t1 - t0) / 16) << crlf;
        }
    }
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */