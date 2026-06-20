/*
 * An AVR library for communication on a Dallas 1-Wire bus.
 *
 * Features (?)
 * ------------
 *
 * * It uses a single GPIO pin (no UARTs).
 * * It does not use dynamic memory allocation. The only drawback is that you
 *   have to know the number of devices on the bus in advance.
 * * It is polled, not interrupt-driven. There are several sections of code
 *   that must run for a specific amount of time and have to disable
 *   interrupts globally.
 * * Only the MATCH_ROM, SEARCH_ROM, and SKIP_ROM commands have been
 *   implemented. At this point other commands would be trivial to add.
 *
 * Directions
 * ----------
 *
 * 1. Modify F_CPU, DALLAS_PORT, DALLAS_DDR, DALLAS_PORT_IN, DALLAS_PIN, and
 *    DALLAS_NUM_DEVICES to match your application.
 * 2. In your code, first run dallas_search_identifiers() to populate the
 *    the list of identifiers with the devices on your bus.
 * 3. ???
 * 4. Profit!
 *
 * Cautions/Caveats
 * ----------------
 *
 * The 1-Wire bus is *very* timing-dependent. If you are having issues and it's
 * not an electrical/connectivity one it is most likely a timing issue. The
 * worst function in this regard is dallas_read(). The delays chosen there are a
 * compromise between having to wait for the bus to return to 5V after being
 * pulled to ground (determined by the RC time constant) while also needing to
 * read the bus before the 15 usec time slot expires.
 *
 * Verify the timing with a logic analyzer or oscilloscope. Also check the
 * datasheet for your specific device to make sure that it is ok with the timing
 * values chosen in the code and modify, if necessary.
 *
 * I am not a 1-Wire expert by any means so this code is provided as-is.
 *
 * Enjoy!
 * ------
 *
 * April 30, 2010
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

#include <avr/io.h>

#include <stdint.h>

#include "dallas_one_wire.h"

#include <util/atomic.h>
#include <util/delay.h>

#include "dallas_one_wire.h"

//////////////////////
// Global variables //
//////////////////////

//DALLAS_IDENTIFIER_LIST_t identifier_list;

/////////////////////////////////
// Private function prototypes //
/////////////////////////////////

static DALLAS_ONEWIRE_STATUS_t dallas_discover_identifier(DALLAS_IDENTIFIER_t *, DALLAS_IDENTIFIER_t *, uint8_t, uint8_t *, uint8_t);

///////////////
// Functions //
///////////////

void dallas_write(uint8_t bit) {
	if (bit == 0x00) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			// Configure the pin as an output.
			DALLAS_DDR |= (1 << DALLAS_PIN);

			// Pull the bus low.
			DALLAS_PORT &= (uint8_t)~(1 << DALLAS_PIN);


			// Wait the required time.
			_delay_us(90);

			// Release the bus.
			DALLAS_PORT |= (1 << DALLAS_PIN);

			// Let the rest of the time slot expire.
			_delay_us(30);
		}
	}
	else {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			// Configure the pin as an output.
			DALLAS_DDR |= (1 << DALLAS_PIN);

			// Pull the bus low.
			DALLAS_PORT &= (uint8_t)~(1 << DALLAS_PIN);

			// Wait the required time.
			_delay_us(10);

			// Release the bus.
			DALLAS_PORT |= (1 << DALLAS_PIN);

			// Let the rest of the time slot expire.
			_delay_us(50);
		}
	}
}

uint8_t dallas_read(void) {
	uint8_t reply;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Configure the pin as an output.
		DALLAS_DDR |= (1 << DALLAS_PIN);

		// Pull the bus low.
		DALLAS_PORT &= (uint8_t)~(1 << DALLAS_PIN);

		// Wait the required time.
		_delay_us(2);

		// Configure as input.
		DALLAS_DDR &= (uint8_t)~(1 << DALLAS_PIN);

		// Wait for a bit.
		_delay_us(11);

		if ((DALLAS_PORT_IN & (1 << DALLAS_PIN)) == 0x00) {
			reply = 0x00;
		}
		else {
			reply = 0x01;
		}

		// Let the rest of the time slot expire.
		_delay_us(47);
	}

	return reply;
}

// Resets the bus and returns 0x01 if a slave indicates present, 0x00 otherwise.
DALLAS_ONEWIRE_STATUS_t dallas_reset(void) {
	DALLAS_ONEWIRE_STATUS_t status;

	// Reset the slave_reply variable.
	status = DALLAS_IDENTIFIER_NO_ERROR;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		// Configure the pin as an output.
		DALLAS_DDR |= (1 << DALLAS_PIN);

		// Pull the bus low.
		DALLAS_PORT &= (uint8_t)~(1 << DALLAS_PIN);

		// Wait the required time.
		_delay_us(500); // 500 uS

		// Switch to an input, enable the pin change interrupt, and wait.
		DALLAS_DDR &= (uint8_t)~(1 << DALLAS_PIN);

		_delay_us(70);

		if ((DALLAS_PORT_IN & (1 << DALLAS_PIN)) == 0x00) {
			status = DALLAS_IDENTIFIER_NO_ERROR;
		} 
		else {

			status = DALLAS_IDENTIFIER_NO_DEVICES;
		}

		_delay_us(420);
	}

	return status;
}

void dallas_write_byte(uint8_t byte) {
	uint8_t position;

	for (position = 0x00; position < 0x08; position++) {
		dallas_write(byte & 0x01);

		byte = (byte >> 1);
	}
}

uint8_t dallas_read_byte(void) {
	uint8_t byte;
	uint8_t position;

	byte = 0x00;

	for (position = 0x00; position < 0x08; position++) {
		byte += (uint8_t)(dallas_read() << position);
	}

	return byte;
}
/*
// Uses the uC to power the bus.
void dallas_drive_bus(void) {
	// Configure the pin as an output.
	DALLAS_DDR |= (1 << DALLAS_PIN);

	// Set the bus high.
	DALLAS_PORT |= (1 << DALLAS_PIN);
}
*/
void dallas_match_rom(DALLAS_IDENTIFIER_t * identifier) {
	uint8_t identifier_bit;
	uint8_t current_byte;
	uint8_t current_bit;

	dallas_reset();
	dallas_write_byte(DALLAS_MATCH_ROM_COMMAND);

	for (identifier_bit = 0x00; identifier_bit < DALLAS_NUM_IDENTIFIER_BITS; identifier_bit++) {
		current_byte = identifier_bit / 8;
		current_bit = (uint8_t)(identifier_bit - (current_byte * 8));

		dallas_write(identifier->identifier[current_byte] & (1 << current_bit));
	}
}

void dallas_skip_rom(void) {
	dallas_reset();
	dallas_write_byte(DALLAS_SKIP_ROM_COMMAND);
}

uint8_t dallasCRC(uint8_t* data, uint8_t length) {
	uint8_t id_bit_number, current_bit, current_byte, input_bit;
	uint8_t crc = 0, crc_8 = 0;
	for (id_bit_number = 0; id_bit_number < (length - 1) * 8; id_bit_number++) {
		current_byte = id_bit_number / 8;
		current_bit = (uint8_t)(id_bit_number - (current_byte * 8));
		
		input_bit = (*data >> current_bit) & 0x01;
		crc_8 = (uint8_t)(((crc & 0x01) ^ input_bit) << 7);
		crc = (crc >> 1);
		
		if(crc_8)
			crc = (crc ^ 0b00001100) + 0b10000000;
		
		if(current_bit == 7) {
			data++;
		}
	}
	return crc;
}

DALLAS_ONEWIRE_STATUS_t dallasIdentifierCRC(DALLAS_IDENTIFIER_t * identifier) {
	
	uint8_t crc;
	
	crc = dallasCRC(identifier->identifier, DALLAS_NUM_IDENTIFIER_BITS / 8);
	identifier->crc = crc;
	if(crc == identifier->identifier[DALLAS_IDENTIFIER_CRC_BYTE])
		return DALLAS_IDENTIFIER_CRC_OK;
	else {
		return DALLAS_IDENTIFIER_CRC_ERROR;
	}
}

DALLAS_ONEWIRE_STATUS_t dallasSearch(DALLAS_IDENTIFIER_LIST_t *identifier_list, uint8_t command) {
        
	uint8_t current_device;
	DALLAS_ONEWIRE_STATUS_t return_status, crc_check_status;
	uint8_t LastDeviceFlag = 0, LastDiscrepancy = 0, last_zero = 0;
	
	identifier_list->num_devices = 0;
	
	for (current_device = 0x00; current_device < DALLAS_NUM_DEVICES; current_device++) {
		last_zero = 0;
		if (current_device == 0x00) {
			return_status = dallas_discover_identifier(&(identifier_list->identifiers[current_device]), &(identifier_list->identifiers[current_device]), LastDiscrepancy, &last_zero, command);
		}
		else {
			return_status = dallas_discover_identifier(&(identifier_list->identifiers[current_device]), &(identifier_list->identifiers[current_device-1]), LastDiscrepancy, &last_zero, command);
		}

		if (return_status == DALLAS_IDENTIFIER_NO_DEVICES)
			return DALLAS_IDENTIFIER_NO_DEVICES; //no devices were found

		LastDiscrepancy = last_zero;

		if (LastDiscrepancy == 0)
			LastDeviceFlag = 1;
		
		if (return_status != DALLAS_IDENTIFIER_SEARCH_ERROR) {
			identifier_list->num_devices = current_device + 0x01;
			crc_check_status = dallasIdentifierCRC(
			&(identifier_list->identifiers[current_device]));                        
			if(crc_check_status != DALLAS_IDENTIFIER_CRC_OK) {
				return crc_check_status;
			}		
		}
		if (LastDeviceFlag)
			return DALLAS_IDENTIFIER_DONE; //all devices were found
	}        
	return return_status;
}

static DALLAS_ONEWIRE_STATUS_t dallas_discover_identifier(DALLAS_IDENTIFIER_t * current_identifier, DALLAS_IDENTIFIER_t * last_identifier, uint8_t LastDiscrepancy, uint8_t * last_zero, uint8_t command) {
	DALLAS_ONEWIRE_STATUS_t return_status;
	uint8_t id_bit_number;
	uint8_t received_two_bits;
	uint8_t current_bit;
	uint8_t current_byte;
	uint8_t search_direction;

	return_status = dallas_reset();
	if( return_status != DALLAS_IDENTIFIER_NO_ERROR )
		return return_status;

	dallas_write_byte(command);

	for (id_bit_number = 0; id_bit_number < DALLAS_NUM_IDENTIFIER_BITS; id_bit_number++) {
		received_two_bits = (dallas_read() << 1);
		received_two_bits += dallas_read();

		current_byte = id_bit_number / 8;
		current_bit = (uint8_t)(id_bit_number - (current_byte * 8));
		
		if(!current_bit) //Only one per byte
			current_identifier->identifier[current_byte] = 0;
		
		if (received_two_bits == 0x02) {
			// All devices have a 1 at this position.
			current_identifier->identifier[current_byte] += (uint8_t)(1 << current_bit);
			dallas_write(0x01);
		}
		else if (received_two_bits == 0x01) {
			// All devices have a 0 at this position.
			dallas_write(0x00);
		}
		else if (received_two_bits == 0x00) {
			if ((id_bit_number + 1) == LastDiscrepancy)
				search_direction = 0x01;
			else if ((id_bit_number + 1) > LastDiscrepancy)
				search_direction = 0x00;
			else {
				if (last_identifier->identifier[current_byte] & (1 << current_bit))
					search_direction = 0x01;
				else
					search_direction = 0x00;
			}

			if (search_direction == 0x00) {
				*last_zero = (id_bit_number + 1);
				// TODO: add LastFamilyDiscrepancy support
				
			}

			current_identifier->identifier[current_byte] += (uint8_t)(search_direction << current_bit);
			dallas_write(search_direction);
		}
		else {
			// Error!
			return DALLAS_IDENTIFIER_SEARCH_ERROR;
		}
	}

	return DALLAS_IDENTIFIER_NO_ERROR;

}

DALLAS_ONEWIRE_STATUS_t dallas_check_device_presence(DALLAS_IDENTIFIER_t * identifier, uint8_t* device_family) {
	
	DALLAS_IDENTIFIER_t  tmp_identifier;
	uint8_t byte, return_code, last_zero = 0;
	
	return_code = dallas_discover_identifier(&tmp_identifier, identifier, 64, &last_zero, DALLAS_SEARCH_ROM_COMMAND);
	
	if (return_code == DALLAS_IDENTIFIER_SEARCH_ERROR)
		return DALLAS_NO_DEVICE;

	for (byte = 0; byte < DALLAS_NUM_IDENTIFIER_BITS / 8; byte++) {
		if (tmp_identifier.identifier[byte] != identifier->identifier[byte])
			return DALLAS_NO_DEVICE;
	}
	
	*device_family = tmp_identifier.identifier[0];
	return DALLAS_DEVICE_IS_PRESENT;
}


void dallas_write_buffer(uint8_t * buffer, uint8_t buffer_length) {
	uint8_t i;

	for (i = 0x00; i < buffer_length; i++) {
		dallas_write_byte(buffer[i]);
	}
}


void dallas_read_buffer(uint8_t * buffer, uint8_t buffer_length) {
	uint8_t i;

	for (i = 0x00; i < buffer_length; i++) {
		buffer[i] = dallas_read_byte();
	}
}
