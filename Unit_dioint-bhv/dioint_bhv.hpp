#pragma once
#include "dioint.h"

class MyApp : MWX_APPDEFS_CRTP(MyApp)
{
public:
    static const uint8_t TYPE_ID = 0x01;

    // template
    #define __MWX_APP_CLASS_NAME MyApp
    #include "_mwx_cbs_hpphead.hpp"
    #undef __MWX_APP_CLASS_NAME

public:
    // constructor
    MyApp() { }

public: // syetem callback
    void on_create(uint32_t& val) { } // called when registered.
    void on_begin(uint32_t& val) { } // called when first call of ::loop()
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
    void wakeup(uint32_t & val) {}

public: // never called the following as hardware class, but define it!
    void receive(mwx::packet_rx& rx) {}
    void transmit_complete(mwx::packet_ev_tx& txev) {}
    void network_event(mwx::packet_ev_nwk& pEvNwk) {}
};