// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_AMB> // include the board support of PAL_AMB

#include "common.hpp"

#include "Child/myAppBhvChild.hpp"
#include "Parent/myAppBhvParent.hpp"

/*** Local objects */
uint8_t u8ID = 0;

/*** Local function prototypes */


/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	// use PAL_AMB board behavior.
	auto&& brd = the_twelite.board.use<PAL_AMB>();

	// now read DIP sw status can be read.
	u8ID = (brd.get_DIPSW_BM() & 0x07);

	// Register App Behavior (set differnt Application by DIP SW settings)
	if (u8ID == 0) {
		// put settings to the twelite main object.
		the_twelite
			<< TWENET::appid(APP_ID)     // set application ID (identify network group)
			<< TWENET::channel(CHANNEL)  // set channel (pysical channel)
			<< TWENET::rx_when_idle();   // open RX channel

		the_twelite.app.use<MY_APP_PARENT>();
	} else {		
		// put settings to the twelite main object.
		the_twelite
			<< TWENET::appid(APP_ID)     // set application ID (identify network group)
			<< TWENET::channel(CHANNEL); // set channel (pysical channel)

		the_twelite.app.use<MY_APP_CHILD>();
	}

	// Register Network Behavior
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk << NWK_SIMPLE::logical_id(u8ID); // set Logical ID.

	/*** BEGIN section */
	the_twelite.begin(); // let the twelite begin!
	Serial << "---PAL_AMB-behavior id=" << int(u8ID) << "---" << mwx::crlf;
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */