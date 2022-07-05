// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <STG_STD>
#include <SM_SIMPLE>

/*** sensor select, define either of USE_SHTC3 or USE_SHT40  */
// use SHTC3 (TWELITE PAL)
#define USE_SHTC3
// use SHT40 (TWELITE ARIA)
#undef USE_SHT40

/*** Sensor Driver */
#if defined(USE_SHTC3)
// for SHTC3
struct SHTC3 {
	uint8_t I2C_ADDR;
	uint8_t CONV_TIME;

	bool setup() {
		// here, initialize some member vars instead of constructor.
		I2C_ADDR = 0x70;
		CONV_TIME = 10;
		return true;
	}

	bool begin() {
		// start read
		if (auto&& wrt = Wire.get_writer(I2C_ADDR)) {
			wrt << 0x60; // SHTC3_TRIG_H
			wrt << 0x9C; // SHTC3_TRIG_L
		} else {
			return false;
		}

		return true;
	}

	int get_convtime() {
		return CONV_TIME;
	}

	bool read(int16_t &i16Temp, int16_t &i16Humd) {
		// read result
		uint16_t u16temp, u16humd;
		uint8_t u8temp_csum, u8humd_csum;
		if (auto&& rdr = Wire.get_reader(I2C_ADDR, 6)) {
			rdr >> u16temp;
			rdr >> u8temp_csum; // skip the crc8 check
			rdr >> u16humd;
			rdr >> u8humd_csum; // skip the crc8 check
		} else {
			return false;
		}

		// check CRC and save the values
		if (   (CRC8_u8CalcU16(u16temp, 0xff) == u8temp_csum)
			&& (CRC8_u8CalcU16(u16humd, 0xff) == u8humd_csum))
		{
			i16Temp = (int16_t)(-4500 + ((17500 * int32_t(u16temp)) >> 16));
			i16Humd = (int16_t)((int32_t(u16humd) * 10000) >> 16);
		} else {
			return false;
		}

		return true;
	}
} sensor_device;
#endif

// for SHT40
#if defined(USE_SHT40)
struct SHT40 {
	uint8_t I2C_ADDR;
	uint8_t CONV_TIME;

	bool setup() {
		// here, initialize some member vars instead of constructor.
		I2C_ADDR = 0x44;
		CONV_TIME = 10;

		return true;
	}

	bool begin() {
		// start read
		if (auto&& wrt = Wire.get_writer(I2C_ADDR)) {
			wrt << 0xF6; // normal prescision.
		} else {
			return false;
		}

		return true;
	}

	int get_convtime() {
		return CONV_TIME;
	}

	bool read(int16_t &i16Temp, int16_t &i16Humd) {
		uint16_t u16temp, u16hum;
		uint8_t u8temp_csum, u8hum_csum;
		if(auto&& x = Wire.get_reader(I2C_ADDR, 6)){
			x >> u16temp;
			x >> u8temp_csum;
			x >> u16hum;
			x >> u8hum_csum;
		}else return false;

		if(CRC8_u8CalcU16(u16temp, 0xFF) != u8temp_csum) return false;
		if(CRC8_u8CalcU16(u16hum, 0xFF) != u8hum_csum) return false;

		i16Temp = (int16_t)(-4500 + ((17500*int32_t(u16temp)) >> 16) );
		i16Humd = (int16_t)(-600 + ((12500*int32_t(u16hum)) >> 16) );

		return true;
	}
} sensor_device;
#endif

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// option bits
uint32_t OPT_BITS = 0;
// logical id
uint8_t LID = 0;

// application use
const char FOURCHARS[] = "BTH1";

// stores sensor value
struct {
	int16_t i16temp;
	int16_t i16humid;
} sensor;

// application state defs
enum class STATE : uint8_t {
	INTERACTIVE = 255,
	INIT = 0,
	SENSOR,
	TX,
	TX_WAIT_COMP,
	GO_SLEEP
};

// simple state machine.
SM_SIMPLE<STATE> step;

/*** Local function prototypes */
void sleepNow();

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	/// init vars or objects
	step.setup(); // initialize state machine
	
	/// load board and settings objects
	auto&& set = the_twelite.settings.use<STG_STD>(); // load save/load settings(interactive mode) support
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>(); // load network support

	/// configure settings
	// configure settings
	set << SETTINGS::appname(FOURCHARS);
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);

	// if SET(DIO12)=LOW is detected, start with intaractive mode.
	if (digitalRead(PIN_DIGITAL::DIO12) == PIN_STATE::LOW) {
		set << SETTINGS::open_at_start();
		step.next(STATE::INTERACTIVE);
		return;
	}

	// load values
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	
	// LID is configured DIP or settings.
	LID = set.u8devid(); // 2nd is setting.
	if (LID == 0) LID = 0xFE; // if still 0, set 0xFE (anonymous child)

	/// configure system basics
	the_twelite << set; // apply settings (from interactive mode)

	/// configure network
	nwk << set; // apply settings (from interactive mode)
	nwk << NWK_SIMPLE::logical_id(LID); // set LID again (LID can also be configured by DIP-SW.)	

	/*** BEGIN section */
	Wire.begin(); // start two wire serial bus.
	
	// let the TWELITE begin!
	the_twelite.begin();

	// setup sensor device
	sensor_device.setup();

	/*** INIT message */
	Serial << "--- TEMP&HUMID:" << FOURCHARS << " ---" << mwx::crlf;
	Serial	<< format("-- app:x%08x/ch:%d/lid:%d"
					, the_twelite.get_appid()
					, the_twelite.get_channel()
					, nwk.get_config().u8Lid
				)
			<< mwx::crlf;
	Serial 	<< format("-- pw:%d/retry:%d/opt:x%08x"
					, the_twelite.get_tx_power()
					, nwk.get_config().u8RetryDefault
					, OPT_BITS
			)
			<< mwx::crlf;
}

// wakeup procedure
void wakeup() {
	Serial	<< mwx::crlf
			<< "--- TEMP&HUMID:" << FOURCHARS << " wake up ---"
			<< mwx::crlf
			<< "..start sensor capture again."
			<< mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
	do {
		switch (step.state()) {
		case STATE::INTERACTIVE:
		break;
		
		case STATE::INIT: // starting state
			// start sensor capture
			sensor_device.begin();
			Serial << sensor_device.get_convtime() << mwx::crlf;
			step.set_timeout(sensor_device.get_convtime()); // set timeout
			step.next(STATE::SENSOR);
		break;

		case STATE::SENSOR: // starting state
			if (step.is_timeout()) {
				// the sensor data should be ready (wait some)
				sensor_device.read(sensor.i16temp, sensor.i16humid);

				Serial << "..finish sensor capture." << mwx::crlf
					<< "     : temp=" << div100(sensor.i16temp) << 'C' << mwx::crlf
					<< "       humd=" << div100(sensor.i16humid) << '%' << mwx::crlf
					;
				Serial.flush();

				step.next(STATE::TX);
			}
		break;

		case STATE::TX:
			step.next(STATE::GO_SLEEP); // set default next state (for error handling.)

			// get new packet instance.
			if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
				// set tx packet behavior
				pkt << tx_addr(0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
					<< tx_retry(0x1) // set retry (0x1 send two times in total)
					<< tx_packet_delay(0, 0, 2); // send packet w/ delay

				// prepare packet payload
				pack_bytes(pkt.get_payload() // set payload data objects.
					, make_pair(FOURCHARS, 4)  // just to see packet identification, you can design in any.
					, uint16_t(sensor.i16temp)
					, uint16_t(sensor.i16humid)
				);

				// do transmit
				MWX_APIRET ret = pkt.transmit();
				Serial << "..transmit request by id = " << int(ret.get_value()) << '.' << mwx::crlf << mwx::flush;

				if (ret) {
					step.clear_flag(); // waiting for flag is set.
					step.set_timeout(100); // set timeout
					step.next(STATE::TX_WAIT_COMP);
				}
			}
		break;

		case STATE::TX_WAIT_COMP: // wait for complete of transmit
			if (step.is_timeout()) { // maybe fatal error.
				the_twelite.reset_system();
			}
			if (step.is_flag_ready()) { // when tx is performed
				Serial << "..transmit complete." << mwx::crlf;
				Serial.flush();
				step.next(STATE::GO_SLEEP);
			}
		break;

		case STATE::GO_SLEEP: // now sleeping
			sleepNow();
		break;

		default: // never be here!
			the_twelite.reset_system();
		}
	} while(step.b_more_loop()); // if state is changed, loop more.
}

// when finishing data transmit, set the flag.
void on_tx_comp(mwx::packet_ev_tx& ev, bool_t &b_handled) {
	step.set_flag(ev.bStatus);
}

// perform sleeping
void sleepNow() {
	step.on_sleep(false); // reset state machine.

	// randomize sleep duration.
	uint32_t u32ct = 1750 + random(0,500);

	// output message
	Serial << "..sleeping " << int(u32ct) << "ms." << mwx::crlf;
	Serial.flush(); // wait until all message printed.
	
	// do sleep.
	the_twelite.sleep(u32ct);
}

/* Copyright (C) 2019-2022 Mono Wireless Inc. All Rights Reserved. *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE     *
 * AGREEMENT).                                                     */