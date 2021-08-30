/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */
#pragma once

#include <TWELITE>
#include <PAL_AMB>
#include "common.hpp"

class MY_APP_CHILD : MWX_APPDEFS_CRTP(MY_APP_CHILD)
{
public:
    static const uint8_t TYPE_ID = 0x02;

    // load common definition for handlers
    #define __MWX_APP_CLASS_NAME MY_APP_CHILD
    #include "_mwx_cbs_hpphead.hpp"
    #undef __MWX_APP_CLASS_NAME

public:
    // constructor
    MY_APP_CHILD() {}

    // begin method (if necessary, configure object here)
    void _setup() {
        i16Temp = 0;
        i16Humd = 0;
    }

    // begin method (if necessary, start object here)
    void _begin() {
        // sleep immediately.
        Serial << "..go into first sleep (1000ms)" << mwx::flush;
        the_twelite.sleep(1000);
    }

public: // syetem callback
    void on_create(uint32_t& val) { _setup();  } // called when registered.
    void on_begin(uint32_t& val) { _begin(); } // called when first call of ::loop()
    void on_message(uint32_t& val) { }

public:
    // TWENET callback handler

    // loop(), same with ::loop()
    void loop() {}

    // called when about to sleep.
    void on_sleep(uint32_t & val) {}

    // callback when waking up from sleep (very early stage, no init of peripherals.)
    void warmboot(uint32_t & val) {}

    // callback when waking up from sleep
    void wakeup(uint32_t & val) {
        Serial << mwx::crlf << "..wakeup" << mwx::crlf;
        // init wire device.
        Wire.begin();
        
        // turn on LED
        digitalWrite(PAL_AMB::PIN_LED, PIN_STATE::LOW);

        // KICK it!
        PEV_Process(E_ORDER_KICK, 0); // pass the event to state machine
    }

public: // never called the following as hardware class, but define it!
    void receive(mwx::packet_rx& rx) {}
    void transmit_complete(mwx::packet_ev_tx& txev) {
        Serial << "..txcomp=" << int(txev.u8CbId) << mwx::crlf;
        PEV_Process(E_ORDER_KICK, txev.u8CbId); // pass the event to state machine
    }
    void network_event(mwx::packet_ev_nwk& pEvNwk) {}

// HERE, APP SPECIFIC DEFS, you can use any up to your design policy.
public:
    static const uint8_t STATE_IDLE = E_MWX::STATE_0;
    static const uint8_t STATE_SENSOR = E_MWX::STATE_1;
    static const uint8_t STATE_TX = E_MWX::STATE_2;
    static const uint8_t STATE_SLEEP = E_MWX::STATE_3;
    
private:
	int16_t i16Temp;
	int16_t i16Humd;
    uint32_t u32Lumi;

public:
    MWX_APIRET shtc3_start();
    MWX_APIRET shtc3_read();

    MWX_APIRET ltr308als_start();
    MWX_APIRET ltr308als_read();
};

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */