#include <TWELITE>
#include <CUE>

bool b_activate_adc3_4 = false;

/*** the setup procedure (called on boot) */
void setup() {
    auto&& cue = the_twelite.board.use<CUE>();
    the_twelite.begin(); // here cue object is setup.
}

/*** run only once at the initial */
void begin() {
    auto&& cue = the_twelite.board.use<CUE>();

    cue.led_one_shot(100); // flashing LED for 100ms.

    Serial << crlf << "--- CUE peripheral testing ---";
    Serial << crlf << " s : sleep 10sec or MAGnet waking.";
    Serial << crlf << " a : start MOTion sensor and sleep, waking at capture finish.";
    Serial << crlf << " l : turn LED on for 1sec.";
    delay(100);
}

/*** on wake up */
void wakeup() {
    auto&& cue = the_twelite.board.use<CUE>();
    cue.led_one_shot(100); // flashing LED for 100ms.

    Serial << crlf;
    if (the_twelite.is_wokeup_by_wktimer()) {
        Serial << "..waking up (WakeTimer)";
    } else 
    if (the_twelite.is_wokeup_by_dio(CUE::PIN_SNS_INT)) {
        Serial << "..waking up (MOTion INT)";
    } else 
    if (the_twelite.is_wokeup_by_dio(CUE::PIN_SNS_NORTH)) {
        Serial << "..waking up (MAGnet INT [N]).";
    } else 
    if (the_twelite.is_wokeup_by_dio(CUE::PIN_SNS_SOUTH)) {
        Serial << "..waking up (MAGnet INT [S])";
    } else {
        Serial << "..waking up (unknown source).";
    }
}

/*** the loop procedure (called every event) */
void loop() {
    auto&& cue = the_twelite.board.use<CUE>();

    if (Serial.available()) {
        int c = Serial.read();
            
        if (c >= 0) {
            Serial << crlf << '[' << char_t(c) << '/' << int(millis() & 0xFFFF) << "] ";

            switch(c) {
                case 's':
                {
                    Serial << "..gonna sleep for 10sec...";
                    
                    // set an interrupt for MAGnet sensor.
                    pinMode(CUE::PIN_SNS_OUT1, PIN_MODE::WAKE_FALLING);
                    pinMode(CUE::PIN_SNS_OUT2, PIN_MODE::WAKE_FALLING);

                    // perform sleep
                    the_twelite.sleep(10000, false);
                }
                break;

                case 'l':
                {
                    Serial << "..turn led on for 1000ms...";
                    cue.led_one_shot(1000);
                }
                break;

                case 'a':
                {
                    Serial << "..start MOTion sensor and sleep...";
                    
                    cue.sns_MC3630.begin(SnsMC3630::Settings(
                        SnsMC3630::MODE_LP_14HZ, SnsMC3630::RANGE_PLUS_MINUS_4G, 8)); // begin MC3630

                    // perform sleep
                    the_twelite.sleep(2000, false);
                }
                break;
            }

        }
    }
    
    // wait for MOTion sensor capture.
    if (cue.sns_MC3630.available()) {
        // stop FIFO capture
        cue.sns_MC3630.end();

        Serial << crlf
            << "..MOTion data is ready."
            << " (ct=" << int(cue.sns_MC3630.get_que().size()) << ')';

        // get all samples and average them.
        int32_t x = 0, y = 0, z = 0;
        for (auto&& v: cue.sns_MC3630.get_que()) {
            Serial << crlf << format(" X=%+05d Y=%+05d Z=%+05d", v.x, v.y, v.z);
        }

        // clear available flag
      	cue.sns_MC3630.get_que().clear(); // clean up the queue
    }

    if (!(millis() & 0x3ff)) { // every 1sec
        ;
    }
}
