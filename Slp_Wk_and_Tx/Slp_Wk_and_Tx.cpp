// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>

#include "Common.h"

/*** function prototype */
MWX_APIRET vTransmit();
void SleepNow();

/*** application defs */
E_STATE eState;
MWX_APIRET txreq_stat; // check tx completion status
uint32_t u32millis_tx; // millis() at Tx 
int dummy_work_count;  // counter for dummy work job. 

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	txreq_stat = MWX_APIRET(false, 0);

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< TWENET::rx_when_idle(false);  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk	<< NWK_SIMPLE::logical_id(DEVICE_ID); // set Logical ID. 

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- Sleep an Tx Act ---" << crlf;
}

/*** begin procedure (called once at boot) */
void begin() {
	Serial << "..begin (run once at boot)" << crlf;
	SleepNow();
}

/*** wake up procedure */
void wakeup() {
	Serial << crlf << int(millis()) << ":wake up!" << crlf;
	eState = E_STATE::INIT;
}

/*** loop procedure (called every event) */
void loop() {
	bool loop_more; // if set, one more loop on state machine.

	do {
		loop_more = false; // set no-loop at the initial.

		switch(eState) {
			case E_STATE::INIT:
				eState = E_STATE::WORK_JOB;
				loop_more = true;

				dummy_work_count = 100;
			break;

			case E_STATE::WORK_JOB:
				// implement work job here (e.g. sensor capture)
				// (the dummy job is counting down to zero every ms.)
				if (TickTimer.available()) {
					Serial << '.';
					dummy_work_count--;
					if (dummy_work_count == 0) {
						Serial << crlf;
						eState = E_STATE::TX;
						loop_more = true;
					}
				}
			break;

			case E_STATE::TX:
				txreq_stat = vTransmit();
				if (txreq_stat) {
					Serial << int(millis()) << ":tx request success! (" << int(txreq_stat.get_value()) << ')' << crlf;
					u32millis_tx = millis();

					eState = E_STATE::WAIT_TX;
					loop_more = true;
				} else {
					Serial << int(millis()) << "!FATAL: tx request failed." << crlf;
				}
				break;

			case E_STATE::WAIT_TX:
				if (the_twelite.tx_status.is_complete(txreq_stat.get_value())) {
					Serial << int(millis()) << ":tx completed! (" << int(txreq_stat.get_value()) << ')' << crlf;
					eState = E_STATE::EXIT_NORMAL;
				} else if (millis() - u32millis_tx > 100) {
					Serial << int(millis()) << "!FATAL: tx timeout." << crlf;
					eState = E_STATE::EXIT_FATAL;
					loop_more = true;
				}
			break;
			
			case E_STATE::EXIT_NORMAL:
				SleepNow();
			break;

			case E_STATE::EXIT_FATAL:
				Serial << crlf << "!FATAL: RESET THE SYSTEM.";
				delay(100);
				the_twelite.reset_system();
			break;
		}

	} while (loop_more);
}

/** transmit a packet */
MWX_APIRET vTransmit() {
	Serial << int(millis()) << ":vTransmit()" << crlf;

	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x3 send four times in total)
			<< tx_packet_delay(0,0,2); // send packet w/ delay (send first packet with randomized delay from 0 to 0ms, repeat every 2ms)

		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair(FOURCC, 4) // string should be paired with length explicitly.
			, uint32_t(millis()) // put timestamp here.
		);
		
		// do transmit 
		//return nwksmpl.transmit(pkt);
		return pkt.transmit(); 
	}

	return MWX_APIRET(false, 0);
}

void SleepNow() {
	uint16_t u16dur = SLEEP_DUR;
	u16dur = random(SLEEP_DUR - SLEEP_DUR_TERMOR, SLEEP_DUR + SLEEP_DUR_TERMOR);

	Serial << int(millis()) << ":sleeping for " << int(u16dur) << "ms" << crlf << mwx::flush;
	the_twelite.sleep(u16dur, false);
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */