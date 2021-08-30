// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <SM_SIMPLE>
#include <STG_STD>

/*** board selection (choose one) */
#define USE_PAL_MAG
//#define USE_CUE
// board dependend definitions.
#if defined(USE_PAL_MAG)
#define BRDN PAL_MAG
#define BRDC <PAL_MAG>
#elif defined(USE_CUE)
#define BRDN CUE
#define BRDC <CUE>
#endif
#include BRDC // include board support, like <PAL_MAG>.

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
const uint8_t FOURCHARS[] = "PMG1";

// store sensor data.
struct SENSOR {
	uint8_t b_north;
	uint8_t b_south;
} sensor;

// application state defs
enum class STATE : uint8_t {
	INTERACTIVE = 255,
	INIT = 0,
	TX,
	TX_WAIT_COMP,
	GO_SLEEP
};

// simple state machine.
SM_SIMPLE<STATE> step;

/*** Local function prototypes */
void sleepNow();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	/// init vars or objects
	step.setup(); // init the state machine
	
	/// load board and settings objects
	auto&& brd = the_twelite.board.use BRDC(); // load board support
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	// settings: configure items
	set << SETTINGS::appname("MAG");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set << SETTINGS::lid_default(0x1); // set default LID
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	
	// if SET=LOW is detected, start with intaractive mode.
	if (digitalRead(brd.PIN_SET) == PIN_STATE::LOW) {
		set << SETTINGS::open_at_start();
		step.next(STATE::INTERACTIVE);
		return;
	}

	// load settings
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.

	// now it can read DIP sw status.
	#if defined(USE_PAL_MAG)
	LID = (brd.get_DIPSW_BM() & 0x07) + 1; // DIP-SW
	#endif
	if (LID == 0) LID = set.u8devid(); // Settings
	else if (LID == 0) LID = 0xFE; // 0xFE
	
	// the twelite main object.
	the_twelite
		<< set                    // apply settings (appid, ch, power)
		;

	// Register Network
	nwk << set;
	nwk << NWK_SIMPLE::logical_id(LID); // set Logical ID. (0xFE means a child device with no ID)

	// LED setup (use periph_led_timer, which will re-start on wakeup() automatically)
	brd.set_led(LED_TIMER::BLINK, 10); // blink (on 10ms/ off 10ms)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- MAG:" << FOURCHARS << " ---" << mwx::crlf;
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

/*** begin procedure the first call */
void begin() {
	auto&& set = the_twelite.settings.use<STG_STD>();
	if (!set.is_screen_opened()) {
		// sleep firstly...
		sleepNow();
	}
}

// wakeup procedure
void wakeup() {
	// if periodic waking up to avoid H/W WDT, sleep again.
	if (the_twelite.is_wokeup_by_wktimer()) {
		Serial << "..wk timer, sleep again.." << mwx::crlf;
		sleepNow();
	}

	// show message
	Serial	<< mwx::crlf
			<< "--- MAG:" << FOURCHARS << " wake up ---"
			<< mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	do {
		switch (step.state()) {
		case STATE::INTERACTIVE: // interactive mode
			break;

		case STATE::INIT: // starting state
			// capture sensor data
			sensor.b_north = the_twelite.is_wokeup_by_dio(BRDN::PIN_SNS_NORTH);
			sensor.b_south = the_twelite.is_wokeup_by_dio(BRDN::PIN_SNS_SOUTH);
			
			Serial << "..sensor north=" << int(sensor.b_north) 
			       << " south=" << int(sensor.b_south) << mwx::crlf;

			step.next(STATE::TX);
		break;

		case STATE::TX: // try transmit a packet
			step.next(STATE::GO_SLEEP); // set default next state (for error handling.)

			// transmit request
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, sensor.b_north
					, sensor.b_south
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf;

				if (ret) {
					step.clear_flag(); // waiting for flag is set.
					step.set_timeout(100); // setting timeout.
					step.next(STATE::TX_WAIT_COMP);
				}
			}
		break;

		case STATE::TX_WAIT_COMP: // wait for complete of transmit
			if (step.is_timeout()) { // maybe fatal error.
				the_twelite.reset_system();
			}
			if (step.is_flag_ready()) { // when tx is performed
				Serial << "..transmit complete." << mwx::crlf;
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
	step.set_flag();
}

// perform sleeping
void sleepNow() {
	uint32_t u32ct = 60000; // PAL must wakeup every 60secs.
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush();

	pinMode(BRDN::PIN_SNS_OUT1, PIN_MODE::WAKE_FALLING);
	pinMode(BRDN::PIN_SNS_OUT2, PIN_MODE::WAKE_FALLING);

	step.on_sleep(false); // reset state machine before sleeping.
	the_twelite.sleep(u32ct); // do sleep
}


/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */