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

#include "fspac_lib.h"

uint8_t fspacParse(char *cmd_ptr[], char *buff, const char separator, cmd_ptr_t parts, const uint8_t buff_size)
{
    cmd_ptr_t cmd_enum = cmd_prefix;

    
    if (buff[0] == 0)
      return 0;
    
    cmd_ptr[cmd_prefix] = buff;    
    
    for (uint8_t i = 0; i < buff_size - 1; i++) 
    {      
        if (buff[i] == separator)
        {
            if (cmd_enum < parts - 1)
                cmd_enum++;
            else
                break;
            cmd_ptr[cmd_enum] = &buff[i+1];
            buff[i] = 0;
            
        } else if (buff[i] == 0)
           break; 
    }
    return cmd_enum + 1;
}

conversion_returns_t fspacHexCharToUI8(char *str, uint8_t *byte)
{
	if (str[0] > ASCII_ZERO - 1 && str[0] < ASCII_ZERO + 10) {
        *byte = str[0] - ASCII_ZERO;
    } else if (str[0] > 64 && str[0] < 71) {
        *byte = str[0] - 55;
    } else 
        return conversion_bad_char2;
    *byte = (*byte << 4);
    
	if (str[1]  > ASCII_ZERO - 1 && str[1] < ASCII_ZERO + 10) {
        *byte += (uint8_t)(str[1] - ASCII_ZERO);
    } else if (str[1] > 64 && str[1] < 71) {
        *byte += (uint8_t)(str[1] - 55);
    } else 
        return conversion_bad_char1;
    return conversion_ok;
}

conversion_returns_t fspacHexStrToUI8Tab(char *str, uint8_t *tab, const uint8_t length)
{

    uint8_t byte = 0;
    conversion_returns_t result;
    
    if (length % 2)
        return conversion_not_parity_length;
    for (uint8_t i = 0; str[i] != 0 && i < length; i++)
    {
        if (!(i % 2))
        {
          if (conversion_ok == (result = fspacHexCharToUI8(&str[i], &byte))) 
          {
              *tab++ = byte;
          } else
            return result;
        }        
    }
    return conversion_ok;
}

conversion_returns_t fspacMemToStr(uint8_t* byte, uint8_t lenght, char* string)
{
	uint8_t nibbleL = 0, nibbleH = 0;
	uint8_t possition;
	for (possition = 0; possition < lenght * 2; possition += 2)
	{
		nibbleL = (*byte & 0x0F);
		nibbleH = (*byte >> 4);
		if(nibbleH <= 9)
			nibbleH += ASCII_ZERO;
		else
			nibbleH += ASCII_A;
		
        string[possition] = nibbleH;
		
        if(nibbleL <= 9)
			nibbleL += ASCII_ZERO;
		else
			nibbleL += ASCII_A;
		
        string[possition+1] = nibbleL;
		
        byte++;
	}
	string[possition] = 0;
    return conversion_ok;
}
