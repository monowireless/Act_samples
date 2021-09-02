// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <MONOSTICK>
#include <STG_STD>

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// option bits
uint32_t OPT_BITS = 0;

/*** function prototype */
bool analyze_payload(packet_rx& rx);

/*** application defs */

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	auto&& brd = the_twelite.board.use<MONOSTICK>();
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();

	// settings: configure items
	set << SETTINGS::appname("PARENT");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set << SETTINGS::lid_default(0x00); // set default LID
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.

	// the twelite main class
	the_twelite
		<< set                    // apply settings (appid, ch, power)
		<< TWENET::rx_when_idle() // open receive circuit (if not set, it can't listen packts from others)
		;

	// Register Network
	nwk << set;							// apply settings (LID and retry)
	nwk << NWK_SIMPLE::logical_id(0x00) // set Logical ID. (0x00 means parent device)
		;

	// configure hardware
	brd.set_led_red(LED_TIMER::ON_RX, 200); // RED (on receiving)
	brd.set_led_yellow(LED_TIMER::BLINK, 500); // YELLOW (blinking)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- MONOSTICK_Parent act ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
}

void on_rx_packet(packet_rx& rx, bool_t &handled) {
	Serial << ".. coming packet (" << int(millis()&0xffff) << ')' << mwx::crlf;

	// output type1 (raw packet)
	//   uint8_t  : 0x01
	//   uint8_t  : src addr (LID)
	//   uint32_t : src addr (long)
	//   uint32_t : dst addr (LID/long)
	//   uint8_t  : repeat count
	//     total 11 bytes of header.
	//     
	//   N        : payload
	if(0) { // this part is disabed.
		serparser_attach pout;
		pout.begin(PARSER::ASCII, rx.get_psRxDataApp()->auData, rx.get_psRxDataApp()->u8Len, rx.get_psRxDataApp()->u8Len);

		Serial << "RAW PACKET -> ";
		pout >> Serial;
		Serial.flush();
	}

	// output type2 (generate ASCII FORMAT)
	//  :0DCC3881025A17000000008D000F424154310F0D2F01D200940100006B39
	//   *1*2*3*4------*5------*6*7--*8
	if (1) {
		smplbuf_u8<256> buf;
		pack_bytes(buf
			, uint8_t(rx.get_addr_src_lid())		// *1:src addr (LID)
			, uint8_t(0xCC)							// *2:cmd id (0xCC, fixed)
			, uint8_t(rx.get_psRxDataApp()->u8Seq)	// *3:seqence number
			, uint32_t(rx.get_addr_src_long())		// *4:src addr (long)
			, uint32_t(rx.get_addr_dst())			// *5:dst addr
			, uint8_t(rx.get_lqi())					// *6:LQI
			, uint16_t(rx.get_length())				// *7:payload length
			, rx.get_payload() 						// *8:payload
				// , make_pair(rx.get_payload().begin() + 4, rx.get_payload().size() - 4)
				//   note: if you want the part of payload, use make_pair().
		);

		serparser_attach pout;
		pout.begin(PARSER::ASCII, buf.begin(), buf.size(), buf.size());
		
		Serial << "ASCII FMT -> ";
		pout >> Serial;
		Serial.flush();
	}

	// packet analyze
	analyze_payload(rx);
}

bool analyze_payload(packet_rx& rx) {
	bool b_handled = false;

#if 1
	// expand packet payload (shall match with sent packet data structure, see pack_bytes())
	uint8_t fourchars[4]{}; // init all elements as default (0).
	auto&& np = expand_bytes(rx.get_payload().begin(), rx.get_payload().end()
		, fourchars
    );
#else
	// an example to pass std::pair<char*,int>.
	char fourchars[5]{};
	auto&& np = expand_bytes(
		    rx.get_payload().begin(), rx.get_payload().end()
			, make_pair((char *)fourchars, 4)
		);
#endif

	// if heading 4 bytes are not present, unexpected packet data.
	if (np == nullptr) return false;

	// display fourchars at first
	Serial
		<< fourchars 
		<< format("(ID=%d/LQ=%d)", rx.get_addr_src_lid(), rx.get_lqi())
		<< "-> ";

	// Slp_Wk_and_Tx
	if (!b_handled && !strncmp((char*)fourchars, "TXSP", 4)) {
		b_handled = true;
		uint32_t tick_ms;
		uint16_t u16work_ct;

		np = expand_bytes(np, rx.get_payload().end()
			, tick_ms
			, u16work_ct
		);

		if (np != nullptr) {
			Serial << format("Tick=%d WkCt=%d", tick_ms, u16work_ct);
		} else {
			Serial << ".. error ..";
		}
	}

	// BRD_APPTWELITE
	if (!b_handled && !strncmp((char*)fourchars, "BAT1", 4)) {
		b_handled = true;

		uint8_t u8DI_BM_remote = 0xff;
		uint16_t au16AI_remote[5];
		np = expand_bytes(np, rx.get_payload().end()
			, u8DI_BM_remote
			, au16AI_remote[0]
			, au16AI_remote[1]
			, au16AI_remote[2]
			, au16AI_remote[3]
			, au16AI_remote[4]
		);

		if (np != nullptr) {
			Serial << format("DI:%04b", u8DI_BM_remote & 0x0F);
			for (auto&& x : au16AI_remote) {
				Serial << format("/%04d", x);
			}
		} else {
			Serial << ".. error ..";
		}
	}

	// AMB
	if 	(!b_handled && 
			(  !strncmp((char*)fourchars, "PAB1", 4) // PAL_AMB
			|| !strncmp((char*)fourchars, "PAB2", 4) // PAL_AMB_usernap
			)
		)
	{
		b_handled = true;

		uint32_t u32lumi;
		uint16_t u16temp;
		uint16_t u16humd;

		np = expand_bytes(np, rx.get_payload().end()
			, u32lumi
			, u16temp
			, u16humd
		);

		if (np != nullptr) {
			Serial 	<< format("Lumi:%d", u32lumi)
					<< " Temp:" << div100(int16_t(u16temp))
					<< " Humid:" << div100(int16_t(u16humd))
					;
		} else {
			Serial << ".. error ..";
		}
	}
	
	// MAG
	if 	(!b_handled && 
			(  !strncmp((char*)fourchars, "PMG1", 4) // PAL_AMB
			)
		)
	{
		b_handled = true;

		uint8_t b_north;
		uint8_t b_south;

		np = expand_bytes(np, rx.get_payload().end()
			, b_north
			, b_south
		);

		if (np != nullptr) {
			Serial 	<< format("N=%d S=%d", b_north, b_south)
					;
		} else {
			Serial << ".. error ..";
		}
	}

	// MOT
	if 	(!b_handled && 
			(  !strncmp((char*)fourchars, "PMT1", 4) // PAL_AMB
			)
		)
	{
		b_handled = true;

		uint8_t siz = 255;
		uint16_t seq = 65535;

		int16_t x[16], y[16], z[16];

		np = expand_bytes(np, rx.get_payload().end()
				, seq
				, siz
			);

		Serial << format("SZ=%d SQ=%d ", siz, seq);
		
		if (np != nullptr && siz > 0) {
			uint32_t v1;
			uint8_t v2;
			for (int i = 0; i < siz; i++) {
				np = expand_bytes(np, rx.get_payload().end(), v1, v2);
				if (np != nullptr) {
					// Coming value format is signed 13bit format (-4000-4000 milliG).
					// v1: [13bit X][13bit Y][6bit Z MSBside]
					// v2: [7bit Z LSBside]
				
					// Each values are splitted into 13bits and expand to int16_t for signess ((v << 3) >> 3).
					x[i] = int16_t(((v1 >> 19) & 0x1FFF) << 3) >> 3; 
					y[i] = int16_t(((v1 >> 6) & 0x1FFF) << 3) >> 3;
					z[i] = int16_t((((v1 & 0x3F) << 7) + (v2 & 0x7F)) << 3) >> 3;
				} else break;
			}
		}

		if (np != nullptr) {
			for (int i = 0; i < siz; i++) {
				Serial << format("(%d,%d,%d)", x[i], y[i], z[i]);
			}
		} else {
			Serial << ".. error ..";
		}
	}

	// Unknown
	if (!b_handled) Serial << "..not analyzed..";

	// finally put line break.
	Serial << mwx::crlf;

	// returns status
	return b_handled;
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */