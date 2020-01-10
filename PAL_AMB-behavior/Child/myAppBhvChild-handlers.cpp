/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

// mwx header
#include <TWELITE>
#include <PAL_AMB>
#include <NWK_SIMPLE>
#include "myAppBhvChild.hpp"

/*****************************************************************/
// MUST DEFINE CLASS NAME HERE
#define __MWX_APP_CLASS_NAME MY_APP_CHILD
#include "_mwx_cbs_cpphead.hpp"
/*****************************************************************/

MWX_STATE(MY_APP_CHILD::STATE_IDLE, uint32_t ev, uint32_t evarg) {
	if (PEV_is_coldboot(ev,evarg)) {
		Serial << "[STATE_IDLE:START_UP(" << int(evarg) << ")]" << mwx::crlf;
		// then perform the first sleep at on_begin().
	} else
	if (PEV_is_warmboot(ev,evarg)) {
		Serial << "[STATE_IDLE:START_UP(" << int(evarg) << ")]" << mwx::crlf;
		PEV_SetState(STATE_SENSOR);
	}
}

MWX_STATE(MY_APP_CHILD::STATE_SENSOR, uint32_t ev, uint32_t evarg) {
	if (ev == E_EVENT_NEW_STATE) {
		Serial << "[STATE_SENSOR:NEW] Start Sensor." << mwx::crlf;

		// start sensor capture
		shtc3_start();
		ltr308als_start();

		// take a nap waiting finish of capture.
		Serial << "..nap for 66ms" << mwx::crlf;
		Serial.flush();
		PEV_KeepStateOnWakeup(); // stay this state on waking up.
		the_twelite.sleep(66, false, false, TWENET::SLEEP_WAKETIMER_SECONDARY);
	} else
	if (PEV_is_warmboot(ev,evarg)) {
		// on wakeup, code starts here.
		Serial << "[STATE_SENSOR:START_UP] Wakeup." << mwx::crlf;

		PEV_SetState(STATE_TX);
	}
}

MWX_STATE(MY_APP_CHILD::STATE_TX, uint32_t ev, uint32_t evarg) {
	static int u8txid;

	if (ev == E_EVENT_NEW_STATE) {
		Serial << "[STATE_TX:NEW]" << mwx::crlf;
		u8txid = -1;

		auto&& r1 = shtc3_read();
		auto&& r2 = ltr308als_read();

		Serial << "..shtc3 t=" << int(i16Temp) << ", h=" << int(i16Humd) << mwx::crlf;
		Serial << "..ltr308als l=" << int(u32Lumi) << mwx::crlf;

		if (r1 && r2) {
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1)  // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, uint32_t(u32Lumi) // luminance (NOTE: not getting data in this act/behavior)
					, uint16_t(i16Temp)
					, uint16_t(i16Humd)
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request (" << int(ret.get_value()) << ')' << mwx::crlf;

				if (ret) {
					u8txid = ret.get_value() & 0xFF;
				}
				else {
					// fail to request
					PEV_SetState(STATE_SLEEP);
				}
			}
		} else {	
			PEV_SetState(STATE_SLEEP);
		}
	} else if (ev == E_ORDER_KICK && evarg == uint32_t(u8txid)) {
		Serial << "[STATE_TX] SUCCESS TX(" << int(evarg) << ')' << mwx::crlf;
		PEV_SetState(STATE_SLEEP);
	}

	// timeout
	if (PEV_u32Elaspsed_ms() > 100) {
		// does not finish TX!
		Serial << "[STATE_TX] FATAL, TX does not finish!" << mwx::crlf << mwx::flush;
		the_twelite.reset_system();
	}
}

MWX_STATE(MY_APP_CHILD::STATE_SLEEP, uint32_t ev, uint32_t evarg) {
	if (ev == E_EVENT_NEW_STATE) {
		Serial << "..sleep for 5000ms" << mwx::crlf;
		pinMode(PAL_AMB::PIN_BTN, PIN_MODE::WAKE_FALLING_PULLUP);
		digitalWrite(PAL_AMB::PIN_LED, PIN_STATE::HIGH);
		Serial.flush();

		the_twelite.sleep(5000); // regular sleep
	}
}

/*****************************************************************/
// common procedure (DO NOT REMOVE)
#include "_mwx_cbs_cpptail.cpp"
// MUST UNDEF CLASS NAME HERE
#undef __MWX_APP_CLASS_NAME
/*****************************************************************/

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */
