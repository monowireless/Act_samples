// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <MONOSTICK>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

/*** function prototype */

/*** application defs */

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	auto&& brd = the_twelite.board.use<MONOSTICK>();
	brd.set_led_red(LED_TIMER::ON_RX, 200); // RED (on receiving)
	brd.set_led_yellow(LED_TIMER::BLINK, 500); // YELLOW (blinking)

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	auto&& nwksmpl = the_twelite.network.use<NWK_SIMPLE>();
	nwksmpl << NWK_SIMPLE::logical_id(0x00); // set Logical ID. (0x00 means parent device)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- MONOSTICK_Parent act ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	// receive RF packet.
    while (the_twelite.receiver.available()) {
		auto&& rx = the_twelite.receiver.read();

		Serial << ".. coming packet (" << int(millis()&0xffff) << ')' << mwx::crlf;

		// output type1 (raw packet)
		//   uint8_t  : 0x01
		//   uint8_t  : src addr (LID)
		//   uint32_t : src addr (long)
		//   uint32_t : dst addr (LID/long)
		//   uint8_t  : repeat count
		//     total 11 bytes of header.
		//     
		//   N        : payload
		{
			serparser_attach pout;
			pout.begin(PARSER::ASCII, rx.get_psRxDataApp()->auData, rx.get_psRxDataApp()->u8Len, rx.get_psRxDataApp()->u8Len);

			Serial << "RAW PACKET -> ";
			pout >> Serial;
			Serial << mwx::flush;
		}

		// output type2 
		{
			smplbuf_u8<128> buf;
			pack_bytes(buf
				, uint8_t(rx.get_addr_src_lid())		// src addr (LID)
				, uint8_t(0xCC)							// cmd id (0xCC, fixed)
				, uint8_t(rx.get_psRxDataApp()->u8Seq)	// seqence number
				, uint32_t(rx.get_addr_src_long())		// src addr (long)
				, uint32_t(rx.get_addr_dst())			// dst addr
				, uint8_t(rx.get_lqi())					// LQI
				, uint16_t(rx.get_length())				// payload length
				, rx.get_payload() 						// payload
					// , make_pair(rx.get_payload().begin() + 4, rx.get_payload().size() - 4)
					//   note: if you want the part of payload, use make_pair().
			);

			serparser_attach pout;
			pout.begin(PARSER::ASCII, buf.begin(), buf.size(), buf.size());
			
			Serial << "FMT PACKET -> ";
			pout >> Serial;
			Serial << mwx::flush;
		}
	}
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */