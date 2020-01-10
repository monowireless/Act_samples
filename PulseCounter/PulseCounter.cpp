// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_MAG> // if use PAL board, you should include board support (to handle h/w watchdog timer).

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

// id
uint8_t u8ID = 0xFE;

// application use
const uint8_t FOURCHARS[] = "PLS1";

bool b_transmit = false;
uint8_t u8txid = 0;

/*** Local function prototypes */
void sleepNow();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	// use PAL_AMB board support.
	auto&& brd = the_twelite.board.use<PAL_MAG>();

	// LED setup (use periph_led_timer, which will re-start on wakeup() automatically)
	brd.set_led(LED_TIMER::BLINK, 10); // blink (on 10ms/ off 10ms)

	// Pulse Counter setup
	PulseCounter.setup();

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
	Serial << "--- Pulse Counter:" << FOURCHARS << " ---" << mwx::crlf;
}

/*** begin procedure the first call */
void begin() {
	// start the pulse counter capturing
	PulseCounter.begin(
		  100 // 100 count to wakeup
		, PIN_INT_MODE::FALLING // falling edge
		);

	sleepNow();
}

/*** loop procedure (called every event) */
void loop() {
	// auto&& brd = the_twelite.board.use<PAL_MAG>();

	if (!b_transmit) {
		uint16_t u16ct = PulseCounter.read(); // read the pulse counter and reset it.
		static uint16_t u16seq;

		Serial << "..pulse counter=" << int(u16ct) << mwx::crlf;

		// set tx packet behavior
		if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
			pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
				<< tx_retry(0x1) // set retry (0x1 send two times in total)
				<< tx_packet_delay(0, 0, 2); // send packet w/ delay

			// prepare packet payload
			pack_bytes(pkt.get_payload() // set payload data objects.
				, make_pair(FOURCHARS, 4) 	// just to see packet identification, you can design in any.
				, u16seq++					// include sequence counter.
				, u16ct						// the pulse counter count.
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
			// note: if pulses comes too frequenly, the interrupt might be skipped,
			//       because measured count may exceed the refrence value here.
			sleepNow();
		}
	}
}

// perform sleeping
void sleepNow() {
	uint32_t u32ct = 1000; // 10sec
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf << mwx::flush;

	the_twelite.sleep(u32ct, false);
}

// wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- Pulse Counter:" << FOURCHARS << " wake up ---"
			<< mwx::crlf;

	if (!PulseCounter.available()) {
		Serial << "..pulse counter does not reach the reference value." << mwx::crlf;
		sleepNow();
	}
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */