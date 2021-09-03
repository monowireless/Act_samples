// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <BRD_APPTWELITE>
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

/*** function prototype */
MWX_APIRET transmit();
void receive();

/*** application defs */
const char APP_FOURCHAR[] = "BAT1";

// sensor values
uint16_t au16AI[5];
uint8_t u8DI_BM;

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */	
	// init vars
	for(auto&& x : au16AI) x = 0xFFFF;
	u8DI_BM = 0xFF;

	// load board and settings
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& brd = the_twelite.board.use<BRD_APPTWELITE>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	
	// settings: configure items
	set << SETTINGS::appname("BRD_APPTWELITE");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	LID = set.u8devid(); // logical ID

	// the twelite main class
	the_twelite
		<< set                      // apply settings (appid, ch, power)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	if (brd.get_M1()) { LID = 0; }

	// Register Network
	nwk << set // apply settings (LID and retry)
			;

	// if M1 pin is set, force parent device (LID=0)
	nwk << NWK_SIMPLE::logical_id(LID); // write logical id again.
	
	/*** BEGIN section */
	// start ADC capture
	Analogue.setup(true, ANALOGUE::KICK_BY_TIMER0); // setup analogue read (check every 16ms)
	Analogue.begin(pack_bits(
						BRD_APPTWELITE::PIN_AI1,
						BRD_APPTWELITE::PIN_AI2,
						BRD_APPTWELITE::PIN_AI3,
						BRD_APPTWELITE::PIN_AI4,
				   		PIN_ANALOGUE::VCC)); // _start continuous adc capture.

	// Timer setup
	Timer0.begin(32, true); // 32hz timer

	// start button check
	Buttons.setup(5); // init button manager with 5 history table.
	Buttons.begin(pack_bits(
						BRD_APPTWELITE::PIN_DI1,
						BRD_APPTWELITE::PIN_DI2,
						BRD_APPTWELITE::PIN_DI3,
						BRD_APPTWELITE::PIN_DI4),
					5, 		// history count
					4);  	// tick delta (change is detected by 5*4=20ms consequtive same values)	


	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial 	<< "--- BRD_APPTWELITE ---" << mwx::crlf;
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

/*** loop procedure (called every event) */
void loop() {
	if (Buttons.available()) {

		uint32_t bp, bc;
		Buttons.read(bp, bc);

		u8DI_BM = uint8_t(collect_bits(bp, 
							BRD_APPTWELITE::PIN_DI4,   // bit3
							BRD_APPTWELITE::PIN_DI3,   // bit2
							BRD_APPTWELITE::PIN_DI2,   // bit1
							BRD_APPTWELITE::PIN_DI1)); // bit0

		transmit();
	}

	if (Analogue.available()) {
		au16AI[0] = Analogue.read(PIN_ANALOGUE::VCC);
		au16AI[1] = Analogue.read_raw(BRD_APPTWELITE::PIN_AI1);
		au16AI[2] = Analogue.read_raw(BRD_APPTWELITE::PIN_AI2);
		au16AI[3] = Analogue.read_raw(BRD_APPTWELITE::PIN_AI3);
		au16AI[4] = Analogue.read_raw(BRD_APPTWELITE::PIN_AI4);
	}

	if (Timer0.available()) {
		static uint8_t u16ct;
		u16ct++;

		if (u8DI_BM != 0xFF && au16AI[0] != 0xFFFF) { // finished the first capture
			if ((u16ct % 32) == 0) { // every 32ticks of Timer0
				transmit();
			}
		}
	}
}

/*** transmit a packet */
MWX_APIRET transmit() {
	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		Serial << "..DI=" << format("%04b ", u8DI_BM);
		Serial << format("ADC=%04d/%04d/%04d/%04d ", au16AI[1], au16AI[2], au16AI[3], au16AI[4]);
		Serial << "Vcc=" << format("%04d ", au16AI[0]);
		Serial << " --> transmit" << mwx::crlf;

		// set tx packet behavior
		pkt << tx_addr(LID == 0 ? 0xFE : 0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x1 send two times in total)
			<< tx_packet_delay(0,50,10); // send packet w/ delay (send first packet with randomized delay from 100 to 200ms, repeat every 20ms)

		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair(APP_FOURCHAR, 4) // string should be paired with length explicitly.
			, uint8_t(u8DI_BM)
		);

		for (auto&& x : au16AI) {
			pack_bytes(pkt.get_payload(), uint16_t(x)); // adc values
		}
		
		// do transmit 
		return pkt.transmit();
	}
	return MWX_APIRET(false, 0);
}

void on_rx_packet(packet_rx& rx, bool_t &handled) {	
	Serial << format("..receive(%08x/%d) : ", rx.get_addr_src_long(), rx.get_addr_src_lid());
	
	// expand packet payload (shall match with sent packet data structure, see pack_bytes())
	char fourchars[5]{}; // init all elements as default (0).
	auto&& np = expand_bytes(rx.get_payload().begin(), rx.get_payload().end()
		, make_pair((uint8_t*)fourchars, 4)  // 4bytes of msg
    );

	// check header
	if (strncmp(APP_FOURCHAR, fourchars, 4)) { return; }

	// read rest of payload
	uint8_t u8DI_BM_remote = 0xff;
	uint16_t au16AI_remote[5];
	expand_bytes(np, rx.get_payload().end()
		, u8DI_BM_remote
		, au16AI_remote[0]
		, au16AI_remote[1]
		, au16AI_remote[2]
		, au16AI_remote[3]
		, au16AI_remote[4]
	);

	Serial << format("DI:%04b", u8DI_BM_remote & 0x0F);
	for (auto&& x : au16AI_remote) {
		Serial << format("/%04d", x);
	}
	Serial << mwx::crlf;
	
	// set local DO
	digitalWrite(BRD_APPTWELITE::PIN_DO1, (u8DI_BM_remote & 1) ? HIGH : LOW);
	digitalWrite(BRD_APPTWELITE::PIN_DO2, (u8DI_BM_remote & 2) ? HIGH : LOW);
	digitalWrite(BRD_APPTWELITE::PIN_DO3, (u8DI_BM_remote & 4) ? HIGH : LOW);
	digitalWrite(BRD_APPTWELITE::PIN_DO4, (u8DI_BM_remote & 8) ? HIGH : LOW);

	// set local PWM : duty is set 0..1024, so 1023 is set 1024.
	Timer1.change_duty(au16AI_remote[1] == 1023 ? 1024 : au16AI_remote[1]);
	Timer2.change_duty(au16AI_remote[2] == 1023 ? 1024 : au16AI_remote[2]);
	Timer3.change_duty(au16AI_remote[3] == 1023 ? 1024 : au16AI_remote[3]);
	Timer4.change_duty(au16AI_remote[4] == 1023 ? 1024 : au16AI_remote[4]);
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */