// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <SM_SIMPLE>

#include "Common.h"

/*** function prototype */
MWX_APIRET Transmit();
void SleepNow();

/*** application defs */
// state machine
SM_SIMPLE<STATE> step;

// sensor capture data (dummy)
struct {
	uint16_t dummy_work_ct_now;
	uint16_t dummy_work_ct_max;  // counter for dummy work job. 
} sensor;

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	step.setup(); // init state machine

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
	memset(&sensor, 0, sizeof(sensor));
	Serial << crlf << int(millis()) << ":wake up!" << crlf;
}

/*** loop procedure (called every event) */
void loop() {
	do {
		switch(step.state()) {
		case STATE::INIT:
			sensor.dummy_work_ct_now = 0;
			sensor.dummy_work_ct_max = random(10,1000);
			
			step.next(STATE::WORK_JOB);
		break;

		case STATE::WORK_JOB:
			// implement work job here (e.g. sensor capture)
			// (the dummy job is counting down to zero every ms.)
			if (TickTimer.available()) {
				Serial << '.';
				sensor.dummy_work_ct_now++;
				if (sensor.dummy_work_ct_now >= sensor.dummy_work_ct_max) {
					Serial << crlf;
					step.next(STATE::TX);
				}
			}
		break;

		case STATE::TX:
			if (Transmit()) {
				Serial << int(millis()) << ":tx request success!" << crlf;
				step.set_timeout(100);
				step.clear_flag();
				step.next(STATE::WAIT_TX);
			} else {
				// normall it should not be here.
				Serial << int(millis()) << "!FATAL: tx request failed." << crlf;
				step.next(STATE::EXIT_FATAL);
			}
		break;

		case STATE::WAIT_TX:
			if (step.is_flag_ready()) {
				Serial << int(millis()) << ":tx completed!" << crlf;
				step.next(STATE::EXIT_NORMAL);
			} else if (step.is_timeout()) {
				Serial << int(millis()) << "!FATAL: tx timeout." << crlf;
				step.next(STATE::EXIT_FATAL);
			}
		break;
			
		case STATE::EXIT_NORMAL:
			SleepNow();
		break;

		case STATE::EXIT_FATAL:
			Serial << crlf << "!FATAL: RESET THE SYSTEM.";
			delay(1000); // wait a while.
			the_twelite.reset_system();
		break;

		default: // should not be here.
			step.next(STATE::EXIT_FATAL);
		break;
		}
	} while (step.b_more_loop());
}

/** transmit complete */
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	step.set_flag(ev.bStatus);
}

/** transmit a packet */
MWX_APIRET Transmit() {
	Serial << int(millis()) << ":vTransmit()" << crlf;

	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x3 send four times in total)
			<< tx_packet_delay(0,0,2); // send packet w/ delay (send first packet with randomized delay from 0 to 0ms, repeat every 2ms)

#if 1
		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair(FOURCC, 4) // string should be paired with length explicitly.
			, uint32_t(millis()) // put timestamp here.
			, uint16_t(sensor.dummy_work_ct_now) // put dummy sensor information.
		);	
#else
		// same payload gerenation as above by using uint8_t*.
		auto&& pay = pkt.get_payload(); // get buffer object.

		// the following code will write data directly to internal buffer of `pay' object.
		uint8_t *p = pay.begin(); // get the pointer of buffer head.
		
		S_OCTET(p, FOURCC[0]); // store byte at pointer `p' and increment the pointer.
		S_OCTET(p, FOURCC[1]);
		S_OCTET(p, FOURCC[2]);
		S_OCTET(p, FOURCC[3]);

		S_DWORD(p, millis()); // store uint32_t data.
		S_WORD(p, sensor.dummy_work_ct_now); // store uint16_t data.

		// Set the size of payload (redim() should be used. resize() will clear buffer.)
		pay.redim(87+6); //p - pay.begin());
#endif
		
		// do transmit 
		//return nwksmpl.transmit(pkt);
		return pkt.transmit(); 
	}

	return MWX_APIRET(false, 0);
}

void SleepNow() {
	uint16_t u16dur = SLEEP_DUR;
	u16dur = random(SLEEP_DUR - SLEEP_DUR_TERMOR, SLEEP_DUR + SLEEP_DUR_TERMOR);

	Serial << int(millis()) << ":sleeping for " << int(u16dur) << "ms" << crlf;
	Serial.flush();

	step.on_sleep(); // reset status of statemachine to INIT state.

	the_twelite.sleep(u16dur, false);
}

/* Copyright (C) 2020-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */