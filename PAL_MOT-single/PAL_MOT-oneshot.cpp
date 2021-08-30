// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_MOT>
#include <SM_SIMPLE>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

/*** state machine */
enum class E_STATE : uint8_t {
	INIT = 0,
	START_CAPTURE,
	WAIT_CAPTURE,
	REQUEST_TX,
	WAIT_TX,
	EXIT_NORMAL,
	EXIT_FATAL
};
SM_SIMPLE<E_STATE> step;

/*** function prototype */
void sleepNow();
MWX_APIRET TxReq();

/*** application use */
const uint8_t FOURCHARS[] = "PMT2";
uint32_t u32tick_capture; // tick when sensor capture started.
uint32_t u32tick_tx; // tick when tx req has been placed.
MWX_APIRET txid; // transmit request has been issued or not.
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

	/*** State Machine */
	step.setup();

	/*** INIT message */
	Serial << crlf << "--- PAL_MOT(OneShot):" << FOURCHARS << " ---" << crlf;
}

/*** the first call after finishing setup() */
void begin() { 
	// sleep immediately, waiting for the first capture.
	sleepNow();
}

/*** when waking up */
void wakeup() {
	Serial << crlf << "--- PAL_MOT(OneShot):" << FOURCHARS << " wake up ---" << crlf;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<PAL_MOT>();

	do {
		// if (TickTimer.available()) Serial << '.';
		switch(step.state()) {
			case E_STATE::INIT:
				brd.sns_MC3630.get_que().clear(); // clear queue in advance (just in case).
				step.next(E_STATE::START_CAPTURE);
			break;

			case E_STATE::START_CAPTURE:
				u32tick_capture = millis();
				brd.sns_MC3630.begin(
					// 400Hz, +/-4G range, get four samples (can be one sample)
					SnsMC3630::Settings(
						SnsMC3630::MODE_LP_400HZ, SnsMC3630::RANGE_PLUS_MINUS_4G, 4)); 

				step.set_timeout(100);
				step.next(E_STATE::WAIT_CAPTURE);
			break;

			case E_STATE::WAIT_CAPTURE:
				if (brd.sns_MC3630.available()) {
					brd.sns_MC3630.end(); // stop now!
					step.next(E_STATE::REQUEST_TX);
				} else if (step.is_timeout()) {
					Serial << crlf << "!!!FATAL: SENSOR CAPTURE TIMEOUT.";
					step.next(E_STATE::EXIT_FATAL);
				}
			break;

			case E_STATE::REQUEST_TX:
				txid = TxReq();
				if (txid) {
					step.set_timeout(100);
					step.clear_flag();
					step.next(E_STATE::WAIT_TX);
				} else {
					Serial << crlf << "!!!FATAL: TX REQUEST FAILS.";
					step.next(E_STATE::EXIT_FATAL);
				}
			break;

			case E_STATE::WAIT_TX:
				if (step.is_flag_ready()) {
					step.next(E_STATE::EXIT_NORMAL);
				}
				if (step.is_timeout()) {
					Serial << crlf << "!!!FATAL: TX TIMEOUT.";
					step.next(E_STATE::EXIT_FATAL);
				}
			break;

			case E_STATE::EXIT_NORMAL:
				sleepNow();
			break;

			case E_STATE::EXIT_FATAL:
				Serial << flush;
				the_twelite.reset_system();
			break;
		}
	} while(step.b_more_loop());
}

// when finishing data transmit, set the flag.
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	step.set_flag(ev.bStatus);
}

MWX_APIRET TxReq() {
	auto&& brd = the_twelite.board.use<PAL_MOT>();
	MWX_APIRET ret = false;

	Serial << "..finish sensor capture." << crlf
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
		ret = pkt.transmit();

		if (ret) {
			Serial << "..txreq(" << int(ret.get_value()) << ')';
		}
	}

	return ret;
}

void sleepNow() {
	Serial << crlf << "..sleeping now.." << crlf;
	Serial.flush();
	step.on_sleep(false); // reset state machine.
	the_twelite.sleep(3000, false); // set longer sleep (PAL must wakeup less than 60sec.)
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */