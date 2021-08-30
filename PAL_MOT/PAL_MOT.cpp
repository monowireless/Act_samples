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
#define BRDN PAL_MAG
#define BRDC <PAL_MAG>
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

/*** function prototype */
void sleepNow();

/*** application use */
const uint8_t FOURCHARS[] = "PMT1";

// stores sensor value
struct {
	int32_t x_ave, y_ave, z_ave;
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

/// FIFO sample count consideration.
// 1. The maximum payload size accepts 16packets=80bytes.
// 2. The FIFO has 32samples and it's necessary to grab samples before reaching 32 samples.
// One of good conditions is:
//  - 15samples/packet
//  - wakeup on 30 samples 
const uint8_t MAX_SAMP_IN_PKT = 15;
const uint8_t FIFO_SAMPLE_COUNT = 30; // FIFO count
const uint8_t N_TX_PACKETS = (FIFO_SAMPLE_COUNT - 1) / MAX_SAMP_IN_PKT + 1;

// txid control
uint16_t txid[N_TX_PACKETS];
const uint16_t TXID_VOID = 0x7FFF; // not requested.
const uint16_t TXID_COMP_SUCCESS = 0x8100;
const uint16_t TXID_COMP_FAIL = 0x8000;
const uint16_t TXID_COMP_MASK = 0x8000;
const uint16_t TXID_DEFERRED_MASK = 0x00FF; // tx request is placed, but not finished
inline bool IS_TXID_VOID(uint16_t id) { return TXID_VOID == id; }
inline bool IS_TXID_COMP_SUCCESS(uint16_t id) { return (id & 0xFF00) == TXID_COMP_SUCCESS; }
inline bool IS_TXID_COMP_FAILED(uint16_t id) { return (id & 0xFF00) == TXID_COMP_FAIL; }
inline bool IS_TXID_COMP(uint16_t id) { return (id & TXID_COMP_MASK); }
inline bool IS_TXID_DEFFERED(uint16_t id) { return (id <= TXID_DEFERRED_MASK); }

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	/// init vars or objects
	step.setup(); // ステートマシンの初期化	
	
	/// load board and settings objects
	auto&& brd = the_twelite.board.use BRDC (); // load board support
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	// settings: configure items
	set << SETTINGS::appname("MOT");
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
	
	// the twelite main object.
	the_twelite
		<< set                    // apply settings (appid, ch, power)
		;

	// Register Network
	nwk << set;
	
	/*** HARDWARE setup section */
	brd.set_led(LED_TIMER::BLINK, 100);

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!
	brd.sns_MC3630.begin(SnsMC3630::Settings(
		SnsMC3630::MODE_LP_14HZ
		, SnsMC3630::RANGE_PLUS_MINUS_4G)
		, FIFO_SAMPLE_COUNT// Interrupt comes every 28samples
		); // begin MC3630

	/*** INIT message */
	Serial << "--- MOT:" << FOURCHARS << " ---" << mwx::crlf;
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

/*** the first call after finishing setup() */
void begin() { 
	auto&& set = the_twelite.settings.use<STG_STD>();
	if (!set.is_screen_opened()) {
		// sleep firstly...
		sleepNow();
	}
}

/*** when waking up */
void wakeup() {
	Serial << "--- PAL_MOT(Cont):" << FOURCHARS << " wake up ---" << mwx::crlf;

	// clear TX id
	txid[0] = TXID_VOID;
	txid[1] = TXID_VOID;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use BRDC ();

	do {
		switch (step.state()) {
		case STATE::INTERACTIVE:
		break;

		case STATE::INIT: // starting state
			if (!brd.sns_MC3630.available()) {
				Serial << "..sensor is not available." << mwx::crlf << mwx::flush;
				sleepNow();
			} else {
				step.next(STATE::SENSOR);
			}
		break;
		
		case STATE::SENSOR:
			// send a packet
			Serial << "..finish sensor capture." << mwx::crlf
				<< "  seq=" << int(brd.sns_MC3630.get_que().back().t) 
				<< "/ct=" << int(brd.sns_MC3630.get_que().size());

			// calc average in the queue.
			{
				// read sensor data in the queue and calculate average.
				// note: these average values are not used for tx packet in this example.
				sensor.x_ave = 0;
				sensor.y_ave = 0;
				sensor.z_ave = 0;

				// this operation does not pop data from the queue, buy just to read them.
				// note: in order to proceed to next sensor capture, all data should be pop'ed.
				for (auto&& v: brd.sns_MC3630.get_que()) {
					sensor.x_ave += v.x;
					sensor.y_ave += v.y;
					sensor.z_ave += v.z;
				}
				sensor.x_ave /= brd.sns_MC3630.get_que().size();
				sensor.y_ave /= brd.sns_MC3630.get_que().size();
				sensor.z_ave /= brd.sns_MC3630.get_que().size();

				Serial << format("/ave=%d,%d,%d", sensor.x_ave, sensor.y_ave, sensor.z_ave) << mwx::crlf;
			}

			step.next(STATE::TX);
		break;

		case STATE::TX: // try transmit a packet
			step.next(STATE::GO_SLEEP); // set default next state (for error handling.)
			
			// Preapre tx packets and transmit them.
			// note: coming samples are larger and it should be splitted into two packets.
			// note: tx requests can be put at once until the internal queue is full.
			for (int ip = 0; ip < N_TX_PACKETS; ip++) {
				if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {		
					// set tx packet behavior
					pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
						<< tx_retry(0x1) // set retry (0x1 send two times in total)
						<< tx_packet_delay(0, 0, 2); // send packet w/ delay
				
					// prepare packet (first)
					uint8_t siz = (brd.sns_MC3630.get_que().size() >= MAX_SAMP_IN_PKT)
										? MAX_SAMP_IN_PKT : brd.sns_MC3630.get_que().size();
					uint16_t seq = brd.sns_MC3630.get_que().front().t;
				
					pack_bytes(pkt.get_payload() // set payload data objects.
						, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
						, seq // add sequence number of capture data of the first entry.
						, siz // count of entry in the packet.
					);

					// store sensor data (36bits into 5byts, alas 4bits are not used...)
					for (int i = 0; i < siz; i++) {
						auto&& v = brd.sns_MC3630.get_que().front();
						uint32_t v1;

						// RANGE_PLUS_MINUS_4G will have sensor value from -4000 to 4000,
						// so send them with 13bit's value (note: accracy is 12bits in FIFO mode).
						v1  = ((uint32_t(v.x) & 0x1FFFUL) << 19)  // X:13bits(signed)
							| ((uint32_t(v.y) & 0x1FFFUL) <<  6)  // Y:13bits(signed)
							| ((uint32_t(v.z) & 0x1FFFUL) >>  7); // Z:6bits from MSB(signed)
						uint8_t v2 = (uint32_t(v.z) & 0x7F);// Z:7bits from LSB

						pack_bytes(pkt.get_payload(), v1, v2); // add into pacekt entry.
						brd.sns_MC3630.get_que().pop(); // pop an entry from queue.
					}

					// perform transmit
					MWX_APIRET ret = pkt.transmit();

					if (ret) {
						Serial << "..txreq(" << int(ret.get_value()) << ')';
						txid[ip] = ret.get_value() & 0xFF;
					} else {
						txid[ip] = TXID_COMP_FAIL;
					}
				}
			}
			
			// tx requests are placed successfully.
			// note: it proceed even if one of requests is failed.
			{
				unsigned int n = 0;
				for(unsigned int ip = 0; ip < N_TX_PACKETS; ip++) {
					if (IS_TXID_DEFFERED(txid[ip])) n++;
				}

				if (n) {
					step.clear_flag();
					step.set_timeout(100*n); // waiting for flag is set.
					step.next(STATE::TX_WAIT_COMP);
				}
			}
		break;

		case STATE::TX_WAIT_COMP: // wait for complete of transmit
			if (step.is_timeout()) { // maybe fatal error.
				the_twelite.reset_system();
			}
			if (step.is_flag_ready()) { // when tx is completed.
				Serial << "..transmit complete." << mwx::crlf;
				step.next(STATE::GO_SLEEP);
			}
		break;

		case STATE::GO_SLEEP:
			sleepNow();
		break;

		default: // never be here!
			the_twelite.reset_system();
		}
	} while(step.b_more_loop());
}

// when finishing data transmit, set the flag.
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	bool b_all_comp = true;
	for(int i = 0; i < N_TX_PACKETS; i++) {
		if (IS_TXID_VOID(txid[i])) continue; 
		if (ev.u8CbId == txid[i]) txid[i] = TXID_COMP_SUCCESS + ev.u8CbId;
		if (!IS_TXID_COMP(txid[i])) b_all_comp = false;
	}

	// both are completed
	if (b_all_comp) step.set_flag(ev.bStatus);
}

void sleepNow() {
	Serial << mwx::crlf << "..sleeping now.." << mwx::crlf;
	Serial.flush();

	pinMode(BRDN::PIN_SNS_INT, WAKE_FALLING); // set interupt pin activated by FIFO accumulation.

	step.on_sleep(false); // reset state machine.
	the_twelite.sleep(60000, false); // set longer sleep (PAL must wakeup every 60sec, just in the case)
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */