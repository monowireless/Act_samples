#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
    // initialize the object. (allocate Tx/Rx buffer, and etc..)
    Serial1.setup(64, 192);

    // start the peripheral with 115200bps.
    Serial1.begin(115200);

    // if uses alternative port DIO14(TxD),15(RxD) instead of DIO11,9.
    // Serial1.begin(115200, uint8_t(serial_jen::E_CONF::PORT_ALT));
}

/*** the loop procedure (called every event) */
void loop() {
    while(Serial1.available()) {
        auto c = Serial1.read();
        Serial << format("[%c]", c);
    }

    while(Serial.available()) {
        auto c = Serial.read();
        Serial1 << char_t(c);
    }
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */