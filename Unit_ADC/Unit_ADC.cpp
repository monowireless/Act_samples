#include <TWELITE>
#include <NWK_SIMPLE>

bool b_activate_adc3_4 = false;

/*** the setup procedure (called on boot) */
void setup() {
    // setup ADC
    //  IMPORTANT NOTICE: ADC3(DIO0), ADC4(DIO1) will have small current leak while sleeping.

    // setup ADC hardware.
    Analogue.setup();
}

/*** run only once at the initial */
void begin() {
    Serial << crlf << "--- ADC testing ---";
    Serial << crlf << " capture ADC ports every 100ms and display them every seconds.";
    Serial << crlf << " [s] : sleep for 5 seconds.";
    Analogue.begin(pack_bits(PIN_ANALOGUE::A1,PIN_ANALOGUE::A2,PIN_ANALOGUE::VCC), 100);
    delay(100);
}

/*** on wake up */
void wakeup() {
    Serial << "waking up.";
}

/*** the loop procedure (called every event) */
void loop() {
    if (Serial.available()) {
        int c = Serial.read();

        if (c >= 0) {
            Serial << crlf << int(millis() & 0xFFFF) << '[' << char_t(c) << "] ";

            if (c == 's') {
                Serial << "gonna sleep...";
                if (b_activate_adc3_4) {
                    // needs Analogue curcuit GND to be aparted from circuit.
                    // digitalWrite(PIN_DIGITAL::DIOXX, LOW); // e.g. control N-FET circuit switch.

                    // then, set pull up for ADC3/4 pin.
                    // pinMode(PIN_DIGITAL::DIO0, PIN_MODE::INPUT_PULLUP);
                    // pinMode(PIN_DIGITAL::DIO1, PIN_MODE::INPUT_PULLUP);
                }

                the_twelite.sleep(5000);
            }

            if (c == 'a') {
                if (!b_activate_adc3_4) {
                    b_activate_adc3_4 = true;
                
                    pinMode(PIN_DIGITAL::DIO0, PIN_MODE::INPUT);
                    pinMode(PIN_DIGITAL::DIO1, PIN_MODE::INPUT);
                    
                    Analogue.begin(pack_bits(PIN_ANALOGUE::A1,PIN_ANALOGUE::A2,PIN_ANALOGUE::A3,PIN_ANALOGUE::A4,PIN_ANALOGUE::VCC), 100);
                } else {
                    b_activate_adc3_4 = false;
                   
                    Analogue.begin(pack_bits(PIN_ANALOGUE::A1,PIN_ANALOGUE::A2,PIN_ANALOGUE::VCC), 100);
                }
            }
        }
    }

    if (!(millis() & 0x3ff)) {
        Serial << crlf << int(millis() & 0xFFFF) << "[ADC]";
        Serial << " AD1=" << int(Analogue.read(PIN_ANALOGUE::A1));
        Serial << " AD2=" << int(Analogue.read(PIN_ANALOGUE::A2));
        if (b_activate_adc3_4) {
            Serial << " AD3=" << int(Analogue.read(PIN_ANALOGUE::A3));
            Serial << " AD4=" << int(Analogue.read(PIN_ANALOGUE::A4));
        }
        Serial << " VCC=" << int(Analogue.read(PIN_ANALOGUE::VCC));
    }
}
