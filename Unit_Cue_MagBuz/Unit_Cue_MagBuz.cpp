/* Copyright (C) 2021 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/*
 * Cue_MagBuz : Sounds buzzer when magnet is not present for TWELITE CUE.
 * - The buzzer flicks as Pi-Pi-Pi-Pi driven by 4Hz timer.
 * - Connecte piezzo buzzer between DIO12(SET) and VCC.
 * - No RF function is implemented.
 * - No accelometer function is implemented.
 * - TWELITE OPEN-CLOSE SENSE PAL (MAG PAL) may work.
 */

#include <TWELITE>
#include <CUE>

// Pin Assign
const auto PIN_BUZ = PIN_DIGITAL::DIO12;
const auto PIN_LED = CUE::PIN_LED;

// function to going sleep
void reset_pins();
void go_sleeping();

// small macro, returns HIGH when true is given, otherwise LOW.
PIN_STATE::eMWX_DIO_HIGHLOW PIN_STATE_BY_BOOL(bool b) { return (b ? PIN_STATE::HIGH : PIN_STATE::LOW); }

/*** the setup procedure (called on boot) */
void setup() {
    // load TWELITE CUE class object.
    auto&& brd = the_twelite.board.use<CUE>();

    // the twelite
    the_twelite << twenet::mac_init_pending(); // no mac initialize (to save battery.)

    // Timer for Pi-Pi-Pi-Pi flickering sound.
    Timer1.begin(4);

    // initial pin settings.
    pinMode(PIN_BUZ, PIN_MODE::OUTPUT_INIT_HIGH);
    pinMode(CUE::PIN_SNS_OUT1, PIN_MODE::WAKE_RISING); // no pullup to save energy
    pinMode(CUE::PIN_SNS_OUT2, PIN_MODE::WAKE_RISING); // no pullup to save energy
}

/*** called when waking up */
void wakeup() {
    Serial << crlf << "w";

    // If either of NORTH/SOUTH pin had an interrupt, the magnet state is changed.
    if (   the_twelite.is_wokeup_by_dio(CUE::PIN_SNS_NORTH)
        || the_twelite.is_wokeup_by_dio(CUE::PIN_SNS_SOUTH)) {
        ; // wakeup by magnet change.
    } else {
        // wakeup, but no change in magnet status.
        go_sleeping();
    }
}

/*** the loop */
bool b_phase = false; // use as soft PWM control
bool b_on = false; // true when beeping
void loop() {
    if (TickTimer.available()) { // TickTimer is working at 1KHz
        if (b_on) {
            b_phase = !b_phase;
            digitalWrite(PIN_BUZ, PIN_STATE_BY_BOOL(b_phase)); // Sounds 500Hz
        }
    }
    if (Timer1.available()) {
        b_on = !b_on; // turn flag
        Serial << (b_on ? '+' : '.'); // Serial Message
        digitalWrite(PIN_LED, PIN_STATE_BY_BOOL(b_on)); // LED

        // check pin state
        auto pin_n = digitalRead(CUE::PIN_SNS_NORTH);
        auto pin_s = digitalRead(CUE::PIN_SNS_SOUTH);
        if (pin_n == PIN_STATE::LOW || pin_s == PIN_STATE::LOW) {
            go_sleeping();
        }
    }
}

/*** function to going sleep. */
void go_sleeping() {
    Serial << 's'; // Serial Message
    digitalWrite(PIN_LED, PIN_STATE::HIGH);
    digitalWrite(PIN_BUZ, PIN_STATE::HIGH);

    pinMode(CUE::PIN_SNS_OUT1, PIN_MODE::WAKE_RISING);
    pinMode(CUE::PIN_SNS_OUT2, PIN_MODE::WAKE_RISING);

    the_twelite.sleep(30000, false); // wakeup every 30sec.
}
