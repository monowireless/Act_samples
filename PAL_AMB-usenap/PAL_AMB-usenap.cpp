// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_AMB> // include the board support of PAL_AMB
#include <SM_SIMPLE>
#include <STG_STD>

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// option bits
uint32_t OPT_BITS = 0;
// logical id
uint8_t LID = 0;

// application use
const uint8_t FOURCHARS[] = "PAB2";

// stores sensor value
struct {
	uint32_t u32luminance;
	int16_t i16temp;
	int16_t i16humid;
} sensor;

// application state defs
enum class STATE : uint8_t {
	INTERACTIVE = 255,
	INIT = 0,
	NAP,
	SENSOR,
	TX,
	TX_WAIT_COMP,
	GO_SLEEP
};

// simple state machine.
SM_SIMPLE<STATE> step;

/*** Local function prototypes */
void sleepNow();
void napNow();
void startSensorCapture();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	/// init vars or objects
	step.setup(); // initialize state machine
	
	/// load board and settings objects
	auto&& brd = the_twelite.board.use<PAL_AMB>(); // load board support
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	/// configure settings
	// configure settings
	set << SETTINGS::appname("PAL_AMB");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);

	// if SET=LOW is detected, start with intaractive mode.
	if (digitalRead(brd.PIN_BTN) == PIN_STATE::LOW) {
		set << SETTINGS::open_at_start();
		step.next(STATE::INTERACTIVE);
		return;
	}

	// load values
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	
	// LID is configured DIP or settings.
	LID = (brd.get_DIPSW_BM() & 0x07); // 1st priority is DIP SW
	if (LID == 0) LID = set.u8devid(); // 2nd is setting.
	if (LID == 0) LID = 0xFE; // if still 0, set 0xFE (anonymous child)

	/// configure system basics
	the_twelite << set; // apply settings (from interactive mode)

	/// configure network
	nwk << set; // apply settings (from interactive mode)
	nwk << NWK_SIMPLE::logical_id(LID); // set LID again (LID can also be configured by DIP-SW.)	

	/// configure hardware
	// LED setup (use periph_led_timer, which will re-start on wakeup() automatically)
	brd.set_led(LED_TIMER::BLINK, 10); // blink (on 10ms/ off 10ms)
	
	/*** BEGIN section */
	Wire.begin(); // start two wire serial bus.
	Analogue.begin(pack_bits(PIN_ANALOGUE::A1, PIN_ANALOGUE::VCC)); // _start continuous adc capture.
	
	// let the TWELITE begin!
	the_twelite.begin();

	/*** INIT message */
	Serial << "--- PAL_AMB:" << FOURCHARS << " ---" << mwx::crlf;
	Serial	<< format("-- app:x%08x/ch:%d/lid:%d"
					, the_twelite.get_appid()
					, the_twelite.get_channel()
					, nwk.get_config().u8Lid
				)
			<< mwx::crlf;
	Serial 	<< format("-- pw:%d/retry:%d/opt:x%08x"
					, the_twelite.get_tx_power()
					, nwk.get_config().u8RetryDefault
					, OPT_BITS
			)
			<< mwx::crlf;
}

/*** begin procedure (called the first time only) */
void begin() {
	auto&& set = the_twelite.settings.use<STG_STD>();
	if (!set.is_screen_opened()) {
		// sleep firstly...
		sleepNow();
	}
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<PAL_AMB>();

	do {
		switch (step.state()) {
		case STATE::INTERACTIVE:
		break;

		case STATE::INIT: // starting state
			// start sensor capture
			brd.sns_SHTC3.begin();
			brd.sns_LTR308ALS.begin();

			step.next(STATE::NAP);
			napNow();
		break;

		case STATE::NAP:
			// tell sensors waking up.
			brd.sns_LTR308ALS.process_ev(E_EVENT_START_UP);
			brd.sns_SHTC3.process_ev(E_EVENT_START_UP);
			
			step.next(STATE::SENSOR);
			break;

		case STATE::SENSOR: // starting state
			// now sensor data should be ready.
			if (brd.sns_LTR308ALS.available() && brd.sns_SHTC3.available()) {
				sensor.u32luminance = brd.sns_LTR308ALS.get_luminance();
				sensor.i16temp = brd.sns_SHTC3.get_temp_cent();
				sensor.i16humid = brd.sns_SHTC3.get_humid_per_dmil();

				Serial << "..finish sensor capture." << mwx::crlf
					<< "  LTR308ALS: lumi=" << int(sensor.u32luminance) << mwx::crlf
					<< "  SHTC3    : temp=" << div100(sensor.i16temp) << 'C' << mwx::crlf
					<< "             humd=" << div100(sensor.i16humid) << '%' << mwx::crlf
					;

				step.next(STATE::TX);
			} else {
				// this cannot be.
				the_twelite.reset_system();
			}
		break;

		case STATE::TX:
			step.next(STATE::GO_SLEEP); // set default next state (for error handling.)

			// get new packet instance.
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, uint32_t(sensor.u32luminance) // luminance
					, uint16_t(sensor.i16temp)
					, uint16_t(sensor.i16humid)
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf << mwx::flush;

				if (ret) {
					step.clear_flag(); // waiting for flag is set.
					step.set_timeout(100); // set timeout.
					step.next(STATE::TX_WAIT_COMP);
				}
			}
		break;

		case STATE::TX_WAIT_COMP: // wait for complete of transmit
			if (step.is_timeout()) { // maybe fatal error.
				the_twelite.reset_system();
			}
			if (step.is_flag_ready()) { // when tx is completed
				Serial << "..transmit complete." << mwx::crlf;
				Serial.flush();
				step.next(STATE::GO_SLEEP);
			}
		break;

		case STATE::GO_SLEEP: // now sleeping
			sleepNow();
		break;

		default: // never be here!
			the_twelite.reset_system();
		}
	} while(step.b_more_loop()); // if state is changed, loop more.
}

// when finishing data transmit, set the flag.
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	step.set_flag(ev.bStatus);
}

// wakeup procedure
void wakeup() {
	// restore state machine status.
	if (step.state() == STATE::INIT) {
		// delete/make shorter this message if power requirement is harder.	
		Serial	<< mwx::crlf
				<< "--- PAL_AMB:" << FOURCHARS << " wake up ---"
				<< mwx::crlf
				<< "..start sensor capture again."
				<< mwx::crlf;
	} else {
		Serial << "..wake up from short nap.." << mwx::crlf;
		the_twelite.start_pending_mac(); // start MAC layer
	}
}

// perform sleeping
void sleepNow() {
	// set sleep duration randomized by 500ms.
	uint32_t u32ct = 1750 + random(0,500);
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush();

	// reset state machine.
	step.on_sleep(false);

	// perform sleep	
	the_twelite.sleep(u32ct);
}

// perform short period sleep
void napNow() {
	uint32_t u32ct = 100;
	Serial << "..nap " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush();

	// resume state machine after waking up.
	step.on_sleep(true);

	// perform sleep (using seconde wk timer.)
	the_twelite.sleep(u32ct, false, false, TWENET::SLEEP_WAKETIMER_SECONDARY);
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */