// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <BRD_APPTWELITE>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

/*** function prototype */
MWX_APIRET transmit();
void receive();

/*** application defs */
const char APP_FOURCHAR[] = "BAT1";
uint8_t u8devid = 0;

uint16_t au16AI[5];
uint8_t u8DI_BM;

/*** setup procedure (run once at cold boot) */
void setup() {
	// init vars
	for(auto&& x : au16AI) x = 0xFFFF;
	u8DI_BM = 0xFF;

	/*** SETUP section */
	auto&& brd = the_twelite.board.use<BRD_APPTWELITE>();

	// check DIP sw settings
	u8devid = (brd.get_M1()) ? 0x00 : 0xFE;

	// setup analogue
	Analogue.setup(true, ANALOGUE::KICK_BY_TIMER0); // setup analogue read (check every 16ms)

	// setup buttons
	Buttons.setup(5); // init button manager with 5 history table.

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	auto&& nwksmpl = the_twelite.network.use<NWK_SIMPLE>();
	nwksmpl << NWK_SIMPLE::logical_id(u8devid); // set Logical ID. (0x00 means parent device)

	/*** BEGIN section */
	// start ADC capture
	Analogue.begin(pack_bits(
						BRD_APPTWELITE::PIN_AI1,
						BRD_APPTWELITE::PIN_AI2,
						BRD_APPTWELITE::PIN_AI3,
						BRD_APPTWELITE::PIN_AI4,
				   		PIN_ANALOGUE::VCC)); // _start continuous adc capture.

	// Timer setup
	Timer0.begin(32, true); // 32hz timer

	// start button check
	Buttons.begin(pack_bits(
						BRD_APPTWELITE::PIN_DI1,
						BRD_APPTWELITE::PIN_DI2,
						BRD_APPTWELITE::PIN_DI3,
						BRD_APPTWELITE::PIN_DI4),
					5, 		// history count
					4);  	// tick delta (change is detected by 5*4=20ms consequtive same values)	


	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- BRD_APPTWELITE(" << int(u8devid) << ") ---" << mwx::crlf;
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

	// receive RF packet.
    if (the_twelite.receiver.available()) {
		receive();
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
		pkt << tx_addr(u8devid == 0 ? 0xFE : 0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
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

void receive() {	
	auto&& rx = the_twelite.receiver.read();
	
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

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */