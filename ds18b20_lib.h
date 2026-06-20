/*
 * An AVR library for communication with DS18B20 1-Wire Digital Thermometer
 */

/******************************************************************************
 *  * Copyright � 2022, pvglab (pvg@poczta.fm).
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License v3 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#ifndef DS18B20_LIB_H
#define DS18B20_LIB_H

#include "dallas_one_wire.h"

#define DS18B20_CONVERT_T_COMMAND 			0x44
#define DS18B20_READ_SCRATCHPAD_COMMAND 	0xBE
#define DS18B20_WRITE_SCRATCHPAD_COMMAND 	0x4E
#define DS18B20_COPY_SCRATCHPAD_COMMAND 	0x48
#define DS18B20_RECALL_CONFIG_COMMAND 		0xB8
#define DS18B20_ALARM_SEARCH_COMMAND		0xEC

typedef enum {
  DS18B20_SCRATCHPAD_CRC_OK 	= 0x00,
  DS18B20_SCRATCHPAD_CRC_ERROR	= 0x01
} DS18B20_STATUS_t;

typedef enum {
  DS18B20_CONFIG_TMAX 			= 0x01,
  DS18B20_CONFIG_TMIN 			= 0x02,
  DS18B20_CONFIG_RESOLUTION 	= 0x04,
  DS18B20_CONFIG_TO_EEPROM 		= 0x08,
  DS18B20_CONFIG_FROM_EEPROM 	= 0x10,
  DS18B20_CONFIG_FROM_SCRATCHPAD= 0x00,
  DS18B20_WAIT 					= 0x80,
  DS18B20_NO_WAIT             	= 0x00
} DS18B20_MODES_t;

typedef enum {
  DS18B20_RESOLUTION_9BIT 	= 0b00011111,
  DS18B20_RESOLUTION_10BIT 	= 0b00111111,
  DS18B20_RESOLUTION_11BIT 	= 0b01011111,
  DS18B20_RESOLUTION_12BIT 	= 0b01111111
} DS18B20_RESOLUTIONS_t;

typedef struct {
	int8_t temperature_max;
	int8_t temperature_min;
    DS18B20_RESOLUTIONS_t resolution;
} DS18B20_CONFIG_t;

typedef int8_t DS18B20_TEMPERATURE_t[2];

void DS18B20convertTemperature(DALLAS_IDENTIFIER_t* device, DS18B20_MODES_t mode);
DS18B20_STATUS_t DS18B20readTemperature(DALLAS_IDENTIFIER_t* device, DS18B20_TEMPERATURE_t temperature);
DS18B20_STATUS_t DS18B20readConfig(DALLAS_IDENTIFIER_t* device, DS18B20_CONFIG_t* config, DS18B20_MODES_t mode);
DS18B20_STATUS_t DS18B20setConfig(DALLAS_IDENTIFIER_t* device, DS18B20_CONFIG_t* config, DS18B20_MODES_t mode);
#define DS18B20searchAlarm(s1) (dallasSearch(s1, DS18B20_ALARM_SEARCH_COMMAND))


#endif /* DS18B20_LIB_H */
