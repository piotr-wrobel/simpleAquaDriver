/*
 *  AVR Fast String Parse and Conversion library
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

#ifndef FSPAC_LIB_H
#define FSPAC_LIB_H

//#define RS_BUFF_LEN 50

#define ASCII_ZERO 0x30
#define ASCII_A    0x37


typedef enum {
    conversion_ok,
    conversion_bad_char1,
    conversion_bad_char2,
    conversion_not_parity_length,
    conversion_out_of_range
} conversion_returns_t;

typedef enum {
  cmd_prefix = 0,
  time_year = 0,
  tconfig_dest = 0,

  cmd_address = 1,
  time_month = 1,
  tconfig_tmax = 1,

  cmd_data = 2,
  time_day = 2,
  tconfig_tmin = 2,

  cmd_postfix = 3,
  time_hour = 3,
  tconfig_resol = 3,
  
  cmd_parts = 4,
  time_minute = 4,
  tconfig_parts = 4,
  
  time_seconds = 5,
  
  time_fake = 6,
  
  time_parts
  
} cmd_ptr_t;

uint8_t fspacParse(char *cmd_ptr[], char *buff, const char separator, cmd_ptr_t parts, const uint8_t buff_size);
conversion_returns_t fspacHexCharToUI8(char *str, uint8_t *byte);
conversion_returns_t fspacHexStrToUI8Tab(char *str, uint8_t *tab, const uint8_t length);
conversion_returns_t fspacMemToStr(uint8_t* byte, uint8_t lenght, char* string);

#endif /* FSPAC_LIB_H */
