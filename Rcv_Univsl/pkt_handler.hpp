/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#ifndef SOURCE_PKT_HANDLER_HPP_
#define SOURCE_PKT_HANDLER_HPP_

#include "common.h"

#include "dup_checker.hpp"
#include "pkt_common.hpp"
#include "pkt_pal.hpp"
#include "pkt_apptwelite.hpp"
#include "pkt_actsamples.hpp"

// common procedure for packet_handelr_xxx, using CRTP.
template <class D>
struct pkt_handler {
	D& self() { return static_cast<D&>(*this); }
	bool analyze(packet_rx& rx, bool &b_handled) {
		return self().pkt.analyze(rx, b_handled);
	}
	void display(packet_rx& rx) {
		Serial
			<< crlf
			<< format("!PKT_%s(%03d-%08x/S=%d/L=%03d/V=%04d)"
					, self().get_label_packet_type()
					, self().pkt.data.u8addr_src
					, self().pkt.data.u32addr_src
					, rx.get_psRxDataApp()->u8Seq
					, rx.get_lqi()
					, self().pkt.data.u16volt
					);

		self().disp_detail(rx);
	}
	void refresh() {
		self()._refresh();
	}
};

// packet analyzer for PAL.
class pkt_handler_pal : public pkt_handler<pkt_handler_pal> {
	friend class pkt_handler<pkt_handler_pal>;
	pkt_pal pkt; // handler object
	void disp_detail(packet_rx& rx);
	const char* get_label_packet_type() { return "PAL"; }
	void _refresh() { return; }
public:
	pkt_handler_pal() : pkt() {}
};

// packet analyzer for Act Samples
class pkt_handler_actsamples : public pkt_handler<pkt_handler_actsamples> {
	friend class pkt_handler<pkt_handler_actsamples>;
	pkt_actsamples pkt;
	void disp_detail(packet_rx& rx);
	const char* get_label_packet_type() { return "Act"; }
	void _refresh() { pkt.refresh(); }
public:
	pkt_handler_actsamples() : pkt() {}
};

// packet analyzer for App_Twelite
class pkt_handler_apptwelite : public pkt_handler<pkt_handler_apptwelite> {
	friend class pkt_handler<pkt_handler_apptwelite>;
	pkt_apptwelite pkt;
	void disp_detail(packet_rx& rx);
	const char* get_label_packet_type() { return "AppTwelite"; }
	void _refresh() { pkt.refresh(); }
public:
	pkt_handler_apptwelite() : pkt() {}
};

// for unknown packet
class pkt_handler_unknown : public pkt_handler<pkt_handler_unknown> {
	friend class pkt_handler<pkt_handler_unknown>;
	struct pkt_unknown {
		struct : public PktDataCommon {
		} data;

		bool analyze(packet_rx& rx, bool &b_handled) {
			data.u32addr_src = rx.get_addr_src_long();
			data.u8addr_src = rx.get_addr_src_lid();
			data.u32addr_dst = rx.get_addr_dst();
			data.u8lqi = rx.get_lqi();

			b_handled = true;
			return true;
		}
	} pkt;
	void disp_detail(packet_rx& rx) {
		Serial << ':';
		for (uint8_t x: rx.get_payload()) { Serial << format("%02X", x); }
	}
	const char* get_label_packet_type() { return "Unknown"; }
	void _refresh() { return; }
public:
	pkt_handler_unknown() : pkt() {}
};

extern pkt_handler_pal g_pkt_pal;
extern pkt_handler_actsamples g_pkt_actsamples;
extern pkt_handler_apptwelite g_pkt_apptwelite;
extern pkt_handler_unknown g_pkt_unknown;

#endif /* SOURCE_PKT_HANDLER_HPP_ */
