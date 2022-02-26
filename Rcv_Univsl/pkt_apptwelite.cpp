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
	//const uint8_t *p_end = rx.get_payload().end();
	b_handled = false;

	// check packet structure.
	if (!identify(rx)) return false;

	// clean data area
	mwx::pnew(data);

	// analyze payload.
	(void)G_BYTE(p); // App Identifier

	(void)G_BYTE(p); // Protocol version

	data.u8addr_src = G_BYTE(p);

	data.u32addr_src = G_DWORD(p);

	data.u8addr_dst = G_BYTE(p);
	if(data.u8addr_dst != 0x00) {
		b_handled = true;
		return false; // ignore
	}

	{
		uint16_t val =  G_WORD(p);

		data.u16timestamp = val & 0x7FFF;
		data.b_lowlatency_tx = !!(val & 0x8000);
	}

	if (rx.get_cmd() != TOCONET_PACKET_CMD_APP_DATA) {
		if (  data.b_lowlatency_tx == false
		   && _dup_chk(data.u32addr_src, data.u16timestamp, rx.get_tick_received())
		   )
		{
			b_handled = true;
			return false;
		}
	}

	data.u8rpt_cnt = G_BYTE(p); // relay flag

	// 0x81 command : inform sensor data
	if (rx.get_cmd() == TYPE_X81) {
		data.u8type = TYPE_X81;                     // set data type for choosing union member.

		data.u16volt = (short)G_WORD(p);			// module voltage

		auto& x = data.x81;

		uint32_t c = G_BYTE(p);								// unused

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
	else {
		b_handled = true;
		return false;
	}
}
