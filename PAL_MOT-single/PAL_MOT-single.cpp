// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <SM_SIMPLE>

#include <STG_STD>

/*** board selection (choose one) */
#define USE_PAL_MOT
//#define USE_CUE
// board dependend definitions.
#if defined(USE_PAL_MOT)
#define BRDN PAL_MOT
#define BRDC <PAL_MOT>
#elif defined(USE_CUE)
#define BRDN CUE
#define BRDC <CUE>
#endif
// include board support
#include BRDC

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// option bits
uint32_t OPT_BITS = 0;

// stores sensor value
struct {
	int32_t x_ave, y_ave, z_ave;
	int32_t x_min, y_min, z_min;
	int32_t x_max, y_max, z_max;
	uint16_t n_seq;
	uint8_t n_samples;
} sensor;

/*** state machine */
enum class E_STATE : uint8_t {
	INTERACTIVE = 255,
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
const uint8_t N_SAMPLES = 4;

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	// board
	auto&& brd = the_twelite.board.use<PAL_MOT>();
	brd.set_led(LED_TIMER::BLINK, 100);

	// the twelite main class
	the_twelite
		<< TWENET::appid(DEFAULT_APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(DEFAULT_CHANNEL);// set channel (pysical channel)
		
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
				memset(&sensor, 0, sizeof(sensor)); // clear sensor data
				step.next(E_STATE::START_CAPTURE);
			break;

			case E_STATE::START_CAPTURE:
				brd.sns_MC3630.begin(
					// 400Hz, +/-4G range, get four samples and will average them.
					SnsMC3630::Settings(
						SnsMC3630::MODE_LP_400HZ, SnsMC3630::RANGE_PLUS_MINUS_4G, N_SAMPLES)); 

				step.set_timeout(100);
				step.next(E_STATE::WAIT_CAPTURE);
			break;

			case E_STATE::WAIT_CAPTURE:
				if (brd.sns_MC3630.available()) {
					brd.sns_MC3630.end(); // stop now!

					sensor.n_samples = brd.sns_MC3630.get_que().size();
					if (sensor.n_samples) sensor.n_seq = brd.sns_MC3630.get_que()[0].get_t();
					else { 
						Serial << crlf << "!!!FATAL: NO SAMPLE.";
						step.next(E_STATE::EXIT_FATAL);
						break;
					}
					
					Serial << "..finish sensor capture ("
							<< "ct=" << int(sensor.n_samples)
							<< "/sq=" << int(sensor.n_seq)
							<< ')' << crlf
							;

					// get all samples and average them.
					for (auto&& v: brd.sns_MC3630.get_que()) {
						sensor.x_ave  += v.x;
						sensor.y_ave  += v.y;
						sensor.z_ave  += v.z;
					}

					if (sensor.n_samples == N_SAMPLES) {
						// if N_SAMPLES == 2^n, division is much faster.
						sensor.x_ave /= N_SAMPLES;
						sensor.y_ave /= N_SAMPLES;
						sensor.z_ave /= N_SAMPLES;
					} else {
						// should not be here, but leave this code.
						sensor.x_ave /= sensor.n_samples;
						sensor.y_ave /= sensor.n_samples;
						sensor.z_ave /= sensor.n_samples;
					}
					Serial << format("  ave=(%d,%d,%d)", sensor.x_ave, sensor.y_ave, sensor.z_ave);

					// just see min and max of X,Y,Z by std::minmax_element.
					// can also be:
					//	int32_t x_max = -999999, x_min = 999999;
					//	for (auto&& v: brd.sns_MC3630.get_que()) {
					//		if (v.x >= x_max) x_max = v.x;
					//		if (v.y <= x_min) x_min = v.x;
					//		...
					//	}	
					auto&& x_minmax = std::minmax_element(
						get_axis_x_iter(brd.sns_MC3630.get_que().begin()),
						get_axis_x_iter(brd.sns_MC3630.get_que().end()));
					sensor.x_min = *x_minmax.first;
					sensor.x_max = *x_minmax.second;

					auto&& y_minmax = std::minmax_element(
						get_axis_y_iter(brd.sns_MC3630.get_que().begin()),
						get_axis_y_iter(brd.sns_MC3630.get_que().end()));
					sensor.y_min = *y_minmax.first;
					sensor.y_max = *y_minmax.second;

					auto&& z_minmax = std::minmax_element(
						get_axis_z_iter(brd.sns_MC3630.get_que().begin()),
						get_axis_z_iter(brd.sns_MC3630.get_que().end()));
					sensor.z_min = *z_minmax.first;
					sensor.z_max = *z_minmax.second;

					Serial << format("/min=(%d,%d,%d)", sensor.x_min, sensor.y_min, sensor.z_min);
					Serial << format("/max=(%d,%d,%d)", sensor.x_max, sensor.y_max, sensor.z_max);
					Serial << crlf;

					brd.sns_MC3630.get_que().clear(); // clean up the queue

					step.next(E_STATE::REQUEST_TX); // next state
				} else if (step.is_timeout()) {
					Serial << crlf << "!!!FATAL: SENSOR CAPTURE TIMEOUT.";
					step.next(E_STATE::EXIT_FATAL);
				}
			break;

			case E_STATE::REQUEST_TX:
				if (TxReq()) {
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

	// prepare tx packet
	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {		
		// set tx packet behavior
		pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x1 send two times in total)
			<< tx_packet_delay(0, 0, 2); // send packet w/ delay
		
		// prepare packet (first)
		pack_bytes(pkt.get_payload() // set payload data objects.
				, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
				, uint16_t(sensor.n_seq)
				, uint8_t(sensor.n_samples)
				, uint16_t(sensor.x_ave)
				, uint16_t(sensor.y_ave)
				, uint16_t(sensor.z_ave)
				, uint16_t(sensor.x_min)
				, uint16_t(sensor.y_min)
				, uint16_t(sensor.z_min)
				, uint16_t(sensor.x_max)
				, uint16_t(sensor.y_max)
				, uint16_t(sensor.z_max)
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