// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_MAG> // include the board support of PAL_MAG

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

// id
uint8_t u8ID = 0;

// application use
const uint8_t FOURCHARS[] = "PMG1";

bool b_transmit = false;
uint8_t u8txid = 0;

/*** Local function prototypes */
void sleepNow();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	// use PAL_AMB board support.
	auto&& brd = the_twelite.board.use<PAL_MAG>();
	// now it can read DIP sw status.
	u8ID = (brd.get_DIPSW_BM() & 0x07) + 1;
	if (u8ID == 0) u8ID = 0xFE; // 0 is to 0xFE

	// LED setup (use periph_led_timer, which will re-start on wakeup() automatically)
	brd.set_led(LED_TIMER::BLINK, 10); // blink (on 10ms/ off 10ms)

	// the twelite main object.
	the_twelite
		<< TWENET::appid(APP_ID)     // set application ID (identify network group)
		<< TWENET::channel(CHANNEL); // set channel (pysical channel)

	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk << NWK_SIMPLE::logical_id(u8ID); // set Logical ID. (0xFE means a child device with no ID)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- PAL_MAG:" << FOURCHARS << " ---" << mwx::crlf;
}

/*** begin procedure the first call */
void begin() {
	sleepNow();
}

/*** loop procedure (called every event) */
void loop() {
	if (!b_transmit) {
		if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
			uint8_t b_north = the_twelite.is_wokeup_by_dio(PAL_MAG::PIN_SNS_NORTH);
			uint8_t b_south = the_twelite.is_wokeup_by_dio(PAL_MAG::PIN_SNS_SOUTH);

			Serial << "..sensor north=" << int(b_north) << " south=" << int(b_south) << mwx::crlf;

			// set tx packet behavior
			pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
				<< tx_retry(0x1) // set retry (0x1 send two times in total)
				<< tx_packet_delay(0, 0, 2); // send packet w/ delay

			// prepare packet payload
			pack_bytes(pkt.get_payload() // set payload data objects.
				, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
				, b_north
				, b_south
			);

			// do transmit
			MWX_APIRET ret = pkt.transmit();
			Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf << mwx::flush;


			if (ret) {
				u8txid = ret.get_value() & 0xFF;
				b_transmit = true;
			}
			else {
				// fail to request
				sleepNow();
			}
		}
	} else { 
		if (the_twelite.tx_status.is_complete(u8txid)) {		
			Serial << "..transmit complete." << mwx::crlf << mwx::flush;
			b_transmit = 0;

			// now sleeping
			sleepNow();
		}
	}
}

// perform sleeping
void sleepNow() {
	uint32_t u32ct = 60000;
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf << mwx::flush;

	pinMode(PAL_MAG::PIN_SNS_OUT1, PIN_MODE::WAKE_FALLING);
	pinMode(PAL_MAG::PIN_SNS_OUT2, PIN_MODE::WAKE_FALLING);

	the_twelite.sleep(u32ct);
}

// wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- PAL_MAG:" << FOURCHARS << " wake up ---"
			<< mwx::crlf;

	if (the_twelite.is_wokeup_by_wktimer()) {
		Serial << "..wk timer, sleep again.." << mwx::crlf;
		sleepNow();
	}
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */