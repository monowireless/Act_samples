#include <TWELITE>

/*** the setup procedure (called on boot) */
void setup() {
}

/*** the loop procedure (called every event) */
void begin() {
    int val_start = -99999;
    int val_end = 99999;

    vAHI_WatchdogStop();

    // div10()
    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div10(i);
            sum += r.b_neg ? -(r.quo + r.rem) : (r.quo + r.rem);
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div10() took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            int32_t quo = i / 10;
            int32_t rem = i - quo * 10;
            sum += quo + rem;
        }
        uint32_t tick_end = millis();

        Serial << crlf << "/10,%10 took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        int errct = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div10(i);
            
            int32_t quo = std::abs(i) / 10;
            int32_t rem = std::abs(i) % 10;
            if (quo != r.quo || rem != r.rem) {
                Serial << crlf << '{' << i << ',' << int(r.quo) << ',' << int(r.rem) << '}';
                Serial.flush();

                errct++;
            }
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div10() error = " << errct;
    }
   
    // div100()
    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div100(i);
            sum += r.b_neg ? -(r.quo + r.rem) : (r.quo + r.rem);
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div100() took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            int32_t quo = i / 100;
            int32_t rem = i - quo * 100;
            sum += quo + rem;
        }
        uint32_t tick_end = millis();

        Serial << crlf << "/100,%100 took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        int errct = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div100(i);
            
            int32_t quo = std::abs(i) / 100;
            int32_t rem = std::abs(i) % 100;
            if (quo != r.quo || rem != r.rem) {
                Serial << crlf << '{' << i << ',' << int(r.quo) << ',' << int(r.rem) << '}';
                Serial.flush();

                errct++;
            }
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div100() error = " << errct;
    }
    
    // div1000()
    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div1000(i);
            sum += r.b_neg ? -(r.quo + r.rem) : (r.quo + r.rem);
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div1000() took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            int32_t quo = i / 1000;
            int32_t rem = i - quo * 1000;
            sum += quo + rem;
        }
        uint32_t tick_end = millis();

        Serial << crlf << "/1000,%1000 took " << int(tick_end - tick_start) << "ms. sum=" << sum;
    }

    if (1) {
        int sum = 0;
        int errct = 0;
        uint32_t tick_start = millis();
        for (int i = val_start; i <= val_end; i++) {
            auto r = div1000(i);
            
            int32_t quo = std::abs(i) / 1000;
            int32_t rem = std::abs(i) % 1000;
            if (quo != r.quo || rem != r.rem) {
                Serial << crlf << '{' << i << ',' << int(r.quo) << ',' << int(r.rem) << '}';
                Serial.flush();

                errct++;
            }
        }
        uint32_t tick_end = millis();

        Serial << crlf << "div1000() error = " << errct;
    }
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */