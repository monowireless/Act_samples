/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */


#include <TWELITE>
#include <NWK_LAYERED>
#include <NWK_SIMPLE>
#include <STG_STD>

#include "common.h"
#include "pkt_common.hpp"
#include "pkt_handler.hpp"

// role is parent (fixed)
static const uint8_t U8_NWK_ROLE = NWK_LAYERED::ROLE_PARENT;
static const uint8_t U8_MY_LID = 0x00;

static const uint32_t U32_MY_APPID_DEFAULT = 0x67726305;
static const uint8_t U8_MY_CH_DEFAULT = 15; // 15 is default ID for PAL/CUE/ARIA.

void setup() {
	// initialize global objects
	mwx::pnew(g_pkt_pal);
	mwx::pnew(g_pkt_apptwelite);
	mwx::pnew(g_pkt_actsamples);
	mwx::pnew(g_pkt_unknown);

	// initialize system behaviors
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& nwk_ly = the_twelite.network.use<NWK_LAYERED>();
	auto&& nwk_sm = the_twelite.network2.use<NWK_SIMPLE>();

	// settings: configure items
	set << SETTINGS::appname("NWK_LAYERED PARENT");
	set << SETTINGS::appid_default(U32_MY_APPID_DEFAULT); // set default appID
	set << SETTINGS::ch_default(U8_MY_CH_DEFAULT); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.

	// the twelite main class
	the_twelite
		<< set                      // apply settings (appid, ch, power)
		<< TWENET::rx_when_idle();  // open receive circuit (if not set, it can't listen packts from others)

	// Register Network
	nwk_ly
		<< NWK_LAYERED::network_role(NWK_LAYERED::ROLE_PARENT) // set a role as parent.
		;

	nwk_sm
		<< NWK_SIMPLE::logical_id(0)             // as a parent device.
		<< NWK_SIMPLE::receive_nwkless_pkt(true) // receive network less packet (e.g. App_Twelite)
		;

	the_twelite.debug_level(0); // set debug level

	// let the twelite begin!
	the_twelite.begin();

	// calc other values
	{
		uint8_t v[4];
		uint32_t appid = the_twelite.get_appid();

		for(int i = 3; i >= 0; --i) {
			v[i] = appid & 0xff;
			appid >>= 8;
		}
		g_u8_appidentifier = mwx::CRC8_u8Calc(v, 4);
	}

	// starting message
	Serial << crlf << "--NWK_LAYERED--"  << mwx::flush;
}


void loop() {
	auto&& nwk_ly = the_twelite.network.use<NWK_LAYERED>();
	//auto&& nwk_sm = the_twelite.network2.use<NWK_SIMPLE>();

	if (TickTimer.available()) {
		static unsigned t;
		if (!(++t & 0x3FF)) { // every 1024ms
			g_pkt_pal.refresh();
			g_pkt_apptwelite.refresh();
			g_pkt_actsamples.refresh();
			g_pkt_unknown.refresh();
		}
	}

	if(Serial.available()) {
		int c = Serial.read();

		if(c >= 0 && c < 256) {
			Serial << crlf << format("[%c]", c);
			switch(c) {
#if DEBUG_DUP_CHECKER == 1
			case 'D': // run test code of duplicate checker.
				test_dup_checker();
			break;
#endif

#if TOCONET_DEBUG == 1
			case 'd': // set internal TWENET debug messages
			{
				static uint8_t d = 0;
				d++; if(d > 5) d = 0;
				the_twelite.debug_level(d); // set debug level

				Serial << format("!set NwkCode debug level to %d.", d);
			}
			break;
#endif

			case '#': // info
			{
				tsToCoNet_NwkLyTr_Context *pc = (tsToCoNet_NwkLyTr_Context *)nwk_ly._get_nwk_context();

				Serial << crlf
					   << format("!!! TWELITE NET(ver%08X) !!!", ToCoNet_u32GetVersion());
				Serial << crlf
					   << format("  AppID %08x, LongAddr, %08x, ShortAddr %04x, Tk: %d",
							   sToCoNet_AppContext.u32AppId, ToCoNet_u32GetSerial(), sToCoNet_AppContext.u16ShortAddress, u32TickCount_ms);

				Serial << crlf << format("  Nwk Info: la=%d ty=%d ro=%02x st=%02x",
							pc->sInfo.u8Layer, pc->sInfo.u8NwkTypeId, pc->sInfo.u8Role, pc->sInfo.u8State);
				Serial << crlf << format("  Nwk Parent: %08x", pc->u32AddrHigherLayer);
			}
			break;

			case '!':
			{
				Serial << crlf << "!INF RESET SYSTEM.";
				delay(1000);
				the_twelite.reset_system();
			}
			break;
			}
		}
	}
}

void on_rx_packet(packet_rx& rx, bool_t &handled) {
	auto type = rx._get_network_type();
	bool b_handled = false;

	// PAL
	if (!b_handled
		&& type == mwx::NETWORK::LAYERED
		&& g_pkt_pal.analyze(rx, b_handled)
	) {
		g_pkt_pal.display(rx);
	}

	// Act samples
	if (!b_handled
		&& type == mwx::NETWORK::SIMPLE
		&& g_pkt_actsamples.analyze(rx, b_handled)
	) {
		g_pkt_actsamples.display(rx);
	}

	// Standard application (e.g. App_Twelite)
	if (!b_handled
		&& type == mwx::NETWORK::NONE
		&& g_pkt_apptwelite.analyze(rx, b_handled)
	) {
		g_pkt_apptwelite.display(rx);
	}

	// unknown
	if (!b_handled) {
		g_pkt_unknown.analyze(rx, b_handled);
		g_pkt_unknown.display(rx);
	}
}

