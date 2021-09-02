// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

// DIO pins
const uint8_t PIN_BTN = 12;

/*** function prototype */
MWX_APIRET Transmit();

/*** application defs */
MWX_APIRET tx_busy; // check tx completion status

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	tx_busy = false;

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk	<< NWK_SIMPLE::logical_id(0xFE); // set Logical ID. (0xFE means a child device with no ID)

	/*** BEGIN section */
	Buttons.begin(pack_bits(PIN_BTN), 5, 10); // check every 10ms, a change is reported by 5 consequent values.
	// Analogue.begin(pack_bits(PIN_ANALOGUE::A1, PIN_ANALOGUE::VCC)); // _start continuous adc capture.

	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- Scratch act ---" << mwx::crlf;
}

/*** begin procedure (called once at boot) */
void begin() {
	Serial << "..begin (run once at boot)" << mwx::crlf;
}

/** on wake up */
void wakeup() {
	Serial << int(millis()) << ":wake up!" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	// digital input change.
	if (Buttons.available()) {
		uint32_t bm, cm;
		Buttons.read(bm, cm);

		if (cm & 0x80000000) {
			// the first capture. (skip it!)
		}

		Serial << int(millis()) << ":BTN" << format("%b") << mwx::crlf;
	}

    // read from serial
	while(Serial.available())  {
        int c = Serial.read();

		Serial << '[' << char(c) << ']';

        switch(c) {
			case 'p':
				Serial << int(millis()) << ":p pressed!" << mwx::crlf;
				break;

			case 't':
				if (!tx_busy) {
					tx_busy = Transmit();
					if (tx_busy) {
						Serial << int(millis()) << ":tx request success! (" << int(tx_busy.get_value()) << ')' << mwx::crlf;
 					} else {
						Serial << int(millis()) << ":tx request failed" << mwx::crlf;;
					}
				}
				break;

			case 's':
				Serial << int(millis()) << ":sleeping for " << 5000 << "ms" << mwx::crlf << mwx::flush;
				the_twelite.sleep(5000);
				break;

            default:
				break;
        }
	}
}

/** on receiving a packet. */
void on_rx_packet(packet_rx& rx, bool_t &handled) {
	Serial << format("rx from %08x/%d", rx.get_addr_src_long(), rx.get_addr_src_lid()) << mwx::crlf;
}

/** on completion of transmitting a packet. */
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	Serial 	<< int(millis()) << ":tx completed!"
			<< format("(id=%d, stat=%d)", ev.u8CbId, ev.bStatus) << mwx::crlf;
	tx_busy = false; // clear tx busy flag.
}

/** transmit a packet */
MWX_APIRET Transmit() {
	Serial << int(millis()) << ":Transmit()" << mwx::crlf;

	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(0xFF)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x3 send four times in total)
			<< tx_packet_delay(100,200,20); // send packet w/ delay (send first packet with randomized delay from 100 to 200ms, repeat every 20ms)

		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair("SCRT", 4) // string should be paired with length explicitly.
			, uint32_t(millis()) // put timestamp here.
		);
		
		// do transmit 
		//return nwksmpl.transmit(pkt);
		return pkt.transmit(); 
	} else {
		Serial << "TX QUEUE is FULL" << mwx::crlf;
		return MWX_APIRET(false, 0);
	}
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE        *
 * AGREEMENT).                                                        */