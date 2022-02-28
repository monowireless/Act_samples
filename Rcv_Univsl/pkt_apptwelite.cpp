/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#include <TWELITE>
#include "pkt_apptwelite.hpp"

using namespace mwx;

/**
 * Packet analyze for App_Twelite.
 * @param rx        rx packet information.
 * @param b_handled true when the packet is handled and should not process further.
 * @return true     the packet is valid for application use.
 * @return false    the packet should be rejected (e.g. different structure, duplicated, etc.)
 */
bool pkt_apptwelite::analyze(mwx::packet_rx& rx, bool& b_handled) {
	const uint8_t *p = rx.get_payload().begin();
	const uint8_t *p_end = rx.get_payload().end();
	b_handled = false;

	// check packet structure.
	if (!identify(rx)) return false;

	// clean data area
	mwx::pnew(data);

	// analyze payload.
	(void)G_BYTE(p); // App Identifier p[0]

	(void)G_BYTE(p); // Protocol version p[1]

	data.u8addr_src = G_BYTE(p); // p[2]

	data.u32addr_src = G_DWORD(p); // p[3..6]

	data.u8addr_dst = G_BYTE(p); // p[7]
	if(data.u8addr_dst != 0x00) {
		b_handled = true;
		return false; // ignore
	}

	{
		uint16_t val =  G_WORD(p); // p[8..9]

		data.u16timestamp = val & 0x7FFF;
		data.b_lowlatency_tx = !!(val & 0x8000);
	}

	// perform duplication check
	if (   (1 && rx.get_cmd() == TYPE_MSG)
		|| (rx.get_cmd() != TYPE_MSG && data.b_lowlatency_tx == false)
		)
	{
		if (_dup_chk(data.u32addr_src, data.u16timestamp, rx.get_tick_received())) {
			b_handled = true;
			return false;
		}
	}

	data.u8rpt_cnt = G_BYTE(p); // relay flag p[10]

	// 0x81 command : inform sensor data
	if (rx.get_cmd() == TYPE_X81) {
		data.u8type = TYPE_X81;                     // set data type for choosing union member.

		data.u16volt = (short)G_WORD(p);			// module voltage

		auto& x = data.x81;

		uint32_t c = G_BYTE(p);						// unused

		c = G_BYTE(p);								// DI state bit
		x.DI1 = ((c & 0x01) == 0x01);
		x.DI2 = ((c & 0x02) == 0x02);
		x.DI3 = ((c & 0x04) == 0x04);
		x.DI4 = ((c & 0x08) == 0x08);
		x.DI_mask = c;

		c = G_BYTE(p);								// DI active state bit
		x.DI1_active = ((c & 0x01) == 0x01);
		x.DI2_active = ((c & 0x02) == 0x02);
		x.DI3_active = ((c & 0x04) == 0x04);
		x.DI4_active = ((c & 0x08) == 0x08);
		x.DI_active_mask = c;

		x.Adc_active_mask = 0;					// ADC
		x.u16Adc1 = G_BYTE(p);
		if (x.u16Adc1 != 0xFF) x.Adc_active_mask |= 1; else x.u16Adc1 = 0xFFFF;
		x.u16Adc2 = G_BYTE(p);
		if (x.u16Adc2 != 0xFF) x.Adc_active_mask |= 2; else x.u16Adc2 = 0xFFFF;
		x.u16Adc3 = G_BYTE(p);
		if (x.u16Adc3 != 0xFF) x.Adc_active_mask |= 4; else x.u16Adc3 = 0xFFFF;
		x.u16Adc4 = G_BYTE(p);
		if (x.u16Adc4 != 0xFF) x.Adc_active_mask |= 8; else x.u16Adc4 = 0xFFFF;

		c = G_BYTE(p);  // additional bits (2bits from LSB) for more accuracy.
		if (x.u16Adc1 != 0xFFFF) x.u16Adc1 = ((x.u16Adc1 * 4 + ((c >> 0) & 0x3)) * 4);
		if (x.u16Adc2 != 0xFFFF) x.u16Adc2 = ((x.u16Adc2 * 4 + ((c >> 2) & 0x3)) * 4);
		if (x.u16Adc3 != 0xFFFF) x.u16Adc3 = ((x.u16Adc3 * 4 + ((c >> 4) & 0x3)) * 4);
		if (x.u16Adc4 != 0xFFFF) x.u16Adc4 = ((x.u16Adc4 * 4 + ((c >> 6) & 0x3)) * 4);

		// returns
		b_handled = true;
		return true;
	}
	// 0x08 command : change request packet
	else if (rx.get_cmd() == TYPE_X80) {
		// 0x80 command : setting request
		b_handled = true;
		return false;
	}
	// Others (serial messages, etc)
	else if (rx.get_cmd() == TYPE_MSG) {
		uint8 u8req = G_BYTE(p);        //p[11] : request ID (unique for packets set)
		uint8 u8pktnum = G_BYTE(p);     //p[12] : total packets number
		uint8 u8idx = G_BYTE(p);        //p[13] : the current packet idx (0..u8pktnum-1)
		uint16 u16offset = G_WORD(p);   //p[14..15] : offset from buffer header
										//            , where the current packet data is stored
		uint8 u8len = G_BYTE(p);        //p[16] : data length of the followings.

		// Packets could be split into some packets, in case that the length exceeds the limit
		// size of a single packet.
		// The necessary information to assemble packets are stored this header block.
		// - The split packets will have the same `u8req`.
		// - This packet is `u8idx+1` of `u8pktnum` packets in total.
		// - Packet data is stored from `buf[u16offset]` to `buf[u16offset+u8len-1]`.

		// Note: The following code does not support split packets.
		if (   u8pktnum == 1 			// data is not split into packets, but a single.
			&& u8idx == 0				// the first packet, the idx of a single packet always zero.
			&& (p_end - p) == u8len		// length data is correct
			&& u8len > 2                // the first packet will have two bytes header information.
			&& u8len <= sizeof(data.msg.payload)
		) {
			data.u8type = TYPE_MSG;                     // set data type for choosing union member.
			auto& x = data.msg;

			// the first two bytes of data payload is header information.
			x.u8dst_addr = G_BYTE(p);
			x.u8message_id = G_BYTE(p);

			// attaching existing buffer
			data.payload_msg.attach(x.payload, 0, sizeof(x.payload));

			// the following part should be 
			for(; p != p_end; ++p) {
				data.payload_msg.push_back(*p); // save data into the container.
			}

			// returns
			b_handled = true;
			return true;
		}

		b_handled = true;
		return false;
	}
}
