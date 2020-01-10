#include "myAppBhvParent.hpp"

void MY_APP_PARENT::loop() {
	while (Serial.available()) {
		int c = Serial.read();

		Serial << uint8_t(c) << "->";

		switch(c) {
			// case 'a': test_a();	break;
		default:
			Serial.println();
		}
	}
}

void MY_APP_PARENT::receive(mwx::packet_rx& rx) {
	uint8_t msg[4];
	uint32_t lumi;
	uint16_t u16temp, u16humid;

	// expand packet payload (shall match with sent packet data structure, see pack_bytes())
	auto&& np = expand_bytes(rx.get_payload().begin(), rx.get_payload().end(), msg);
	
	// if PING packet, respond pong!
	if (!strncmp((const char*)msg, (const char*)FOURCHARS, 4)) {
		// get rest of data
		expand_bytes(np, rx.get_payload().end(), lumi, u16temp, u16humid);

		// print them
		Serial << format("Packet(%x:%d/lq=%d/sq=%d): ",
							rx.get_addr_src_long(), rx.get_addr_src_lid(),
							rx.get_lqi(), rx.get_psRxDataApp()->u8Seq)
			   << "temp=" << double(int16_t(u16temp)/100.0)
			   << "C humid=" << double(int16_t(u16humid)/100.0)
			   << "% lumi=" << int(lumi)
			   << mwx::crlf << mwx::flush;
    }
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */