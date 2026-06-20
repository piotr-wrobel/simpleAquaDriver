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

#include <stdint.h>
#include <avr/io.h>

#include "ds18b20_lib.h"

#define DS18B20_SCRATCHPAD_LENGTH 9

void DS18B20convertTemperature(DALLAS_IDENTIFIER_t* device, DS18B20_MODES_t mode)
{
	dallas_match_rom(device);
	dallas_write_byte(DS18B20_CONVERT_T_COMMAND);
	if(mode & DS18B20_WAIT)
		while( !dallas_read());
}

DS18B20_STATUS_t DS18B20readTemperature(DALLAS_IDENTIFIER_t* device, DS18B20_TEMPERATURE_t temperature)
{
	uint8_t crc;
	uint8_t messageBuf[DS18B20_SCRATCHPAD_LENGTH];
	
	dallas_match_rom(device); //Select device
	dallas_write_byte(DS18B20_READ_SCRATCHPAD_COMMAND);
	dallas_read_buffer(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	crc = dallasCRC(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	temperature[0] = (int8_t)(((messageBuf[1] & 0x0F) << 4) | (messageBuf[0] >> 4));
    temperature[1] = (int8_t)((messageBuf[1] & 0x80) | (messageBuf[0] & 0x0F));
	
	if (crc != messageBuf[DS18B20_SCRATCHPAD_LENGTH - 1])
		return DS18B20_SCRATCHPAD_CRC_ERROR;
	else
		return DS18B20_SCRATCHPAD_CRC_OK;
}

DS18B20_STATUS_t DS18B20readConfig(DALLAS_IDENTIFIER_t* device, DS18B20_CONFIG_t* config, DS18B20_MODES_t mode)
{
	uint8_t crc;
	uint8_t messageBuf[DS18B20_SCRATCHPAD_LENGTH];
	
	if (mode & DS18B20_CONFIG_FROM_EEPROM) {
		dallas_match_rom(device); //Select device
		dallas_write_byte(DS18B20_RECALL_CONFIG_COMMAND);	
		while( !dallas_read());
	}
	dallas_match_rom(device); //Select device
	dallas_write_byte(DS18B20_READ_SCRATCHPAD_COMMAND);
	dallas_read_buffer(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	crc = dallasCRC(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	config->temperature_max = (int8_t)messageBuf[2];
    config->temperature_min = (int8_t)messageBuf[3];
	config->resolution = messageBuf[4];
	
	if (crc != messageBuf[DS18B20_SCRATCHPAD_LENGTH - 1])
		return DS18B20_SCRATCHPAD_CRC_ERROR;
	else
		return DS18B20_SCRATCHPAD_CRC_OK;
}

DS18B20_STATUS_t DS18B20setConfig(DALLAS_IDENTIFIER_t* device, DS18B20_CONFIG_t* config, DS18B20_MODES_t mode)
{
	uint8_t crc;
	uint8_t messageBuf[DS18B20_SCRATCHPAD_LENGTH];
	
	dallas_match_rom(device); //Select device
	dallas_write_byte(DS18B20_READ_SCRATCHPAD_COMMAND);
	dallas_read_buffer(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	crc = dallasCRC(messageBuf, DS18B20_SCRATCHPAD_LENGTH);
	
	if (crc != messageBuf[DS18B20_SCRATCHPAD_LENGTH - 1])
		return DS18B20_SCRATCHPAD_CRC_ERROR;

	dallas_match_rom(device); //Select device
	dallas_write_byte(DS18B20_WRITE_SCRATCHPAD_COMMAND);
	if(mode & DS18B20_CONFIG_TMAX)
		dallas_write_byte((uint8_t)config->temperature_max);
	else
		dallas_write_byte(messageBuf[2]);
	if(mode & DS18B20_CONFIG_TMIN)
		dallas_write_byte((uint8_t)config->temperature_min);
	else
		dallas_write_byte(messageBuf[3]);
	if(mode & DS18B20_CONFIG_RESOLUTION)
		dallas_write_byte(config->resolution);
	else
		dallas_write_byte(messageBuf[4]);
	
	if (mode & DS18B20_CONFIG_TO_EEPROM) {
		dallas_match_rom(device); //Select device
		dallas_write_byte(DS18B20_COPY_SCRATCHPAD_COMMAND);	
		if(mode & DS18B20_WAIT)
			while( !dallas_read());		
	}
	
	return DS18B20_SCRATCHPAD_CRC_OK;
	
}

