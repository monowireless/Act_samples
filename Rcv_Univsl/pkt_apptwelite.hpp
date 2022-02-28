/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#ifndef SOURCE_PKT_APPTWELITE_HPP_
#define SOURCE_PKT_APPTWELITE_HPP_


#include <TWELITE>

#include <EASTL/fixed_vector.h>

#include "dup_checker.hpp"

#include "common.h"
#include "pkt_common.hpp"


class pkt_apptwelite {
	// 15entries MAX
	dup_check<dup_check<>::MODE_REJECT_OLDER_SEQ + 15, 3000, 15, 7> _dup_chk;

public:
	pkt_apptwelite() : data{} {}

	bool identify(mwx::packet_rx& rx) {
		const uint8_t *p = rx.get_payload().begin();
		// const uint8_t *e = rx.get_payload().end();
		uint16 u16len = rx.get_length();

		//   0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 4 5 6 -
		//  150175810000380026C9000C04220000FFFFFFFFFF
		//  IdPt
		if (rx.get_cmd() == TYPE_X81		// 0x81 command (sensor data information)
			&& u16len == 21					// 
			&& p[0] == g_u8_appidentifier	// generated from AppID
			&& p[1] == PROTOCOL_VERSION 	// protocol version (0x01)
			&& (p[3] & 0x80) == 0x80 	 	// SID: MSB must be set
			) {
			return true;
		} else 
		if (rx.get_cmd() == TYPE_MSG		// Message
			&& p[0] == g_u8_appidentifier	// generated from AppID
			&& p[1] == PROTOCOL_VERSION		// protocol version (0x01)
			&& (p[3] & 0x80) == 0x80		// SID: MSB must be set
			&& u16len == p[16] + 17     	// packet data length
			) {
			return true;
		}

		return false;
	}

	bool analyze(mwx::packet_rx& rx, bool &b_handled);
	void refresh() { _dup_chk.clean_by_timeout(millis()); }

public:
	static const uint8_t PROTOCOL_VERSION = 0x01;
	static const uint8_t TYPE_MSG = TOCONET_PACKET_CMD_APP_DATA;
	static const uint8_t TYPE_X81 = TOCONET_PACKET_CMD_APP_USER + 0;
	static const uint8_t TYPE_X80 = TOCONET_PACKET_CMD_APP_USER + 1;

	struct DataTwelite_x81 {

		/**
		 * true: DI1 is activated (set as Lo),
		 */
		uint8_t DI1;

		/**
		 * true: DI1 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI1_active;

		/**
		 * true: DI2 is activated (set as Lo)
		 */
		uint8_t DI2;

		/**
		 * true: DI2 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI2_active;

		/**
		 * true: DI3 is activated (set as Lo)
		 */
		uint8_t DI3;

		/**
		 * true: DI3 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI3_active;

		/**
		 * true: DI4 is activated (set as Lo)
		 */
		uint8_t DI4;

		/**
		 * true: DI4 is activated before.
		 * false: the port had not been activated ever.
		 */
		uint8_t DI4_active;

		/**
		 * DI state mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * Note: same values as DI?.
		 */
		uint8_t DI_mask;

		/**
		 * DI active mask, active if bit set. (LSB:DI1, bit2:DI2, ...)
		 * Note: same values as DI?_active.
		 */
		uint8_t DI_active_mask;


		/**
		 * ADC1 value in mV
		 */
		uint16_t u16Adc1;

		/**
		 * ADC2 value in mV
		 */
		uint16_t u16Adc2;

		/**
		 * ADC3 value in mV
		 */
		uint16_t u16Adc3;

		/**
		 * ADC4 value in mV
		 */
		uint16_t u16Adc4;

		/**
		 * if bit set, Adc has value (LSB: ADC1, bit2: ADC2, ...),
		 * otherwise Adc is connected to Vcc voltage.
		 * (note: Setting Adc as Vcc level means unused to App_Twelite firmware,
		 *        even the hardware can measure up to 2.47V)
		 */
		uint8_t Adc_active_mask;
	};

	struct DataTwelite_Msg {
		uint8_t u8dst_addr;
		uint8_t u8message_id;
		uint8_t payload[80];
	};

	struct _data_apptwelite : public PktDataCommon {
		/**
		 * sequence counter
		 */
		uint16_t u16timestamp;

		/**
		 * true when trying to low latency transmit (same packets will come)
		 */
		uint8_t b_lowlatency_tx;

		/**
		 * packet repeat count
		 *   e.g.) if set 1, the packet passed to one repeater (router) to the destination.
		 */
		uint8_t u8rpt_cnt;

		union {
			DataTwelite_x81 x81;
			DataTwelite_Msg msg;
		};
		
		/**
		 * vector to attach msg.payload[].
		 * note: placing class object as union member is confusing, therefore
		 *       attaching vector is used instead.
		 */
		mwx::smplbuf_u8_attach payload_msg;
	} data;
};

#endif /* SOURCE_PKT_APPTWELITE_HPP_ */
