/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#ifndef SOURCE_PKT_PAL_HPP_
#define SOURCE_PKT_PAL_HPP_

#include <TWELITE>

#include <EASTL/intrusive_hash_map.h>
#include <EASTL/utility.h>
#include <EASTL/fixed_vector.h>

#include "common.h"
#include "pkt_common.hpp"

/********************************************************************************
 * definition of each sensor object
 ********************************************************************************/

/**
 * Sensor ID Enum
 */
enum class E_SENSOR_TYPE : uint8_t {
    HALLIC	= 0x00,
    TEMP	= 0x01,
    HUM		= 0x02,
    ILLUM	= 0x03,
    ACCEL	= 0x04,
    EVENT	= 0x05,
    LED		= 0x06,

    ADC		= 0x30,
    DIO		= 0x31,
    EEPROM	= 0x32,
    REPLY	= 0x33,
    FACTOR	= 0x34,

	VOID    = 0xFF
};


/**
 * PAL sensor data structure
 */
struct _s_pal_packet : public PktDataCommon {
	uint8_t u8b; // packet type
	uint8_t u8lqi_1st;
	uint8_t u8id;
	uint8_t u8pkt;
	uint16_t u16fct;
	uint32_t u32addr;
	uint32_t u32addr_1st;
	uint32_t u32addr_rcvr;

	uint8_t packet_type() { return u8b & 0x7F; }
	bool is_pal_packet() { return u8b & 0x80; }
};

/********************************************************************************
 * definition of each sensor object
 ********************************************************************************/
/**
 * PAL hall IC (magnet)
 */
struct _pal_sensor_hallic {
	uint8_t u8status; // 0: no magnet, 1 or 2: magnet close

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL temperature sensor
 */
struct _pal_sensor_temp {
	int16_t i16value; // in C x 100 (2451 -> 24.51 degC)

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL humidity sensor
 */
struct _pal_sensor_humidity {
	uint16_t u16value; // in % x100 (5452 -> 54.52%)

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL light sensor
 */
struct _pal_sensor_illum {
	uint32_t u32value; // in Lux

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL accelerometer sensor.
 */
struct _pal_sensor_accel_xyz {
	int16_t i16x; // X axis in mG
	int16_t i16y; // Y
	int16_t i16z; // Z
};

/**
 * PAL event: PAL event information
 */
struct _pal_event {
	uint8_t u8event; // Event code
	uint8_t u8data1; // param1
	uint8_t u8data2; // param2
	uint8_t u8data3; // param3

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL LED: LED status information.
 */
struct _pal_led {
	uint8_t u8state;

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL ADC: A/D converter measurement result.
 */
struct _pal_adc {
	uint16_t u16volt; // volt in mV.

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL DIO: digital port information
 */
struct _pal_dio {
	uint32_t u32dio_map; // bitmap of DIO ports.

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL EEPROM: internal use.
 */
struct _pal_eeprom {
	uint8_t u8stat;

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};

/**
 * PAL property: additional information of sent packet.
 */
struct _pal_property {
	uint8_t u8pktid;
	uint8_t u8device;
	uint8_t u8factor;

	inline bool analyze(const uint8_t*&p, const uint8_t*e, uint8_t &ext);
};


/********************************************************************************
 * definition for map structure
 ********************************************************************************/
/**
 * Key data type for map structure.
 */
struct _pal_map_key {
	uint32_t _u32_ser;
	_pal_map_key(uint8_t type = 0, uint8_t ext = 0) { _u32_ser = (type << 8) + ext; }
	uint32_t get_ser() const  { return _u32_ser; }
};

/**
 * required == operator for _pal_map_key.
 */
static inline bool operator==(const _pal_map_key& lhs, const _pal_map_key& rhs) {
	return lhs.get_ser() == rhs.get_ser();
}

/**
 * Serial number generator for hash calculation.
 */
struct _pal_map_hash {
    size_t operator()(const _pal_map_key& key) const { return (size_t)(key.get_ser()); }
};


/**
 * PAL sensor element data structure.
 */
struct _pal_sensor : public eastl::intrusive_hash_node_key<_pal_map_key> {
	uint8_t u8type;
	uint8_t u8ext;

	union {
		struct _pal_sensor_hallic magnet;
		struct _pal_sensor_temp temp;
		struct _pal_sensor_humidity humidity;
		struct _pal_sensor_illum illum;
		struct _pal_sensor_accel_xyz accel_xyz;
		struct _pal_event event;
		struct _pal_led led;
		struct _pal_adc adc;
		struct _pal_dio dio;
		struct _pal_eeprom eeprom;
		struct _pal_property property;
	};

	operator bool() { return (u8type != 0xFF); }
};


/********************************************************************************
 * definition for PAL data structure
 ********************************************************************************/
class pkt_pal {
	static const unsigned N_ENTRIES = 15;
	static const unsigned N_ENTRIES_ACCEL = 16;
	static const unsigned N_BUCKET_HASH = 13;

public:
	using tvect = eastl::fixed_vector<_pal_sensor, N_ENTRIES, false>;
	using tvect_accel = eastl::fixed_vector<int16_t, N_ENTRIES_ACCEL, false>;
	using tmap = eastl::intrusive_hash_map<_pal_map_key, _pal_sensor, N_BUCKET_HASH, _pal_map_hash>;

private:


	// pool of pal sensor elements.
	tvect _vect_pal_sensors;
	tmap _map_pal_sensors;
	_pal_sensor _void_entry;

	// xyz data
	tvect_accel _accel_x;
	tvect_accel _accel_y;
	tvect_accel _accel_z;
	uint8_t _accel_sample_info;

	bool b_pal_packet_analyze_body(packet_rx& rx);
	bool b_pal_packet_analyze_payload(const uint8_t* p, const uint8_t* p_end);

public:
	pkt_pal()
		: _vect_pal_sensors(), _map_pal_sensors()
		, _accel_x(), _accel_y(), _accel_z(), _accel_sample_info(0)
	{
		_void_entry.u8type = 0xFF;
	}

	bool analyze(mwx::packet_rx& rx, bool& b_handled);

	_s_pal_packet& base_info() {
		return data;
	}

	_pal_sensor& find_sensor(_pal_map_key key) {
		auto&& iter = _map_pal_sensors.find(key);
		if (iter != _map_pal_sensors.end()) {
			return *iter;
		} else {
			return _void_entry;
		}
	}
	tvect &sensors() { return _vect_pal_sensors; }
	size_t size_sensors() { return _vect_pal_sensors.size(); }

	tvect_accel &accel_x() { return _accel_x; }
	tvect_accel &accel_y() { return _accel_y; }
	tvect_accel &accel_z() { return _accel_z; }
	size_t size_accel() { return _accel_x.size(); }

public:
	// packet base information
	_s_pal_packet data;
};

#endif /* SOURCE_PKT_PAL_HPP_ */
