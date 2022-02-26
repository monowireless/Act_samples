/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#ifndef SOURCE_PKT_ACTSAMPLES_HPP_
#define SOURCE_PKT_ACTSAMPLES_HPP_

#include <TWELITE>
#include "dup_checker.hpp"

#include "common.h"
#include "pkt_common.hpp"

class pkt_actsamples {
public:
	pkt_actsamples() : data{} {}

	bool analyze(mwx::packet_rx& rx, bool &b_handled);
	void refresh() { return; }

public:
	struct _data_apptwelite : public PktDataCommon {
		uint8_t FOURCC[4];
		smplbuf_u8<80> payload;
	} data;
};

#endif /* SOURCE_PKT_ACTSAMPLES_HPP_ */
