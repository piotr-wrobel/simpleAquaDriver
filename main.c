
//********* UWAGA !! Na fabrycznym procku nalezy zdjac FUSEBIT CKDIV8 
// i ustawic Fast Rising Power - bity SUT[1:0] na 01  ***********


#include <stdio.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart/uart.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <string.h>


#define UART_DEBUG
#define BAUD0 38400

#define OPOZNIENIE 10
#define KROK 1
#define KROK_ENC 5
#define WYP_MIN 0
#define WYP_MAX 255


//definicje PIN�W
#define WY PD6
#define WY_DDR DDRD
#define WY_PORT PORTD
#define OCR OCR0A

#define ENC_DDR DDRD
#define ENC_PORT PORTD
#define ENC_PIN PIND
#define ENC_SWITCH PD4
#define ENC_CLK PD2
#define ENC_DT PD3

#define UART_BUFFER_SIZE 32
#define COMMAND_START '>'
#define COMMAND_RETURN '<'

const char COMMAND_GET_PWM_CURRENT[] PROGMEM=">get pwm curr";
const char COMMAND_GET_PWM_SAVED[] PROGMEM=">get pwm saved";
const char COMMAND_SAVE_PWM[] PROGMEM=">save pwm";
const char COMMAND_RESTORE_PWM[] PROGMEM=">restore pwm";
const char COMMAND_SET_PWM[] PROGMEM=">set pwm:";

char napis[5];
uint8_t switch_mode = 0;
volatile uint8_t wypelnienie = 0, zmiana_wypelnienia = 0;

#ifdef UART_DEBUG
	const char S_NL[] PROGMEM="\r\n";
	const char S_START[] PROGMEM="Start...\r\n";
	const char S_TPRZYC[] PROGMEM="Tryb przycisku\r\n";
	const char S_TPOT[] PROGMEM="Tryb potencjometru/encodera\r\n";
	//const char S_WCISNIETY[] PROGMEM="wcisniety...\r\n";
	const char S_OEEPROM[] PROGMEM="odczyt z eeprom:";
	const char S_ZEEPROM[] PROGMEM="zapis do eeprom:";
	const char S_BZEEPROM[] PROGMEM="wypelnienie bez zmian";	
	const char S_POMIAR[] PROGMEM="pomiar:";
	const char DEBUG_UART_FRAME_ERROR[] PROGMEM="UART_FRAME_ERROR";
	const char DEBUG_UART_OVERRUN_ERROR[] PROGMEM="UART_OVERRUN_ERROR";
	const char DEBUG_UART_BUFFER_OVERFLOW[] PROGMEM="UART_BUFFER_OVERFLOW";
#endif

uint8_t zapisz_wypelnienie(uint8_t wypelnienie);
#ifdef UART_DEBUG
void UARTuitoa(uint16_t liczba, char *string);
#endif


//INT0 interrupt 
ISR(INT0_vect )
{
	if(ENC_PIN & (1<<ENC_DT))
	{
		if(wypelnienie < WYP_MAX - KROK_ENC) 
			wypelnienie += KROK_ENC;
		else
			wypelnienie = WYP_MAX;
		uart0_putc('+');
	}
	else
	{
		if(wypelnienie >= KROK_ENC) 
			wypelnienie -= KROK_ENC;
		else
			wypelnienie = WYP_MIN;
		uart0_putc('-');
	}
	zmiana_wypelnienia = 1;
}

//INT1 interrupt
ISR(INT1_vect )
{
	if(ENC_PIN & (1<<ENC_CLK))
	{
		if(wypelnienie < WYP_MAX - KROK_ENC) 
			wypelnienie += KROK_ENC;
		else
			wypelnienie = WYP_MAX;
		uart0_putc('+');
	}
	else
	{
		if(wypelnienie >= KROK_ENC) 
			wypelnienie -= KROK_ENC;
		else
			wypelnienie = WYP_MIN;
		uart0_putc('-');
	}
	zmiana_wypelnienia = 1;

}

int main(void)
{
  	char uart_buffer[UART_BUFFER_SIZE] = "";
	uint8_t uart_buffer_tmp_pointer = 0;
	uint8_t tmp = 0, znak;
	uint16_t uart_znak;
	// next four instructions. // Niepotrzebne, wylaczony fuse bit CKDIV8
    //CLKPR=(1<<CLKPCE); 
    //CLKPR=0; // 8 MHZ
	
	cli();	
	//########### I/O ###########
	//Ustawienie pinów
    OCR0A = WYP_MIN; // Na poczatek wypelnienie min, lampka przyciemniona
    
    WY_DDR  |= (1<<WY); // jako wyjscia
    WY_PORT |=  (1<<WY); //Stan wysoki, wygaszenie lampki

    ////############# PWM Fast #############
	TCCR0A |= (1<<COM0A0) | (1<<COM0A1) | (1<<WGM00) | (1<<WGM01); //Fast PWM, set OC0A on compare match, clear at BOTTOM
	TCCR0B |= ((1<<CS01) | (1<<CS00)); // Internal clock, prescaler 64 f= CPU clock / 256 / 64  (8MHz/256/64=488Hz)
	/* set ENC_CLK and ENC_DT as input */	
	ENC_DDR &=~ (1 << ENC_CLK);					/* PD2 and PD3 as input */
	ENC_DDR &=~ (1 << ENC_DT);        
	ENC_PORT |= (1 << ENC_DT)|(1 << ENC_CLK);   /* PD2 and PD3 pull-up enabled   */

	EIMSK |= (1<<INT0)|(1<<INT1);				/* enable INT0 and INT1 */
	EICRA |= (1<<ISC01)|(1<<ISC11)|(1<<ISC10); 	/* INT0 - falling edge, INT1 - reising edge */

	ENC_DDR &=~ (1 << ENC_SWITCH);				/* PD4 as input */
	ENC_PORT |= (1 << ENC_SWITCH);   			/* PD4 pull-up enabled   */

	/* enable interrupt's */
	sei();		

#ifdef UART_DEBUG
	uart0_init(UART_BAUD_SELECT(BAUD0,F_CPU));
	uart0_puts_p(S_NL);
	uart0_puts_p(S_START);
	uart0_puts_p(S_TPOT);
#endif
	eeprom_busy_wait();
	__EEGET(wypelnienie,0); //Wczytujemy z pamieci zapisane wypelnienie
#ifdef UART_DEBUG
	UARTuitoa((uint16_t)wypelnienie, napis);
	uart0_puts_p(S_OEEPROM);
	uart0_puts(napis);
	uart0_puts_p(S_NL);
#endif
	for(tmp=WYP_MIN;tmp<wypelnienie;tmp+=KROK) // Plynnie rozjasniamy do osiagniecia zapisanego w eeprom
	{
		OCR=tmp;
		_delay_ms(OPOZNIENIE);
	}
	OCR0A=wypelnienie;

	
	while(1) //Petla glówna
	{
			if(!(ENC_PIN & (1<<ENC_SWITCH)))
			{
				(void)zapisz_wypelnienie(wypelnienie);
			}
			if(zmiana_wypelnienia)
			{
				OCR0A=wypelnienie;
			#ifdef UART_DEBUG
				UARTuitoa(wypelnienie, napis);
				uart0_puts(napis);
				uart0_puts_p(S_NL);
			#endif
				zmiana_wypelnienia = 0;
			}
			uart_znak = uart0_getc();
			if(uart_znak & (UART_NO_DATA | UART_BUFFER_OVERFLOW | UART_OVERRUN_ERROR | UART_FRAME_ERROR))
			{
				switch (uart_znak)
				{
					case UART_NO_DATA:
					break;
					case UART_BUFFER_OVERFLOW:
						uart0_puts_p(DEBUG_UART_BUFFER_OVERFLOW);
						uart0_puts_p(S_NL);
					break;
					case UART_OVERRUN_ERROR:
						uart0_puts_p(DEBUG_UART_OVERRUN_ERROR);
						uart0_puts_p(S_NL);					
					break;
					case UART_FRAME_ERROR:
						uart0_puts_p(DEBUG_UART_FRAME_ERROR);
						uart0_puts_p(S_NL);						
					break;										
				}
			} else{
				znak = (uint8_t)(uart_znak & 0x00FF);
				if(!uart_buffer_tmp_pointer && znak == COMMAND_START){ 				//Jeśli 1 znak to '>' zaczynamy zapisywać komendę
					uart_buffer[uart_buffer_tmp_pointer] = znak;
					uart_buffer[++uart_buffer_tmp_pointer] = 0;
				}else if(uart_buffer_tmp_pointer && znak != '\r' && znak != '\n'){	//jeśli następne znaki nie są końcem linii, zapisujemy je do bufora
					uart_buffer[uart_buffer_tmp_pointer] = znak;
					uart_buffer[++uart_buffer_tmp_pointer] = 0;
				} else if(uart_buffer_tmp_pointer){ 								//Mamy już coś w buforze i wystąpił koniec linii, zatem mamy gotową komendę
					uart_buffer_tmp_pointer = 0;
				}
			}
			if(uart_buffer[0] && !uart_buffer_tmp_pointer){							//Tu korzystamy z komendy
				if(strcmp_P(uart_buffer, COMMAND_GET_PWM_CURRENT) == 0){
					UARTuitoa(wypelnienie,napis);
					uart_putc(COMMAND_RETURN);
					uart0_puts(napis);
					uart0_puts_p(S_NL);
				}
				if(strcmp_P(uart_buffer, COMMAND_GET_PWM_SAVED) == 0){
					eeprom_busy_wait();
					__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
					UARTuitoa(tmp,napis);
					uart_putc(COMMAND_RETURN);
					uart0_puts(napis);
					uart0_puts_p(S_NL);
				}
				if(strcmp_P(uart_buffer, COMMAND_SAVE_PWM) == 0){
					UARTuitoa(zapisz_wypelnienie(wypelnienie),napis);
					uart_putc(COMMAND_RETURN);
					uart0_puts(napis);
					uart0_puts_p(S_NL);					
				}
				if(strcmp_P(uart_buffer, COMMAND_RESTORE_PWM) == 0){
					eeprom_busy_wait();
					__EEGET(wypelnienie,0); // Wczytujemy poprzednie ustawienie z EEPROM					
					OCR0A = wypelnienie;
					UARTuitoa(0,napis);
					uart_putc(COMMAND_RETURN);
					uart0_puts(napis);
					uart0_puts_p(S_NL);					
				}
				
				uint8_t dlugosc_1 = strlen_P(COMMAND_SET_PWM);
				uint8_t dlugosc_2 = strlen(uart_buffer);
				int8_t dopasowanie = strncmp_P(uart_buffer, COMMAND_SET_PWM, dlugosc_1);
				if(!dopasowanie && dlugosc_2 > dlugosc_1)
				{
					strcpy(napis, uart_buffer + dlugosc_1);
					if(strlen(napis) < 4)
					{
						int16_t new_pwm = atoi(napis);
						if(new_pwm > 0 && new_pwm <256)
						 {
							wypelnienie = (uint8_t)new_pwm;
							OCR0A = wypelnienie;
							UARTuitoa(0,napis);	
						 } else{
							UARTuitoa(1,napis);	
						 }
					} else {
						UARTuitoa(1,napis);		
					}
					uart_putc(COMMAND_RETURN);
					uart0_puts(napis);
					uart0_puts_p(S_NL);
				}
				uart_buffer[0] = 0;
			}
	}
}

uint8_t zapisz_wypelnienie(uint8_t wypelnienie)
{
	uint8_t tmp;

	eeprom_busy_wait();
	__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
	if(tmp != wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
	{
		eeprom_busy_wait();
		__EEPUT(0, wypelnienie);
	#ifdef UART_DEBUG
		UARTuitoa((uint16_t)wypelnienie, napis);
		uart0_puts_p(S_ZEEPROM);
		uart0_puts(napis);
		uart0_puts_p(S_NL);
	#endif
		return 0;										
	} else {
	#ifdef UART_DEBUG
		uart0_puts_p(S_BZEEPROM);
		uart0_puts_p(S_NL);
	#endif
		return 1;				
	}
}

#ifdef UART_DEBUG
void UARTuitoa(uint16_t liczba, char *string)
{
	uint8_t nibble=0,pozycja;
	for(pozycja=0;pozycja<4;pozycja++)
	{
		nibble=(liczba>>12);
		liczba=(liczba<<4);
		if(nibble <= 9)
			nibble+=48;
		else
			nibble+=55;
		string[pozycja]=nibble;
	}
	string[pozycja]=0;
}
#endif
