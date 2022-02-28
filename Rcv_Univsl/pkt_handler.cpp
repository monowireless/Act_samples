/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */

#include "pkt_handler.hpp"

pkt_handler_pal g_pkt_pal;
pkt_handler_actsamples g_pkt_actsamples;
pkt_handler_apptwelite g_pkt_apptwelite;
pkt_handler_unknown g_pkt_unknown;

void pkt_handler_pal::disp_detail(packet_rx& rx) {
	// Board type
	Serial << format(":BrdID=%x", pkt.base_info().u8pkt);

	// for ACCEL info.
	if (pkt.size_accel() > 0) {
		Serial << crlf << "  ACCEL(Ct=%d, Info=%d):";
		auto& x = pkt.accel_x();
		auto& y = pkt.accel_y();
		auto& z = pkt.accel_z();
		for(unsigned i = 0; i < pkt.size_accel(); i++) {
			Serial << format(" (%d,%d,%d)", x[i], y[i], z[i]);
		}
	}

	// others
	for(_pal_sensor& x : pkt.sensors()) {
		Serial << crlf << "  ";
		switch(E_SENSOR_TYPE(x.u8type)) {
			case E_SENSOR_TYPE::HALLIC:
				Serial << format("MAG#%d stat=%d", x.u8ext, x.magnet.u8status);
				break;
			case E_SENSOR_TYPE::TEMP:
				Serial << format("TEMP#%d temp=", x.u8ext) << div100(x.temp.i16value) << "C";
				break;
			case E_SENSOR_TYPE::HUM:
				Serial << format("HUMI#%d humd=", x.u8ext) << div100(x.humidity.u16value) << "%";
				break;
			case E_SENSOR_TYPE::ILLUM:
				Serial << format("LIGHT#%d lux=%d", x.u8ext, x.illum.u32value);
				break;
			case E_SENSOR_TYPE::EVENT:
				Serial << format("EVENT#%d param=%02x:%02x:%02x", x.u8ext, x.event.u8data1, x.event.u8data2, x.event.u8data3);
				break;
			case E_SENSOR_TYPE::LED:
				Serial << format("LED#%d stat=%d", x.u8ext, x.led.u8state);
				break;
			case E_SENSOR_TYPE::ADC:
				Serial << format("ADC#%d V=%04dmV", x.u8ext, x.adc.u16volt);
				break;
			case E_SENSOR_TYPE::DIO:
				Serial << format("DIO#%d %x", x.u8ext, x.dio.u32dio_map);
				break;
			case E_SENSOR_TYPE::EEPROM:
				Serial << format("EEPROM#%d stat=%d", x.u8ext, x.eeprom.u8stat);
				break;
			case E_SENSOR_TYPE::FACTOR:
				Serial << format("FACTOR#%d dev=%d factor=%d pkt_id=%d", x.u8ext, x.property.u8device, x.property.u8factor, x.property.u8pktid);
				break;
			default:
				Serial << format("Unknown%d#%d", x.u8type, x.u8ext);
				break;
		}
	}
}

void pkt_handler_actsamples::disp_detail(packet_rx& rx) {
	Serial << ':' << pkt.data.FOURCC << ':' << int(pkt.data.payload.size()) << ':';
	for(auto x: pkt.data.payload) {
		Serial << format("%02X", x);
	}
}

void pkt_handler_apptwelite::disp_detail(packet_rx& rx) {
	switch (pkt.data.u8type) {
		case pkt_apptwelite::TYPE_X81:
			if (1) {
				auto &x = pkt.data.x81;
				
				Serial
					<< (pkt.data.b_lowlatency_tx ? '!' : ':')
					<< format("DI=%c%c%c%c"
							, x.DI_active_mask & 0b0001 ? (x.DI1 ? '1' : '0') : '-'
							, x.DI_active_mask & 0b0010 ? (x.DI2 ? '1' : '0') : '-'
							, x.DI_active_mask & 0b0100 ? (x.DI3 ? '1' : '0') : '-'
							, x.DI_active_mask & 0b1000 ? (x.DI4 ? '1' : '0') : '-'
							)
					<< format(" AD=%04d:%04d:%04d:%04d"
							, x.Adc_active_mask & 0b0001 ? x.u16Adc1 : 9999
							, x.Adc_active_mask & 0b0010 ? x.u16Adc2 : 9999
							, x.Adc_active_mask & 0b0100 ? x.u16Adc3 : 9999
							, x.Adc_active_mask & 0b1000 ? x.u16Adc4 : 9999
							)
					<< format(" TS=%d", pkt.data.u16timestamp)
					;
			}
		break;

		case pkt_apptwelite::TYPE_MSG:
		{
			if (pkt.data.payload_msg.size() > 2) {
				uint8_t *p = pkt.data.payload_msg.begin(), *p_end = pkt.data.payload_msg.end();
				auto &x = pkt.data.msg;
			
				Serial << format(":MSG(Src=%02X,Cmd=%02X,", x.u8dst_addr, x.u8message_id); 
				for(; p != p_end; p++) Serial << format("%02x", *p);
				Serial << ')';
			} else {
				Serial << format(":<err>");
			}
		}
		break;

		default:
			Serial << ":<unsupported>";
			break;	
	}
}
