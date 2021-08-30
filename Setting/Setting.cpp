// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <STG_STD> // interactive mode

/*** Settings, Interactive mode */
#if 0
const TWESTG_tsElement SET_STD_DEFSETS[SIZE_SET_STD_DEFSETS] = {
	{ E_TWESTG_DEFSETS_APPID,  // アプリケーションID
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x67726301 }}, // 32bit (デフォルトのIDは配列決め打ちなので、ボード定義でオーバライドが必要)
		{ "AID", "\033[41mAPP ID [HEX:32bit]\033[0m ", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'a' },
		{ {.u32 = 0}, {.u32 = 0}, TWESTGS_VLD_u32AppId, NULL },
	},
	{ E_TWESTG_DEFSETS_LOGICALID,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 1 }}, 
		{ "LID", "Device ID [1-100,etc]", "" }, 
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 3, 'i' },
		{ {.u32 = 0}, {.u32 = 100}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNEL,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 18 }},
		{ "CHN", "Channel [11-26]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'c' },
		{ {.u32 = 11}, {.u32 = 26}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNELS_3,
		{ TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, {.u16 = ((1UL << 18) >> 11) }},
		{ "CHL", "Channels Set", 
		  "Input up to 3 channels like '11,15,24'."
		},
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'c' },
		{ {.u16 = 0}, {.u16 = 0xFFFF}, TWESTGS_VLD_u32ChList, NULL },
	},
	{ E_TWESTG_DEFSETS_POWER_N_RETRY,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = 0x03 }},
		{ "PWR", "RF Power/Retry [HEX:8bit]",
			"YZ Y=Retry(0:default,F:0,1-9:count\r\n"
			"Z = Power(3:Max,2,1,0 : Min)" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 2, 'x' },
		{ {.u32 = 0}, {.u32 = 0xFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPTBITS, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OP1", "Option1 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'o' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD1, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OP2", "Option2 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'p' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD2, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OP3", "Option3 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'q' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_OPT_DWORD3, 
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = 0x00000000 }}, 
		{ "OP4", "Option4 [HEX:32bit]", "" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'r' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};
#endif

const TWESTG_tsMsgReplace SET_MSGS[] = {
	{ (uint8_t)E_STGSTD_SETID::OPTBITS,    "ｵﾌﾟｼｮﾝﾋﾞｯﾄ", "オプション１を設定してください" },
	{ (uint8_t)E_STGSTD_SETID::OPT_DWORD2, "オプション2", "オプション２を設定してください\r\nとりあえず\r\n" },
	{ E_TWESTG_DEFSETS_VOID } // terminator
};

/*** Config part */
// app name
const char *APP_NAME = "Setting";
// application ID
const uint32_t DEF_APP_ID = 0x1234abcd;
uint32_t APP_ID = DEF_APP_ID;

// channel
const uint8_t DEF_CHANNEL = 13;
uint8_t CHANNEL = DEF_CHANNEL;

// LID
uint8_t LID = 0xFE;

// DIO pins
const uint8_t PIN_BTN = 12;

/*** function prototype */
MWX_APIRET vTransmit();

/*** application defs */
MWX_APIRET txreq_stat; // check tx completion status

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	txreq_stat = MWX_APIRET(false, 0);

	/// interactive mode settings
	auto&& set = the_twelite.settings.use<STG_STD>();
			// declare to use interactive setting.
			// once activated, use `set.serial' instead of `Serial'.

	// common configs
	set << SETTINGS::appname(APP_NAME)          // set application name appears in interactive setting menu.
		<< SETTINGS::appid_default(DEF_APP_ID)  // set the default application ID.
		<< SETTINGS::ch_default(DEF_CHANNEL)    // set default channel
		<< SETTINGS::lid_default(7)             // set default logical id
		//<< SETTINGS::open_at_start()            // if set, show the mode at the initial.
		;
	
	// more detailed configs...
	set.replace_item_name_desc(SET_MSGS); // replace names and descs

	// hide some items
	bool apiret = set.hide_items(
		//E_STGSTD_SETID::APPID, // never be removed
		E_STGSTD_SETID::LOGICALID, // never be removed
		E_STGSTD_SETID::CHANNEL, // never be removed
		//E_STGSTD_SETID::CHANNELS_3, // hide default
		//E_STGSTD_SETID::POWER_N_RETRY,
		//E_STGSTD_SETID::OPTBITS, // show it (for example)
		E_STGSTD_SETID::UARTBAUD,
		
		//E_STGSTD_SETID::OPT_DWORD2,
		E_STGSTD_SETID::OPT_DWORD3, 
		E_STGSTD_SETID::OPT_DWORD4,
		E_STGSTD_SETID::ENC_MODE,
		E_STGSTD_SETID::ENC_KEY_STRING
		); // hide some item(s).
	Serial << format("hide_items %d", apiret) << crlf;

	// acquired EEPROM saved data	
	set.reload(); // must call this before getting data, if configuring method is called.

	APP_ID = set.u32appid();
	CHANNEL = set.u8ch();
	LID = set.u8devid();

	/// the twelite main class
	the_twelite
		//<< TWENET::appid(APP_ID)    // set application ID (identify network group)
		//<< TWENET::channel(CHANNEL) // set channel (pysical channel)
		<< set                      // APP_ID, CHANNEL, POWER, RETRY setting will be applied.
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// check if set obj did well.
	the_twelite >> Serial;

	/// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk	//<< NWK_SIMPLE::logical_id(LID) // set Logical ID. (0xFE means a child device with no ID)
		<< set // LID/REPERATMAX will be set from interactive mode.
		;

	// check if set obj did well.
	Serial 	<< format("nwk << set: LID=%d RPT=%d"
						, nwk.get_config().u8Lid
						, nwk.get_config().u8RepeatMax
					)
			<< mwx::crlf;

	/*** BEGIN section */
	Buttons.begin(pack_bits(PIN_BTN), 5, 10); // check every 10ms, a change is reported by 5 consequent values.
	// Analogue.begin(pack_bits(PIN_ANALOGUE::A1, PIN_ANALOGUE::VCC)); // _start continuous adc capture.

	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- Scratch act ---" << mwx::crlf;
}

/*** begin procedure (called once at boot) */
void begin() {
	Serial << "..begin (run once at boot)" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	// digital input change.
	if (Buttons.available()) {
		uint32_t bm, cm;
		Buttons.read(bm, cm);

		if (cm & 0x80000000) {
			// the first capture.
		}

		Serial << int(millis()) << ":BTN" << format("%b") << mwx::crlf;
	}

    // read from serial
	while(Serial.available())  {
        int c = Serial.read();

		Serial << '[' << char(c) << ']';

        switch(c) {
			case 'p':
				Serial << int(millis()) << ":p pressed!" << mwx::crlf;
				break;

			case 't':
				if (!txreq_stat) {
					txreq_stat = vTransmit();
					if (txreq_stat) {
						Serial << int(millis()) << ":tx request success! (" << int(txreq_stat.get_value()) << ')' << mwx::crlf;
 					} else {
						Serial << int(millis()) << ":tx request failed" << mwx::crlf;;
					 }
				}
				break;

			case 's':
				Serial << int(millis()) << ":sleeping for " << 5000 << "ms" << mwx::crlf << mwx::flush;
				the_twelite.sleep(5000);
				break;

            default:
				break;
        }
	}

	// packet
	if (the_twelite.receiver.available()) {
		auto&& rx = the_twelite.receiver.read();

		// just dump a packet.
		Serial << format("rx from %08x/%d", rx.get_addr_src_long(), rx.get_addr_src_lid()) << mwx::crlf;
	}

	// check tx completion
	if (txreq_stat) {
		if (the_twelite.tx_status.is_complete(txreq_stat.get_value())) {
			Serial << int(millis()) << ":tx completed! (" << int(txreq_stat.get_value()) << ')' << mwx::crlf;
			txreq_stat = MWX_APIRET(false, 0);
		}
	}
}

void wakeup() {
	Serial << int(millis()) << ":wake up!" << mwx::crlf;
}

/** transmit a packet */
MWX_APIRET vTransmit() {
	Serial << int(millis()) << ":vTransmit()" << mwx::crlf;

	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(0xFF)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x3 send four times in total)
			<< tx_packet_delay(100,200,20); // send packet w/ delay (send first packet with randomized delay from 100 to 200ms, repeat every 20ms)

		// prepare packet payload
		pack_bytes(pkt.get_payload() // set payload data objects.
			, make_pair("SCRT", 4) // string should be paired with length explicitly.
			, uint32_t(millis()) // put timestamp here.
		);
		
		// do transmit 
		//return nwksmpl.transmit(pkt);
		return pkt.transmit(); 
	}

	return MWX_APIRET(false, 0);
}

/* Copyright (C) 2020 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */