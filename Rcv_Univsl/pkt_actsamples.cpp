/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#include <TWELITE>
#include "pkt_actsamples.hpp"

using namespace mwx;

bool pkt_actsamples::analyze(mwx::packet_rx& rx, bool &b_handled) {
	b_handled = false;

	// clean data area
	mwx::pnew(data);

	data.u32addr_src = rx.get_addr_src_long();
	data.u8addr_src = rx.get_addr_src_lid();
	data.u32addr_dst = rx.get_addr_dst();

	data.u8lqi = rx.get_lqi();

	const uint8_t *p = rx.get_payload().begin();
	const uint8_t *p_end = rx.get_payload().end();

	p = expand_bytes(p, p_end, data.FOURCC);
	if (p == nullptr) return false;

	// save payload information.
	for(; p != p_end; p++) data.payload.append(*p);

	// exit success
	b_handled = true;
	return true;
}
