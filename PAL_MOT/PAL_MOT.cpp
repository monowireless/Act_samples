// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <PAL_MOT>

/*** Config part */
// application ID
const uint32_t APP_ID = 0x1234abcd;

// channel
const uint8_t CHANNEL = 13;

/*** function prototype */
void sleepNow();

/*** application use */
const uint8_t FOURCHARS[] = "PMT1";

bool b_transmit;
uint16_t txid[2]; // transmit packet ID
const uint8_t MAX_SAMP_IN_PKT = 15; // max samples count in one packet.

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */

	// board
	auto&& brd = the_twelite.board.use<PAL_MOT>();
	brd.set_led(LED_TIMER::BLINK, 100);

	// the twelite main class
	the_twelite
		<< TWENET::appid(APP_ID)     // set application ID (identify network group)
		<< TWENET::channel(CHANNEL); // set channel (pysical channel)

	// Register Network
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	nwk	<< NWK_SIMPLE::logical_id(0xFE); // set Logical ID. (0xFE means a child device with no ID)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!
	brd.sns_MC3630.begin(SnsMC3630::Settings(
		SnsMC3630::MODE_LP_14HZ, SnsMC3630::RANGE_PLUS_MINUS_4G)); // begin MC3630

	/*** INIT message */
	Serial << "--- PAL_MOT(Cont):" << FOURCHARS << " ---" << mwx::crlf;
}

/*** the first call after finishing setup() */
void begin() { 
	// sleep immediately, waiting for the first capture.
	sleepNow();
}

/*** when waking up */
void wakeup() {
	Serial << "--- PAL_MOT(Cont):" << FOURCHARS << " wake up ---" << mwx::crlf;

	b_transmit = false;
	txid[0] = 0xFFFF;
	txid[1] = 0xFFFF;
}

/*** loop procedure (called every event) */
void loop() {
	auto&& brd = the_twelite.board.use<PAL_MOT>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();

	if (!b_transmit) {
		if (!brd.sns_MC3630.available()) {
			Serial << "..sensor is not available." << mwx::crlf << mwx::flush;
			sleepNow();
		}

		// send a packet
		Serial << "..finish sensor capture." << mwx::crlf
			<< "  seq=" << int(brd.sns_MC3630.get_que().back().t) 
			<< "/ct=" << int(brd.sns_MC3630.get_que().size());

		// calc average in the queue.
		{
			int32_t x = 0, y = 0, z = 0;
			for (auto&& v: brd.sns_MC3630.get_que()) {
				x += v.x;
				y += v.y;
				z += v.z;
			}
			x /= brd.sns_MC3630.get_que().size();
			y /= brd.sns_MC3630.get_que().size();
			z /= brd.sns_MC3630.get_que().size();

			Serial << format("/ave=%d,%d,%d", x, y, z) << mwx::crlf;
		}

		// mostly, coming samples are split into two packets.
		for (int ip = 0; ip < 2; ip++) {
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {		
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay
			
				// prepare packet (first)
				uint8_t siz = (brd.sns_MC3630.get_que().size() >= MAX_SAMP_IN_PKT)
									? MAX_SAMP_IN_PKT : brd.sns_MC3630.get_que().size();
				uint16_t seq = brd.sns_MC3630.get_que().front().t;
			
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, seq // add sequence number of capture data of the first entry.
					, siz // count of entry in the packet.
				);

				// store sensor data (36bits into 5byts, alas 4bits are not used...)
				for (int i = 0; i < siz; i++) {
					auto&& v = brd.sns_MC3630.get_que().front();
					uint32_t v1;

					// if RANGE_PLUS_MINUS_4G is applied, the value range is -4000 to 4000,
					// so need to be half if send in 12bits.
					v1  = ((uint16_t(v.x/2) & 4095) << 20)  // X:12bits
						| ((uint16_t(v.y/2) & 4095) <<  8)  // Y:12bits
						| ((uint16_t(v.z/2) & 4095) >>  4); // Z:8bits from MSB
					uint8_t v2 = (uint16_t(v.z/2) & 255);   // Z:4bits from LSB
					pack_bytes(pkt.get_payload(), v1, v2); // add into pacekt entry.
					brd.sns_MC3630.get_que().pop(); // pop an entry from queue.
				}

				// perform transmit
				MWX_APIRET ret = pkt.transmit();

				if (ret) {
					Serial << "..txreq(" << int(ret.get_value()) << ')';
					txid[ip] = ret.get_value() & 0xFF;
				} else {
					sleepNow();
				}
			}
		}

		// finished tx request
		b_transmit = true;
	} else {
		if(		the_twelite.tx_status.is_complete(txid[0])
			 && the_twelite.tx_status.is_complete(txid[1]) ) {

			sleepNow();
		}
	}
}

void sleepNow() {
	Serial << mwx::crlf << "..sleeping now.." << mwx::crlf << mwx::flush;
	pinMode(PAL_MOT::PIN_SNS_INT, WAKE_FALLING);
	the_twelite.sleep(60000, false); // set longer sleep (PAL must wakeup every 60sec, just in the case)
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */