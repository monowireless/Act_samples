/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#ifndef SOURCE_PKT_COMMON_HPP_
#define SOURCE_PKT_COMMON_HPP_

struct PktDataCommon {
	/**
	 * Data type ID (depending on data structure)
	 */
	uint8_t u8type;

	/**
	 * LQI value
	 */
	uint8_t u8lqi;


	/**
	 * source address (logical ID)
	 */
	uint8_t u8addr_src;

	/**
	 * destination address (logical ID)
	 */
	uint8_t u8addr_dst;

	/**
	 * Module voltage (if available)
	 */
	uint16_t u16volt;

	/**
	 * used for any.
	 */
	uint16_t u16aux;

	/**
	 * source address (Serial ID)
	 */
	uint32_t u32addr_src;

	/**
	 * dest address (Serial ID)
	 */
	uint32_t u32addr_dst;
};



#endif /* SOURCE_PKT_COMMON_HPP_ */
