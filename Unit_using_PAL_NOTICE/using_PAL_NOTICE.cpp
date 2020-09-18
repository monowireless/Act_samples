#include <TWELITE>
#include <PAL_NOTICE>

inline uint8_t toggle3(uint8_t& v) {
    v++;
    if (v > 3) v = 0;
    return v;
}

inline uint16_t toggle10(uint8_t& v) {
    v++;
    if (v > 10) v = 0;
    return v;
}

/*** the setup procedure (called on boot) */
void setup() {
    // board
	auto&& brd = the_twelite.board.use<PAL_NOTICE>();
	brd.set_led(LED_TIMER::BLINK, 100);
}

/*** the loop procedure (called every event) */
void begin() {
	auto&& brd = the_twelite.board.use<PAL_NOTICE>();
    brd.test_led();

    // default s/w is off
    brd.set_led_master_sw_on();
}

void loop() {
	auto&& brd = the_twelite.board.use<PAL_NOTICE>();
    
    static uint8_t led_stat[4];
    const char str_led_stat[4][16] = {
        "OFF",
        "ON(FULL)",
        "ON",
        "BLINK"
    };
    static uint8_t led_blink_duty;
    static uint8_t led_blink_cycle;
    static uint8_t led_pwm[4];

    while(Serial.available()) {
        auto c = Serial.read();

        if (c >= 0x20 && c <= 0x7E)
            Serial << crlf << '[' << char_t(c) << ']' << ' ';

        switch(c) {
        case 'r': 
            brd.set_leds(toggle3(led_stat[0]), PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP);
            Serial << "R=" << str_led_stat[led_stat[0]];
            break;
        case 'R':
            brd.set_led_brightness_r1000(toggle10(led_pwm[0]) * 100);
            Serial << "R BRIGHTNESS=" << int(led_pwm[0]*100);
            break;
        case 'g':
            brd.set_leds(PAL_NOTICE::LED_NOP, toggle3(led_stat[1]), PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP);
            Serial << "G=" << str_led_stat[led_stat[1]];
            break;
        case 'G':
            brd.set_led_brightness_g1000(toggle10(led_pwm[1])*100);
            Serial << "G BRIGHTNESS=" << int(led_pwm[1]*100);
            break;
        case 'b':
            brd.set_leds(PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP, toggle3(led_stat[2]), PAL_NOTICE::LED_NOP);
            Serial << "B=" << str_led_stat[led_stat[2]];
            break;
        case 'B':
            brd.set_led_brightness_b1000(toggle10(led_pwm[2])*100);
            Serial << "B BRIGHTNESS=" << int(led_pwm[2]*100);
            break;
        case 'w':
            brd.set_leds(PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP, PAL_NOTICE::LED_NOP, toggle3(led_stat[3]));
            Serial << "W=" << str_led_stat[led_stat[3]];
            break;
        case 'W':
            brd.set_led_brightness_w1000(toggle10(led_pwm[3])*100);
            Serial << "W BRIGHTNESS=" << int(led_pwm[3]*100);
            break;

        case 'c':
            brd.set_blink_cycle_ms(toggle10(led_blink_cycle) * 100);
            Serial << "BLINK_CYCLE=" << int(led_blink_cycle * 100) << "ms.";
            break;
        case 'C':
            brd.set_blink_duty1000(toggle10(led_blink_duty) * 100);
            Serial << "BLINK_DUTY=" << int(led_blink_duty * 100);
            break;
        case '\r':
            Serial << crlf;
            Serial << crlf << "NOTICE PAL LED control:"
                   << crlf << "r,g,b,w -> toggle each LED status"
                   << crlf << "R,G,B,W -> toggle each LED brightness"
                   << crlf << "c       -> toggle blink cycle"
                   << crlf << "C       -> toggle blink duty"
                   << crlf;
            break;
        }
    }
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */