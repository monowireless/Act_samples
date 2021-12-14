// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <ARIA> // include the board support of TWELITE ARIA
#include <STG_STD>
#include <SM_SIMPLE>

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
const char FOURCHARS[] = "ARA1";

// stores sensor value
struct {
	int16_t i16temp;
	int16_t i16humid;
	uint8_t b_north;
	uint8_t b_south;
} sensor;

// application state defs
enum class STATE : uint8_t {
	INTERACTIVE = 255,
	INIT = 0,
	SENSOR,
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
	step.setup(); // initialize state machine
	
	/// load board and settings objects
	auto&& brd = the_twelite.board.use<ARIA>(); // load board support
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	/// configure settings
	// configure settings
	set << SETTINGS::appname("ARIA");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);

	// if SET=LOW is detected, start with intaractive mode.
	if (digitalRead(brd.PIN_SET) == PIN_STATE::LOW) {
		set << SETTINGS::open_at_start();
		step.next(STATE::INTERACTIVE);
		return;
	}

	// load values
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	
	LID = set.u8devid(); // set logical ID

	/// configure system basics
	the_twelite << set; // apply settings (from interactive mode)

	/// configure network
	nwk << set; // apply settings (from interactive mode)
	nwk << NWK_SIMPLE::logical_id(LID); // set LID again (LID can also be configured by DIP-SW.)	

	/// configure hardware
	// LED setup (use periph_led_timer, which will re-start on wakeup() automatically)
	brd.set_led(LED_TIMER::BLINK, 10); // blink (on 10ms/ off 10ms)
	
	// let the TWELITE begin!
	the_twelite.begin();

	/*** INIT message */
	Serial << "--- ARIA:" << FOURCHARS << " ---" << mwx::crlf;
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

// wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- ARIA:" << FOURCHARS << " wake up ";

    if (the_twelite.is_wokeup_by_wktimer()) {
        Serial << "(WakeTimer) ---";
    } else 
    if (the_twelite.is_wokeup_by_dio(ARIA::PIN_SNS_NORTH)) {
        Serial << "(MAGnet INT [N]) ---";
    } else 
    if (the_twelite.is_wokeup_by_dio(ARIA::PIN_SNS_SOUTH)) {
        Serial << "(MAGnet INT [S]) ---";
    } else {
        Serial << "(unknown source) ---";
    }

	Serial  << mwx::crlf
			<< "..start sensor capture again."
			<< mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<ARIA>();

	do {
		switch (step.state()) {
		case STATE::INTERACTIVE:
		break;
		
		case STATE::INIT: // starting state
			// start sensor capture
			brd.sns_SHT4x.begin();

			step.next(STATE::SENSOR);
		break;

		case STATE::SENSOR: // starting state
			//  wait until sensor capture finish
			if (!brd.sns_SHT4x.available()) {
				brd.sns_SHT4x.process_ev(E_EVENT_TICK_TIMER);
			}else{ // now sensor data is ready.
				sensor.i16temp = brd.sns_SHT4x.get_temp_cent();
				sensor.i16humid = brd.sns_SHT4x.get_humid_per_dmil();

				// read magnet sensor
				sensor.b_north = digitalRead(ARIA::PIN_SNS_NORTH);
				sensor.b_south = digitalRead(ARIA::PIN_SNS_SOUTH);

				Serial << "..finish sensor capture." << mwx::crlf
					<< "  MAGnet   : north=" << int(sensor.b_north) << mwx::crlf
					<< "             south=" << int(sensor.b_south) << mwx::crlf
					<< "  SHT4x    : temp=" << div100(sensor.i16temp) << 'C' << mwx::crlf
					<< "             humd=" << div100(sensor.i16humid) << '%' << mwx::crlf
					;
				Serial.flush();

				step.next(STATE::TX);
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
					, uint8_t(sensor.b_north)
					, uint8_t(sensor.b_south)
					, uint16_t(sensor.i16temp)
					, uint16_t(sensor.i16humid)
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf << mwx::flush;

				if (ret) {
					step.clear_flag(); // waiting for flag is set.
					step.set_timeout(100); // set timeout
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

// perform sleeping
void sleepNow() {
	step.on_sleep(false); // reset state machine.

	// randomize sleep duration.
	uint32_t u32ct = 1750 + random(0,500);

	// set an interrupt for MAGnet sensor.
	pinMode(ARIA::PIN_SNS_OUT1, PIN_MODE::WAKE_FALLING);
	pinMode(ARIA::PIN_SNS_OUT2, PIN_MODE::WAKE_FALLING);

	// output message
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush(); // wait until all message printed.
	
	// do sleep.
	the_twelite.sleep(u32ct);
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */