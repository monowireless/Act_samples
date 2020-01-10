// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_MOT>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

/*** function prototype */
void sleepNow();

/*** application use */
const uint8_t FOURCHARS[] = "PMT2";

bool b_transmit; // transmit request has been issued or not.
uint16_t txid; // transmit packet ID.
const uint8_t MAX_SAMP_IN_PKT = 15; // max samples count in one packet.

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */

	// board
	auto&& brd = the_twelite.board.use<PAL_MOT>();
	brd.set_led(LED_TIMER::BLINK, 100);

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(CHANNEL);// set channel (pysical channel)
		
	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk	<< NWK_SIMPLE::logical_id(0xFE); // set Logical ID. (0xFE means a child device with no ID)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- PAL_MOT(OneShot):" << FOURCHARS << " ---" << mwx::crlf;
}

/*** the first call after finishing setup() */
void begin() { 
	// sleep immediately, waiting for the first capture.
	sleepNow();
}

/*** when waking up */
void wakeup() {
	Serial << mwx::crlf << "--- PAL_MOT(OneShot):" << FOURCHARS << " wake up ---" << mwx::crlf;
	auto&& brd = the_twelite.board.use<PAL_MOT>();

	brd.sns_MC3630.get_que().clear(); // clear queue in advance (just in case).
	brd.sns_MC3630.begin(SnsMC3630::Settings(
			SnsMC3630::MODE_LP_400HZ, SnsMC3630::RANGE_PLUS_MINUS_4G, 4)); 
				// 400Hz, +/-4G range, get four samples (can be one sample)

	b_transmit = false;
	txid = 0xFFFF;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<PAL_MOT>();

	if (!b_transmit) {
		if (brd.sns_MC3630.available()) {
			brd.sns_MC3630.end(); // stop now!

			Serial << "..finish sensor capture." << mwx::crlf
				<< "  ct=" << int(brd.sns_MC3630.get_que().size());

			// get all samples and average them.
			int32_t x = 0, y = 0, z = 0;
			for (auto&& v: brd.sns_MC3630.get_que()) {
				x += v.x;
				y += v.y;
				z += v.z;
			}
			x /= brd.sns_MC3630.get_que().size();
			y /= brd.sns_MC3630.get_que().size();
			z /= brd.sns_MC3630.get_que().size();

			Serial << format("/ave=%d,%d,%d", x, y, z) << mwx::crlf;

			// just see X axis, min and max
			//auto&& x_axis = get_axis_x(brd.sns_MC3630.get_que());
			//auto&& x_minmax = std::minmax_element(x_axis.begin(), x_axis.end());
			auto&& x_minmax = std::minmax_element(
				get_axis_x_iter(brd.sns_MC3630.get_que().begin()),
				get_axis_x_iter(brd.sns_MC3630.get_que().end()));

			brd.sns_MC3630.get_que().clear(); // clean up the queue

			// prepare tx packet
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {		
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay
				
				// prepare packet (first)
				pack_bytes(pkt.get_payload() // set payload data objects.
						, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
						, uint16_t(x)
						, uint16_t(y)
						, uint16_t(z)
						, uint16_t(*x_minmax.first)  // minimum of captured x
						, uint16_t(*x_minmax.second) // maximum of captured x
					);

				// perform transmit
				MWX_APIRET ret = pkt.transmit();

				if (ret) {
					Serial << "..txreq(" << int(ret.get_value()) << ')';
					txid = ret.get_value() & 0xFF;
				} else {
					sleepNow();
				}
				
				// finished tx request
				b_transmit = true;
			}
		}
	} else {
		// wait until transmit completion.
		if(the_twelite.tx_status.is_complete(txid)) {
			sleepNow();
		}
	}
}

void sleepNow() {
	Serial << mwx::crlf << "..sleeping now.." << mwx::crlf << mwx::flush;
	the_twelite.sleep(3000, false); // set longer sleep (PAL must wakeup less than 60sec.)
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */