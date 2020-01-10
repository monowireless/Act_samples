/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */
#pragma once

#include <TWELITE>
#include <PAL_AMB>
#include "common.hpp"

class MY_APP_PARENT : MWX_APPDEFS_CRTP(MY_APP_PARENT)
{
public:
    static const uint8_t TYPE_ID = 0x01;

    // load common definition for handlers
    #define __MWX_APP_CLASS_NAME MY_APP_PARENT
    #include "_mwx_cbs_hpphead.hpp"
    #undef __MWX_APP_CLASS_NAME

public:
    // constructor
    MY_APP_PARENT() {}

    // begin method (if necessary, configure object here)
    void _setup() {
    }

    // begin method (if necessary, start object here)
    void _begin() {
    }

public:
    // TWENET callback handler (mandate)
    void loop();

    void on_sleep(uint32_t & val) {
    }

    void warmboot(uint32_t & val) {
    }

    void wakeup(uint32_t & val) {
    }

    void on_create(uint32_t& val) { _setup();  }
    void on_begin(uint32_t& val) { _begin(); }
    void on_message(uint32_t& val) { }

public: // never called the following as hardware class, but define it!
    void network_event(mwx::packet_ev_nwk& pEvNwk) {}
    void receive(mwx::packet_rx& rx);
    void transmit_complete(mwx::packet_ev_tx& pEvTx) {}

private:
public:
};

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */