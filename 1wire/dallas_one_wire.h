/*
 * An AVR library for communication on a Dallas 1-Wire bus.
 *
 * The main documentation is in dallas_one_wire.c.
 */

/******************************************************************************
 * Copyright � 2010, Mike Roddewig (mike@dietfig.org).
 * Copyright � 2022, pvglab (pvg@poczta.fm).
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

#ifndef DALLAS_ONE_WIRE_H
#define DALLAS_ONE_WIRE_H

/////////////
// Defines //
/////////////

// AVR clock frequency
//#define F_CPU 8000000UL // 8 MHz

// Pin defines

#define DALLAS_PORT 	PORTB
#define DALLAS_PORT_IN 	PINB
#define DALLAS_DDR 	DDRB
#define DALLAS_PIN 	PB3

// The number of devices on the bus.
#define DALLAS_NUM_DEVICES 6


// The number of bits in an identifier.
#define DALLAS_NUM_IDENTIFIER_BITS 64
#define DALLAS_IDENTIFIER_CRC_BYTE ((DALLAS_NUM_IDENTIFIER_BITS / 8 ) - 1 )

// One Wire Commands

#define DALLAS_MATCH_ROM_COMMAND 	0x55
#define DALLAS_SKIP_ROM_COMMAND		0xCC
#define DALLAS_SEARCH_ROM_COMMAND 	0xF0


// One Wire Families
#define ONEWIRE_DS18B20_FAMILY 0x28
#define ONEWIRE_DS2415_FAMILY  0x24
#define ONEWIRE_GAA2415_FAMILY 0xA4
#define ONEWIRE_DS2417_FAMILY  0x27

/////////////////////////////////////
// Identifier routine return codes //
/////////////////////////////////////

typedef enum {
  DALLAS_IDENTIFIER_NO_ERROR		= 0x00,
  DALLAS_IDENTIFIER_DONE			= 0x01,
  DALLAS_IDENTIFIER_SEARCH_ERROR	= 0x02,
  DALLAS_IDENTIFIER_NO_DEVICES		= 0x03,
  DALLAS_IDENTIFIER_CRC_OK			= 0x00,
  DALLAS_IDENTIFIER_CRC_ERROR		= 0x04,
  DALLAS_DEVICE_IS_PRESENT			= 0x00,
  DALLAS_NO_DEVICE					= 0x01
} DALLAS_ONEWIRE_STATUS_t;

////////////////
// Structures //
////////////////

typedef struct {
	uint8_t identifier[DALLAS_NUM_IDENTIFIER_BITS / 8];
	uint8_t crc;
} DALLAS_IDENTIFIER_t;

typedef struct {
	DALLAS_IDENTIFIER_t identifiers[DALLAS_NUM_DEVICES];
	uint8_t num_devices;
} DALLAS_IDENTIFIER_LIST_t;

///////////////
// Functions //
///////////////


// Writes the LSB of the argument to the bus.
void dallas_write(uint8_t);

// Write a byte to the bus.
void dallas_write_byte(uint8_t);

// Write the specified number of bytes to the bus from the supplied buffer.
void dallas_write_buffer(uint8_t * buffer, uint8_t buffer_length);

// Read a bit from the bus and returns it as the LSB.
uint8_t dallas_read(void);

// Reads a byte from the bus.
uint8_t dallas_read_byte(void);

// Reads the specified number of bytes from the bus into the supplied buffer.
void dallas_read_buffer(uint8_t * buffer, uint8_t buffer_length);

// Resets the bus. Returns...

DALLAS_ONEWIRE_STATUS_t dallas_reset(void);

// Powers the bus from the AVR (max 40 mA).
void dallas_drive_bus(void);

// Sends a MATCH ROM command to the specified device. Automatically resets the
// bus.
void dallas_match_rom(DALLAS_IDENTIFIER_t *);

// Sends a SKIP ROM command. Automatically resets the bus.
void dallas_skip_rom(void);

// Calculate generic CRC 
uint8_t dallasCRC(uint8_t* data, uint8_t length);

// Calculate CRC for idientifer
DALLAS_ONEWIRE_STATUS_t dallasIdentifierCRC(DALLAS_IDENTIFIER_t *);

// Populates the identifier list. Returns...
// 0 - if devices were found and there was no error
// 1 - if there was a bus error
// 2 - if there were more devices than specified by DALLAS_NUM_DEVICES
DALLAS_ONEWIRE_STATUS_t dallasSearch(DALLAS_IDENTIFIER_LIST_t*  identifier_list, uint8_t command);
#define dallas_search_identifiers(s1) (dallasSearch(s1, DALLAS_SEARCH_ROM_COMMAND))

// Check device presence, returns:
// DALLAS_DEVICE_IS_PRESENT, DALLAS_NO_DEVICE
DALLAS_ONEWIRE_STATUS_t dallas_check_device_presence(DALLAS_IDENTIFIER_t *, uint8_t*);

// Returns the list of identifiers.
// DALLAS_IDENTIFIER_LIST_t * get_identifier_list(void);

#endif
