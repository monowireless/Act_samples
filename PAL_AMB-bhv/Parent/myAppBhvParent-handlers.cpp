/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

// mwx header
#include <TWELITE>
#include <PAL_AMB>
#include "myAppBhvParent.hpp"

/*****************************************************************/
// MUST DEFINE CLASS NAME HERE
#define __MWX_APP_CLASS_NAME MY_APP_PARENT
#include "_mwx_cbs_cpphead.hpp"
/*****************************************************************/

	MWX_TICKTIMER_INT(uint32_t arg, uint8_t& handled) {
		// blink LED
		digitalWrite(PAL_AMB::PIN_LED, ((millis() >> 9) & 1) ? PIN_STATE::HIGH : PIN_STATE::LOW);
	}

	MWX_DIO_EVENT(PAL_AMB::PIN_BTN, uint32_t arg) {
		Serial << "Button Pressed" << mwx::crlf;
		
		static uint32_t u32tick_last;
		uint32_t tick = millis();

		if (tick - u32tick_last > 100) {
			PEV_Process(E_ORDER_KICK, 0UL);
		}

		u32tick_last = tick;
	}

    MWX_STATE(E_MWX::STATE_0, uint32_t ev, uint32_t evarg) {
		if (ev == E_EVENT_START_UP) {
			Serial << "[STATE_0:START_UP]" << mwx::crlf;	
		} else
		if (ev == E_ORDER_KICK) {
			PEV_SetState(E_MWX::STATE_1);
		}
    }

    MWX_STATE(E_MWX::STATE_1, uint32_t ev, uint32_t evarg) {
		if (ev == E_EVENT_NEW_STATE) {
			Serial << "[STATE_1]" << mwx::crlf;	
		} else
		if (ev == E_ORDER_KICK) {
			PEV_SetState(E_MWX::STATE_2);
		} else
		if (ev == E_EVENT_TICK_SECOND) {
			Serial << "<1>";
		}	
    }
	
    MWX_STATE(E_MWX::STATE_2, uint32_t ev, uint32_t evarg) {
		if (ev == E_EVENT_NEW_STATE) {
			Serial << "[STATE_2]" << mwx::crlf;	
		} else
		if (ev == E_ORDER_KICK) {
			PEV_SetState(E_MWX::STATE_3);
		} else
		if (ev == E_EVENT_TICK_SECOND) {
			Serial << "<2>";
		}	
    }
	
    MWX_STATE(E_MWX::STATE_3, uint32_t ev, uint32_t evarg) {
		if (ev == E_EVENT_NEW_STATE) {
			Serial << "[STATE_3]" << mwx::crlf;	
		} else
		if (ev == E_ORDER_KICK) {
			PEV_SetState(E_MWX::STATE_1);
		} if (PEV_u32Elaspsed_ms() > 3000) {
			Serial << "[STATE_3:timeout]" << mwx::crlf;	
			PEV_SetState(E_MWX::STATE_2);
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