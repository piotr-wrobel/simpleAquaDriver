
//********* UWAGA !! Na fabrycznym procku nale¿y zdj¹æ FUSEBIT CKDIV8 
// i ustawiæ Fast Rising Power - bity SUT[1:0] na 01  ***********


#include <avr/eeprom.h>
#include <avr/io.h>
#include <util/delay.h>

#define OPOZNIENIE 10
#define WCISNIETY 0
#define ZWOLNIONY 1
#define KROK 1
#define WYP_MIN 3


//definicje PINÓW
#define WY PB0
#define WE PB3

uint8_t czekaj(uint8_t naco)
{
	for(uint8_t i=0;i<40;i++)
	{
		if(naco)
		{
			if(PINB & (1<<WE))
			{
				_delay_ms(OPOZNIENIE);
				if(PINB & (1<<WE))
					return 1;
			}
		}else
		{
			if(!(PINB & (1<<WE)))
			{
				_delay_ms(OPOZNIENIE);
				if(!(PINB & (1<<WE)))
					return 1;
			}
		}
		_delay_ms(OPOZNIENIE);
	}

	return 0;
}


int main(void)
{
    
	// next four instructions. // Niepotrzebne, wy³¹czony fuse bit CKDIV8
    //CLKPR=(1<<CLKPCE); 
    //CLKPR=0; // 8 MHZ
	
	
	//########### I/O ###########
	//Ustawienie pinów
    DDRB  |= (1<<WY); // jako wyjœcia
    PORTB |=  (1<<WY); //Stan wysoki, wygaszenie lampki
	DDRB  &=~ (1<<WE); //Ustawienie pinów klawiszy jako wejœcie 
    PORTB |=  (1<<WE); //w³¹czenie rezystora podci¹gaj¹cego tzw. Pull_up

    //############# PWM Fast #############
	TCCR0A |= (1<<COM0A0) | (1<<COM0A1) | (1<<WGM00) | (1<<WGM01); //Fast PWM, set OC0A on compare match, clear at BOTTOM
	TCCR0B |= (1<<CS01); // Internal clock, prescaler 64 f= CPU clock / 8 * 256  (9,6MHz/8*256=4,6kHz)
	
	OCR0A=WYP_MIN; // Na poczatek wypelnienie min, lampka przyciemniona
	uint8_t wypelnienie=0,tmp=0;
	
	eeprom_busy_wait();
	__EEGET(wypelnienie,0); //Wczytujemy z pamieci zapisane wypelnienie
	
	for(tmp=WYP_MIN;tmp<wypelnienie;tmp+=KROK) // Plynnie rozjasniamy do osiagniecia zapisanego w eeprom
	{
		OCR0A=tmp;
		_delay_ms(OPOZNIENIE);
	}
	OCR0A=wypelnienie;

	while(1) //Pêtla g³ówna
	{
		if(!(PINB & (1<<WE)))//jeœli klawisz jest wduszony
		{
			_delay_ms(50); // Czekamy chwilke
			if(!(PINB & (1<<WE))) // Nadal wduszony
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
							OCR0A=wypelnienie+20;
							_delay_ms(100);
						}
							
						OCR0A=wypelnienie;
					}
					eeprom_busy_wait();
					__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
					if(tmp!=wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
					{
						OCR0A=0; //Migniemy na potwierdzenie zapisu :)
						eeprom_busy_wait();
						__EEPUT(0,wypelnienie);
						_delay_ms(300);
					}
					OCR0A=wypelnienie;	
				}else //Nie bylo zwolnienia klawisza, czyli rozjasniamy
				{
					while(czekaj(WCISNIETY)) // Rozjasniamy o KROK w petli, z opoznieniem
					{
						if(wypelnienie<=(255-KROK))
							wypelnienie+=KROK;
						else //Juz jest maxx rozjasniona,zamigamy szybko !:)
						{
							OCR0A=wypelnienie-150;
							_delay_ms(100);
						}
						OCR0A=wypelnienie;
					}
					eeprom_busy_wait();
					__EEGET(tmp,0); // Wczytujemy poprzednie ustawienie z EEPROM
					if(tmp!=wypelnienie) // Jesli sie zmienilo, to zapisujemy nowe ustawienie
					{
						OCR0A=0; //Migniemy na potwierdzenie zapisu :)
						eeprom_busy_wait();
						__EEPUT(0,wypelnienie);
						_delay_ms(300);
					}
					OCR0A=wypelnienie;
				}
			}
		}
	}
}

