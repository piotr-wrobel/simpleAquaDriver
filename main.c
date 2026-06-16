
//********* UWAGA !! Na fabrycznym procku nalezy zdjac FUSEBIT CKDIV8 
// i ustawic Fast Rising Power - bity SUT[1:0] na 01  ***********


#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>
#include "uart/uart.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>


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

char napis[5];
uint8_t switch_mode = 0;
volatile uint8_t wypelnienie = 0, zmiana_wypelnienia = 0;

#ifdef UART_DEBUG
static void UARTuitoa(uint16_t liczba, char *string);
#endif

#ifdef UART_DEBUG
	const char S_NL[] PROGMEM="\r\n";
	const char S_START[] PROGMEM="Start...\r\n";
	const char S_TPRZYC[] PROGMEM="Tryb przycisku\r\n";
	const char S_TPOT[] PROGMEM="Tryb potencjometru/encodera\r\n";
	//const char S_WCISNIETY[] PROGMEM="wcisniety...\r\n";
	const char S_OEEPROM[] PROGMEM="odczyt z eeprom:";
	const char S_ZEEPROM[] PROGMEM="zapis do eeprom:";
	const char S_POMIAR[] PROGMEM="pomiar:";
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
  	uint8_t tmp=0;
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
				eeprom_busy_wait();
				__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
				if(tmp!=wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
				{
					eeprom_busy_wait();
					__EEPUT(0,wypelnienie);
				#ifdef UART_DEBUG
					UARTuitoa((uint16_t)wypelnienie, napis);
					uart0_puts_p(S_ZEEPROM);
					uart0_puts(napis);
					uart0_puts_p(S_NL);
				#endif										
				}
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
	}
}

#ifdef UART_DEBUG
static void UARTuitoa(uint16_t liczba, char *string)
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
