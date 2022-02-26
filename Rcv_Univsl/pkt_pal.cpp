/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#include "pkt_pal.hpp"

using namespace mwx;

inline bool _pal_sensor_hallic::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 2 > e) return false;
	ext = 0x00;
	(void)G_BYTE(p);
	u8status = G_BYTE(p);
	return true;
}

inline bool _pal_sensor_temp::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 3 > e) return false;
	ext = 0x00;
	(void)G_BYTE(p);
	i16value = (int16_t)G_WORD(p);
	return (i16value < -32760) ? false : true;
}

inline bool _pal_sensor_humidity::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 3 > e) return false;
	ext = 0x00;
	(void)G_BYTE(p);
	u16value = G_WORD(p);
	return (u16value == 0x8001 || u16value == 0x8000) ? false : true;
}

inline bool _pal_sensor_illum::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 5 > e) return false;
	ext = 0x00;
	(void)G_BYTE(p);
	u32value = G_DWORD(p);
	return (u32value == 0xFFFFFFFF) ? false : true;
}

inline bool _pal_event::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 5 > e) return false;

	ext = G_BYTE(p);
	u8event = G_BYTE(p);
	u8data1 = G_BYTE(p);
	u8data2 = G_BYTE(p);
	u8data3 = G_BYTE(p);

	return true;
}

inline bool _pal_led::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 1 > e) return false;

	ext = 1;
	u8state = G_BYTE(p);

	return true;
}

inline bool _pal_adc::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 1 > e) return false;
	ext = G_BYTE(p);

	if(ext == 0x01 || ext == 0x08) { // for version compatibility, 0x1 of ext is moved to 0x08.
		ext = 0x08;
		if (p + 1 > e) return false;
		uint8_t u8pwr = G_BYTE(p);
		u16volt = DecodeVolt(u8pwr);
	} else {
		if (p + 2 > e) return false;
		u16volt = G_WORD(p);
	}

	return true;
}


inline bool _pal_dio::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 1 > e) return false;
	ext = G_BYTE(p);

	if (ext <= 8) {
		if (p + 1 > e) return false;
		u32dio_map = G_BYTE(p);
	} else if (ext <= 16) {
		if (p + 2 > e) return false;
		u32dio_map = G_WORD(p);
	} else {
		if (p + 4 > e) return false;
		u32dio_map = G_DWORD(p);
	}

	return true;
}

inline bool _pal_eeprom::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 2 > e) return false;
	ext = G_BYTE(p);
	u8stat = G_BYTE(p);

	return true;
}

inline bool _pal_property::analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext) {
	if (p + 3 > e) return false;
	ext = 0x00;

	u8pktid = G_BYTE(p);
	u8device = G_BYTE(p);
	u8factor = G_BYTE(p);

	return true;
}

/**
 * analyze packet payload (array of various sensor data) and put them into map structure.
 */
bool pkt_pal::b_pal_packet_analyze_payload(const uint8_t* p, const uint8_t* p_end) {
	// refresh sensor elements container
	_vect_pal_sensors.clear();
	_map_pal_sensors.clear();
	_accel_x.clear();
	_accel_y.clear();
	_accel_z.clear();

	if (p == p_end) return false;

	uint8_t n_sensors;
	p = expand_bytes(p, p_end, n_sensors);
	if (p == nullptr) return false;

	while(n_sensors) {
		uint8_t u8sns_type;

		// read the sns type and block length in byte
		if ((p = expand_bytes(p, p_end, u8sns_type)) == nullptr) return false; // error

		if(u8sns_type == uint8_t(E_SENSOR_TYPE::ACCEL)) {
			// for accel data, 2 entries are packed in 9bytes and 10 or 16 entries in total.
			(void)G_BYTE(p);
			uint8_t u8num_samples = G_BYTE(p);
			_accel_sample_info = G_BYTE(p);
			G_BYTE(p);

			// size check
			if (u8num_samples > _accel_x.capacity()) return false;

			// reserve area
			_accel_x.reserve(u8num_samples);
			_accel_y.reserve(u8num_samples);
			_accel_z.reserve(u8num_samples);

			unsigned j = 0;
			while( j < u8num_samples ){
				// expand 9bytes pack to 12bit x 3axis x 2blocks.
				int16_t v[6];
				for (int k = 0; k < 3; k++) {
					int a = G_BYTE(p);
					int b = G_BYTE(p);
					int c = G_BYTE(p);

					v[k]   = (a << 4) | (b >> 4);
					v[k+1] = (b << 8) | c;
				}
				for (auto &x : v) { x = (x << 4) >> 4; }

				_accel_x[j] = v[0];
				_accel_y[j] = v[1];
				_accel_z[j] = v[2];
				_accel_x[j+1] = v[3];
				_accel_y[j+1] = v[4];
				_accel_z[j+1] = v[5];

				j += 2;
			}
		} else {
			// simple structure (one data block, one element)
			if (_vect_pal_sensors.full()) return false;  // check data full before allocating.
			auto&& x = _vect_pal_sensors.push_back();   // allocate an object  (no copy op.)

			x.u8type = u8sns_type;

			bool b_success = false;

			switch(E_SENSOR_TYPE(u8sns_type)) {
			case E_SENSOR_TYPE::HALLIC:	b_success = x.magnet.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::TEMP:   b_success = x.temp.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::HUM:    b_success = x.humidity.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::ILLUM:  b_success = x.illum.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::EVENT:  b_success = x.event.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::LED:    b_success = x.led.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::ADC:    b_success = x.adc.analyze(p, p_end, x.u8ext);
				if (x.u8ext == 8) data.u16volt = x.adc.u16volt; // module voltage
				break;
			case E_SENSOR_TYPE::DIO:    b_success = x.dio.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::EEPROM: b_success = x.eeprom.analyze(p, p_end, x.u8ext); break;
			case E_SENSOR_TYPE::FACTOR: b_success = x.property.analyze(p, p_end, x.u8ext); break;
			default:
				break; // stop analysing
			}

			if (!b_success) {
				_vect_pal_sensors.pop_back(); // dis-allocate an object.
				break; // exit while loop
			}
		}

		n_sensors--;
	}

	return true;
}

bool pkt_pal::b_pal_packet_analyze_body(packet_rx& rx) {
	const uint8_t *p = rx.get_payload().begin();
	const uint8_t *p_end = rx.get_payload().end();

	// clean data area
	memset(&data, 0, sizeof(data));
	
	// this must be via network layer
	if (!rx.get_psRxDataApp()->bNwkPkt) return false;

	// check cmd data. (0..7, should be 0:TOCONET_PACKET_CMD_APP_DATA)
	if (rx.get_psRxDataApp()->u8Cmd != TOCONET_PACKET_CMD_APP_DATA) return false;

	p = expand_bytes(p, p_end, data.u8b);
	if (p == nullptr) return false;

	if(data.packet_type() != 'T' && data.packet_type() != 'R') return false;

	// store base info
	data.u32addr = rx.get_psRxDataApp()->u32SrcAddr;

	// receiving from router device
	if (data.u8b == 'R') {
		p = expand_bytes(p, p_end
			, data.u32addr_1st
			, data.u8lqi_1st
			);
		if (p == nullptr) return false;
	}

	// other header info
	p = expand_bytes(p, p_end
			, data.u8id   // LID
			, data.u16fct // Seq number
			, data.u8pkt  // kind of packet
			);

	// for PktDataCommon
	data.u8type = 0;
	data.u32addr_src = data.u32addr;
	data.u32addr_dst = 0xFFFFFFFF; //dummy data
	data.u8addr_src = data.u8id;
	data.u8addr_dst = 0xFF; // dummy data
	data.u8lqi = rx.get_lqi();

	// copy the rest of payload
	if (p_end - p == 0) return false;
	b_pal_packet_analyze_payload(p, p_end);

	return true;
}


/**
 * Packet analyze for PAL.
 * @param rx        rx packet information.
 * @param b_handled true when the packet is handled and should not process further.
 * @return true     the packet is valid for application use.
 * @return false    the packet should be rejected (e.g. different structure, duplicated, etc.)
 */
bool pkt_pal::analyze(packet_rx& rx, bool& b_handled) {
	bool b_success = false;

	b_handled = false;

	if (b_pal_packet_analyze_body(rx)) {
		if (_vect_pal_sensors.size() > 0) {
			for(auto&&x : _vect_pal_sensors) {
				x.mKey = _pal_map_key(x.u8type, x.u8ext);
				_map_pal_sensors.insert(x); // create hash table
			}
			b_success = true;
		}
	}

	if(b_success) b_handled = true;
	return b_success;
}
