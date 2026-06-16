
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
#define WCISNIETY 0
#define ZWOLNIONY 1
#define KROK 1
#define KROK_ENC 5
#define WYP_MIN 0
#define WYP_MAX 255


//definicje PIN�W
#define WY PD6
#define WY_DDR DDRD
#define WY_PORT PORTD
#define OCR OCR0A

#define SWITCH PC5
#define SWITCH_DDR DDRC
#define SWITCH_PORT PORTC
#define SWITCH_PIN PINC

#define ENC_DDR DDRD
#define ENC_PORT PORTD
#define ENC_PIN PIND
#define ENC_SWITCH PD4
#define ENC_CLK PD2
#define ENC_DT PD3

char napis[5];
uint8_t switch_mode = 0;
volatile uint8_t wypelnienie = 0;



uint8_t czekaj(uint8_t naco);
#ifdef UART_DEBUG
static void UARTuitoa(uint16_t liczba, char *string);
#endif
//uint16_t ADC_run(void);

#ifdef UART_DEBUG
	const char S_NL[] PROGMEM="\r\n";
	const char S_START[] PROGMEM="Start...\r\n";
	const char S_TPRZYC[] PROGMEM="Tryb przycisku\r\n";
	const char S_TPOT[] PROGMEM="Tryb potencjometru/encodera\r\n";
	const char S_WCISNIETY[] PROGMEM="wcisniety...\r\n";
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
	UARTuitoa(wypelnienie, napis);
	uart0_puts(napis);
	uart0_puts_p(S_NL);
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
	UARTuitoa(wypelnienie, napis);
	uart0_puts(napis);
	uart0_puts_p(S_NL);
}

int main(void)
{
  
	//uint16_t adc_result = 0;
	uint8_t tmp=0; //, adc_result8=0,adc_result8_prev = 0;
	// next four instructions. // Niepotrzebne, wylaczony fuse bit CKDIV8
    //CLKPR=(1<<CLKPCE); 
    //CLKPR=0; // 8 MHZ
	
	
	//########### I/O ###########
	//Ustawienie pinów
    OCR0A = WYP_MIN; // Na poczatek wypelnienie min, lampka przyciemniona
    
    WY_DDR  |= (1<<WY); // jako wyjscia
    WY_PORT |=  (1<<WY); //Stan wysoki, wygaszenie lampki
	SWITCH_DDR  &=~ (1<<SWITCH); //Ustawienie pinów klawiszy jako wejscie 
    SWITCH_PORT |=  (1<<SWITCH); //wlaczenie rezystora podciagajacego tzw. Pull_up

    ////############# PWM Fast #############
	TCCR0A |= (1<<COM0A0) | (1<<COM0A1) | (1<<WGM00) | (1<<WGM01); //Fast PWM, set OC0A on compare match, clear at BOTTOM
	TCCR0B |= ((1<<CS01) | (1<<CS00)); // Internal clock, prescaler 64 f= CPU clock / 256 / 64  (8MHz/256/64=488Hz)
	

#ifdef UART_DEBUG
	uart0_init(UART_BAUD_SELECT(BAUD0,F_CPU));
	uart0_puts_p(S_NL);
	uart0_puts_p(S_START);
#endif 

	
	if(!(SWITCH_PIN & (1<<SWITCH)))
	{
		switch_mode = 1;
	#ifdef UART_DEBUG
		uart0_puts_p(S_TPRZYC);
	#endif
	} else
	{
	#ifdef UART_DEBUG
		uart0_puts_p(S_TPOT);
		/* set ENC_CLK and ENC_DT as input */	
		cli();
		ENC_DDR &=~ (1 << ENC_CLK);					/* PD2 and PD3 as input */
		ENC_DDR &=~ (1 << ENC_DT);        
		ENC_PORT |= (1 << ENC_DT)|(1 << ENC_CLK);   /* PD2 and PD3 pull-up enabled   */

		EIMSK |= (1<<INT0)|(1<<INT1);				/* enable INT0 and INT1 */
		EICRA |= (1<<ISC01)|(1<<ISC11)|(1<<ISC10); 	/* INT0 - falling edge, INT1 - reising edge */

		ENC_DDR &=~ (1 << ENC_SWITCH);				/* PD4 as input */
		ENC_PORT |= (1 << ENC_SWITCH);   			/* PD4 pull-up enabled   */


		/* enable interrupt's */
		sei();	
	#endif
	}

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
		//_delay_ms(500);
		//WY_PORT ^= (1 << WY);
		if(switch_mode)
		{
			
			if(!(SWITCH_PIN & (1<<SWITCH)))//jesli klawisz jest wduszony
			{
			#ifdef UART_DEBUG
				uart0_puts_p(S_WCISNIETY);
			#endif 

				_delay_ms(50); // Czekamy chwilke
				if(!(SWITCH_PIN & (1<<SWITCH))) // Nadal wduszony
				{
					
					if(czekaj(ZWOLNIONY)) // Czekamy na zwolnienie, czy bedzie sciemnianie ?
					{
						_delay_ms(400); // Opoznienie przed sciemnianiem
						while(czekaj(WCISNIETY)) // Sciemniamy o KROK w petli, z opoznieniem
						{
							if(wypelnienie>=WYP_MIN+KROK)
								wypelnienie-=KROK;
							else //Juz jest maxx sciemniona, zamigamy szybko !:)
							{
								OCR=wypelnienie+20;
								_delay_ms(100);
							}
								
							OCR=wypelnienie;
						}
						eeprom_busy_wait();
						__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
						if(tmp!=wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
						{
							OCR=0; //Migniemy na potwierdzenie zapisu :)
							eeprom_busy_wait();
							__EEPUT(0,wypelnienie);
							_delay_ms(300);
						#ifdef UART_DEBUG
							UARTuitoa((uint16_t)wypelnienie, napis);
							uart0_puts_p(S_ZEEPROM);
							uart0_puts(napis);
							uart0_puts_p(S_NL);
						#endif
						}
						OCR=wypelnienie;	
					}else //Nie bylo zwolnienia klawisza, czyli rozjasniamy
					{
						while(czekaj(WCISNIETY)) // Rozjasniamy o KROK w petli, z opoznieniem
						{
							if(wypelnienie<=(255-KROK))
								wypelnienie+=KROK;
							else //Juz jest maxx rozjasniona,zamigamy szybko !:)
							{
								OCR=wypelnienie-150;
								_delay_ms(100);
							}
							OCR=wypelnienie;
						}
						eeprom_busy_wait();
						__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
						if(tmp!=wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
						{
							OCR=0; //Migniemy na potwierdzenie zapisu :)
							eeprom_busy_wait();
							__EEPUT(0,wypelnienie);
							_delay_ms(300);
						#ifdef UART_DEBUG
							UARTuitoa((uint16_t)wypelnienie, napis);
							uart0_puts_p(S_ZEEPROM);
							uart0_puts(napis);
							uart0_puts_p(S_NL);
						#endif							
						}
						OCR=wypelnienie;
					}
				}
			}			
		} else
		{
			// adc_result = ADC_run();
			// adc_result8 = adc_result / 4;
			// if( adc_result8 != adc_result8_prev )
			// {
			// #ifdef UART_DEBUG
			// 	UARTuitoa(adc_result, napis);
			// 	uart0_puts("pomiar:");
			// 	uart0_puts(napis);
				
			// 	UARTuitoa((uint16_t)adc_result8, napis);
			// 	uart0_puts(":");
			// 	uart0_puts(napis);
			// 	uart0_puts("\r\n");
			// #endif
			// 	while(adc_result8 > adc_result8_prev)
			// 	{
			// 		adc_result8_prev += KROK;
			// 		OCR0A=adc_result8_prev;
			// 		_delay_ms(8);
			// 	}
			// 	while(adc_result8 < adc_result8_prev)
			// 	{
			// 		adc_result8_prev -= KROK;
			// 		OCR0A=adc_result8_prev;
			// 		_delay_ms(8);
			// 	}
			// }			

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
			OCR0A=wypelnienie;
		}
	}
}

uint8_t czekaj(uint8_t naco)
{
	for(uint8_t i=0;i<40;i++)
	{
		if(naco)
		{
			if(SWITCH_PIN & (1<<SWITCH))
			{
				_delay_ms(OPOZNIENIE);
				if(SWITCH_PIN & (1<<SWITCH))
					return 1;
			}
		}else
		{
			if(!(SWITCH_PIN & (1<<SWITCH)))
			{
				_delay_ms(OPOZNIENIE);
				if(!(SWITCH_PIN & (1<<SWITCH)))
					return 1;
			}
		}
		_delay_ms(OPOZNIENIE);
	}

	return 0;
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

uint16_t ADC_run(void)
{
	ADCSRA |= (1<<ADEN); //Wlaczenie przetwornika AD
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); //Preskaler 128 dla przetwornika AD (przy 8MHz clk)
	ADMUX = (1<<REFS0); // AVCC with external capacitor at AREF pin and connect ADC0(PC0)
	ADCSRA |= (1<<ADSC); // Convert
	_delay_ms(1);
	while (ADCSRA & (1<<ADSC));
	uint8_t low = ADCL;
	uint16_t val = ((ADCH&0x03) << 8) | low;
	//discard previous result
	ADCSRA |= (1<<ADSC); // Convert
	while (ADCSRA & (1<<ADSC));
	low = ADCL;
	val = ((ADCH&0x03) << 8) | low;
	_delay_ms(1);
	ADCSRA |= (1<<ADSC); // Convert
	while (ADCSRA & (1<<ADSC));
	low = ADCL;
	val += ((ADCH&0x03) << 8) | low;
	_delay_ms(1);
	ADCSRA |= (1<<ADSC); // Convert
	while (ADCSRA & (1<<ADSC));
	low = ADCL;
	val += ((ADCH&0x03) << 8) | low;

	ADCSRA &= ~(1<<ADEN); //Wylaczenie przetwornika AD
	return val/3;
}

