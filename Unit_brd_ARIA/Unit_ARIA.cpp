#include <TWELITE>
#include <ARIA>

/*** the setup procedure (called on boot) */
void setup() {
    auto&& aria = the_twelite.board.use<ARIA>();
    the_twelite.begin(); // here aria object is setup.
}

/*** run only once at the initial */
void begin() {
    auto&& aria = the_twelite.board.use<ARIA>();

    aria.led_one_shot(100); // flashing LED for 100ms.

    Serial << crlf << "--- ARIA peripheral testing ---";
    Serial << crlf << " s : sleep 10sec or MAGnet waking.";
    Serial << crlf << " t : start measurement Temperature and Humidity.";
    Serial << crlf << " l : turn LED on for 1sec.";
    delay(100);
}

/*** on wake up */
void wakeup() {
    auto&& aria = the_twelite.board.use<ARIA>();
    aria.led_one_shot(100); // flashing LED for 100ms.

    Serial << crlf;
    if (the_twelite.is_wokeup_by_wktimer()) {
        Serial << "..waking up (WakeTimer)";
    } else 
    if (the_twelite.is_wokeup_by_dio(ARIA::PIN_SNS_NORTH)) {
        Serial << "..waking up (MAGnet INT [N]).";
    } else 
    if (the_twelite.is_wokeup_by_dio(ARIA::PIN_SNS_SOUTH)) {
        Serial << "..waking up (MAGnet INT [S])";
    } else {
        Serial << "..waking up (unknown source).";
    }
}

/*** the loop procedure (called every event) */
void loop() {
    auto&& aria = the_twelite.board.use<ARIA>();

    if (Serial.available()) {
        int c = Serial.read();
            
        if (c >= 0) {
            Serial << crlf << '[' << char_t(c) << '/' << int(millis() & 0xFFFF) << "] ";

            switch(c) {
                case 's':
                {
                    Serial << "..gonna sleep for 10sec...";
                    
                    // set an interrupt for MAGnet sensor.
                    pinMode(ARIA::PIN_SNS_OUT1, PIN_MODE::WAKE_FALLING);
                    pinMode(ARIA::PIN_SNS_OUT2, PIN_MODE::WAKE_FALLING);

                    // perform sleep
                    the_twelite.sleep(10000, false);
                }
                break;

                case 'l':
                {
                    Serial << "..turn led on for 1000ms...";
                    aria.led_one_shot(1000);
                }
                break;

                case 't':
                {
                    aria.sns_SHT4x.begin();
                }
                break;
            }

        }
    }
    
    // wait for MOTion sensor capture.
    if (aria.sns_SHT4x.available()) {
        aria.sns_SHT4x.end();

        double temp = aria.sns_SHT4x.get_temp();
        double hum = aria.sns_SHT4x.get_humid();

        Serial << format( "temp=%.02f, hum=%.02f", temp, hum ) << crlf;
    }else{
        aria.sns_SHT4x.process_ev(E_EVENT_TICK_TIMER);
    }

    if (!(millis() & 0x3ff)) { // every 1sec
        ;
    }
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */