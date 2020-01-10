#include "myAppBhvChild.hpp"

MWX_APIRET MY_APP_CHILD::shtc3_start() {
	// start read
	if (auto&& wrt = Wire.get_writer(0x70)) {
		wrt << 0x60; // SHTC3_TRIG_H
		wrt << 0x9C; // SHTC3_TRIG_L
	} else {
		return false;
	}

	return true;
}

MWX_APIRET MY_APP_CHILD::shtc3_read() {
   	// read result
	uint16_t u16temp, u16humd;
	uint8_t u8temp_csum, u8humd_csum;
	if (auto&& rdr = Wire.get_reader(0x70, 6)) {
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

MWX_APIRET MY_APP_CHILD::ltr308als_start() {
	if (auto&& wrt = Wire.get_writer(0x53)) {
		wrt << 0x05;
		wrt << 0x00;
	} else {
		return false;
	}

	if (auto&& wrt = Wire.get_writer(0x53)) {
		wrt << 0x04;
		wrt << 0x32;
	}

	if (auto&& wrt = Wire.get_writer(0x53)) {
		wrt << 0x00;
		wrt << 0x02;
	}

	return true;
}

static MWX_APIRET WireWriteAngGet(uint8_t addr, uint8_t cmd) {
	uint8_t val = 0;

	if (auto&& wrt = Wire.get_writer(addr)) {
		wrt << cmd;
	} else return false;

	if (auto&& rdr = Wire.get_reader(addr, 1)) {
		rdr >> val;
	} else return false;

	return MWX_APIRET(true, val);
}

MWX_APIRET MY_APP_CHILD::ltr308als_read() {
	MWX_APIRET ret;

	// check status
	ret = WireWriteAngGet(0x53, 0x07);
	if(!ret) return false;
	if(!(uint32_t(ret) & 0x08)) return false;

	// read the 24bit value
	uint8_t c1, c2, c3;
	ret = WireWriteAngGet(0x53, 0x0D);
	if(ret) c1 = ret.get_value(); else return false;
	
	ret = WireWriteAngGet(0x53, 0x0E);
	if(ret) c2 = ret.get_value(); else return false;

	ret = WireWriteAngGet(0x53, 0x0F);
	if(ret) c3 = ret.get_value(); else return false;

	uint32_t v = c1 + (c2 << 8) + (c3 << 16);
	v = ((v * 307 + 128) >> 8) + ((v * 51 + 32768) >> 16); // v = v * 0.6 * 2.0

	// store the value
	u32Lumi = v;

	// set stand by mode
	if (auto&& wrt = Wire.get_writer(0x53)) {
		wrt << 0x00;
		wrt << 0x00;
	}

	return MWX_APIRET(true, v);
}

/* Copyright (C) 2019 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */