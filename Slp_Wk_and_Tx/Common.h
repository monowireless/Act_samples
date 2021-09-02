#pragma once

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

// DIO pins
const uint8_t PIN_BTN = 12;

// ID
const uint8_t DEVICE_ID = 0xFE; // LOGICAL ID (0xFE: Anonymous, 1..0xEF: Child Device)

// FOURCC
#define FOURCC "TXSP"

// Sleep Dur
const uint16_t SLEEP_DUR = 2000; // every 2000ms
const uint16_t SLEEP_DUR_TERMOR = 200; // SLEEP_DUR +/- 200

// Application control
enum class STATE {
    INIT = 0,    // INIT STATE
    WORK_JOB,    // do some job (e.g sensor capture)
    TX,          // reuest transmit
    WAIT_TX,     // wait its completion
    EXIT_NORMAL, // normal exiting.
    EXIT_FATAL   // has a fatal error (will do system reset)
};

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */