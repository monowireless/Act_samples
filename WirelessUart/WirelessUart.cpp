// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <STG_STD>

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// channel
const uint8_t DEFAULT_LID = 1;

/*** function prototype */
MWX_APIRET transmit(uint8_t addr, const uint8_t* b, const uint8_t* e);

/*** application defs */
const uint8_t FOURCHARS[] = "WURT";

/*** setup procedure (run once at cold boot) */
void setup() {
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();

	/*** INTERACTIE MODE */
	// settings: configure items
	set << SETTINGS::appname("WirelessUART");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set << SETTINGS::lid_default(DEFAULT_LID); // set default lid
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.
	
	/*** SETUP section */
	// the twelite main class
	the_twelite
		<< set                      // from iteractive mode (APPID/CH/POWER)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	nwk	<< set;						// from interactive mode (LID/REPEAT)

	/*** BEGIN section */
	SerialParser.begin(PARSER::ASCII, 128); // Initialize the serial parser
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- WirelessUart (id=" << int(nwk.get_config().u8Lid) << ") ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
    // read from serial
	while(Serial.available())  {
		if (SerialParser.parse(Serial.read())) {
			Serial << ".." << SerialParser;
			const uint8_t* b = SerialParser.get_buf().begin();
			uint8_t addr = *b; ++b; // the first byte is destination address.
			transmit(addr, b, SerialParser.get_buf().end());
		}
	}
}

/** transmit a packet */
MWX_APIRET transmit(uint8_t dest, const uint8_t* b, const uint8_t* e) {
	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(dest) // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x3 send four times in total)
			<< tx_packet_delay(20,100,10); // send packet w/ delay (send first packet with randomized delay from 20 to 100ms, repeat every 10ms)

		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair(FOURCHARS, 4) // string should be paired with length explicitly.
			, make_pair(b, e) // put timestamp here.
		);
		
		// do transmit 
		return pkt.transmit(); 
	}

	return false;
}

/** on receiving a packet */
void on_rx_packet(packet_rx& rx, bool_t &handled) {
	// check the packet header.
	const uint8_t* p = rx.get_payload().begin();
	if (rx.get_length() > 4 && !strncmp((const char*)p, (const char*)FOURCHARS, 4)) {
		Serial << format("..rx from %08x/%d", rx.get_addr_src_long(), rx.get_addr_src_lid()) << mwx::crlf;

		smplbuf_u8<128> buf;
		mwx::pack_bytes(buf			
				, uint8_t(rx.get_addr_src_lid())            // src addr (LID)
				, make_pair(p+4, rx.get_payload().end()) );	// data body

		serparser_attach pout;
		pout.begin(PARSER::ASCII, buf.begin(), buf.size(), buf.size());
		Serial << pout;
	}
}

/* Copyright (C) 2019-2021 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */