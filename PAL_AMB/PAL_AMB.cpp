// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_AMB> // include the board support of PAL_AMB

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

// id
uint8_t u8ID = 0;

// application use
const uint8_t FOURCHARS[] = "PAB1";

bool b_transmit = false;
uint8_t u8txid = 0;

/*** Local function prototypes */
void sleepNow();
void startSensorCapture();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	// use PAL_AMB board support.
	auto&& brd = the_twelite.board.use<PAL_AMB>();
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
	Wire.begin(); // start two wire serial bus.
	Analogue.begin(pack_bits(PIN_ANALOGUE::A1, PIN_ANALOGUE::VCC)); // _start continuous adc capture.

	the_twelite.begin(); // start twelite!

	startSensorCapture();

	/*** INIT message */
	Serial << "--- PAL_AMB:" << FOURCHARS << " ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<PAL_AMB>();

	// mostly process every ms.
	if (TickTimer.available()) {
		
		//  wait until sensor capture finish
		if (!brd.sns_LTR308ALS.available()) {
			// this will take around 50ms.
			// note: to save battery life, perform sleeping to wait finish of sensor capture.
			brd.sns_LTR308ALS.process_ev(E_EVENT_TICK_TIMER);
		}

		if (!brd.sns_SHTC3.available()) {
			brd.sns_SHTC3.process_ev(E_EVENT_TICK_TIMER);
		}

		// now sensor data is ready.
		if (brd.sns_LTR308ALS.available() && brd.sns_SHTC3.available() && !b_transmit) {
			Serial << "..finish sensor capture." << mwx::crlf
				<< "  LTR308ALS: lumi=" << int(brd.sns_LTR308ALS.get_luminance()) << mwx::crlf
				<< "  SHTC3    : temp=" << brd.sns_SHTC3.get_temp() << 'C' << mwx::crlf
				<< "             humd=" << brd.sns_SHTC3.get_humid() << '%' << mwx::crlf
				<< mwx::flush;

			 // get new packet instance.
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, uint32_t(brd.sns_LTR308ALS.get_luminance()) // luminance
					, uint16_t(brd.sns_SHTC3.get_temp())
					, uint16_t(brd.sns_SHTC3.get_humid())
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
		}
	}

	// wait to complete transmission.
	if (b_transmit) {
		if (the_twelite.tx_status.is_complete(u8txid)) {		
			Serial << "..transmit complete." << mwx::crlf << mwx::flush;

			// now sleeping
			sleepNow();
		}
	}
}

// kick sensor capturing.
void startSensorCapture() {
	auto&& brd = the_twelite.board.use<PAL_AMB>();

	// start sensor capture
	brd.sns_SHTC3.begin();
	brd.sns_LTR308ALS.begin();
	b_transmit = false;
}

// perform sleeping
void sleepNow() {
	uint32_t u32ct = 1750 + random(0,500);
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf << mwx::flush;

	the_twelite.sleep(u32ct);
}

// wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- PAL_AMB:" << FOURCHARS << " wake up ---"
			<< mwx::crlf
			<< "..start sensor capture again."
			<< mwx::crlf;
	startSensorCapture();
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */