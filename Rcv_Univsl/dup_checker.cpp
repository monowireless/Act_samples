/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#include "dup_checker.hpp"

#if DEBUG_DUP_CHECKER == 1
template <typename T>
void test_dup_list_all(T &d) {
	Serial << format("[%d]", d._entries().size());
	for (auto&& x : d._entries()) {
		Serial << format("(%08x,s=%02d,t=%04x,v=%d)", x.mKey, x._u16_value, x._u8_ts & 0xFFFF, x._u8_vect_idx);
		Serial.flush();
	}
}

template <typename T>
void test_dup_check(T &d, uint32_t ser_hw, uint8_t ser_pkt, uint32_t ts) {
	bool b_dup = d.check_dup(ser_hw, ser_pkt, ts);
	Serial << crlf << format("!%08x seq=%d ts=%d -> %s", ser_hw, ser_pkt, ts, b_dup ? "DUP" : "accept");
	Serial << '-';
	test_dup_list_all(d);
}

void test_dup_checker() {
	// dup_check<16,7,1000> d;
	dup_check<dup_check<>::MODE_REJECT_SAME_SEQ, 1000, 31, 13> d;
	dup_check<dup_check<>::MODE_REJECT_OLDER_SEQ + 6, 1000, 15, 7> d2;
	
	Serial << crlf << "!dup_checker (reject same), size=" << int(sizeof(d));

	uint32_t ts = 1000;
	test_dup_check(d, 0x12345678, 0x11, ts+=100);
	test_dup_check(d, 0x123456AB, 0x81, ts+=100);
	test_dup_check(d, 0x12345678, 0x11, ts+=100);
	test_dup_check(d, 0x12345C00, 0x41, ts+=100);
	test_dup_check(d, 0x123456AB, 0x81, ts+=100);
	test_dup_check(d, 0x12345C00, 0x41, ts+=100);
	test_dup_check(d, 0x123456AB, 0x81, ts+=100);
	test_dup_check(d, 0x123456AB, 0x83, ts+=100);
	test_dup_check(d, 0x12345C00, 0x42, ts+=100);

	for (int i = 0; i < 31; i++) {
		test_dup_check(d, 0x12345C00 + i, 0x42, ts+i);
	}

	ts += 500;
	Serial << crlf << "timeout ts=" << int(ts);
	d.clean_by_timeout(ts);
	test_dup_list_all(d);

	test_dup_check(d, 0x12345678, 0x12, ts);

	Serial << crlf << "!dup_checker (reject older), size=" << int(sizeof(d2));
	test_dup_check(d2, 0x12345678, 0x11, ts+=100);
	test_dup_check(d2, 0x123456AB, 0x81, ts+=100);
	test_dup_check(d2, 0x12345678, 0x11, ts+=100);
	test_dup_check(d2, 0x12345C00, 0x3C, ts+=100);
	test_dup_check(d2, 0x12345C00, 0x3F, ts+=100);
	test_dup_check(d2, 0x123456AB, 0x81, ts+=100);
	test_dup_check(d2, 0x12345C00, 0x41, ts+=100);
	test_dup_check(d2, 0x123456AB, 0x80, ts+=100);
	test_dup_check(d2, 0x12345C00, 0x40, ts+=100);
	test_dup_check(d2, 0x12345C00, 0x42, ts+=100);
	test_dup_check(d2, 0x12345678, 0x10, ts+=100);
	test_dup_check(d2, 0x12345678, 0x13, ts+=100);

	for (int i = 0; i < 16; i++) {
		test_dup_check(d2, 0x12345C00 + i, 0x42, ts+i);
	}

	ts += 500;
	Serial << crlf << "timeout ts=" << int(ts);
	d2.clean_by_timeout(ts);
	test_dup_list_all(d2);

	test_dup_check(d2, 0x12345678, 0x14, ts);
}
#endif
