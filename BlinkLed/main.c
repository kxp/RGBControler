/*
 * BlinkLed.c
 *
 * Created: 22/04/2018 19:37:06
 * Author : Andre Cachopas
 */ 
#define F_CPU 1000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>

//Not needed.
//# define sei()  __asm__ __volatile__ ("sei" ::: "memory")
//Global vars
unsigned short int CNT_500ms = 1;
unsigned short int CNT_20ms = 1;

void init(void)
{
	///////////////Portos/////////////////////////
	//DDR as 1 set the PIN as Output. 
	DDRD = 0b11111110;
	DDRB = 0b11100001;//LCD PB0,PB1,PB2,PB3
	PORTB = 0b00000000;
	//DDRA=0b00110000; //LCD no PA6 e PA7
	//PORTA=0b00100000;
	//DDRC=0b01111111;//todos como entrada excepto o ultimo q é led a piscar
	//PORTC=0b01111111;//activa os pull ups internos po keypad

	////////////////inicia LCD///////////////
	//LCDInit(LS_NONE);//LCD
	
	///////////////// deactivates the JTAG ////////////////
	MCUCR=(1<<JTD); 
	MCUCR=(1<<JTD);//We have to do it twice!

	////////////////// 8Bit PWM PD6 & PD7 ///////////////
	TCCR2A=0b10100101;
	TCCR2B=0b00000011;//WGM22 oFF, we have CTC mode of operation
	//TCCR2B=0b00001010;//prescaller de 8
	//Max value of the OCR
	OCR2A = 255;
	OCR2B = 255;
	//////////////timer 1, 16bits sonar/////
	//TCCR1A=0X00;
	//TCCR1B=(1<<CS10);
	//TCCR1C=0X00;

	//////////////////////////CNT_500ms//////////////
	OCR0A = 90;//78;
	TCCR0A=0b00000010;//WGM01=1
	TCCR0B=0b00000011;// 5ms pre-sacller 64
	TIMSK0=0b00000010;
	
	/////////////////ADmux///////////
	//ADMUX=0b11100000;
	//ADCSRA=0b10000011;//Divisor de 8, tamos a 1MHz 125
	
	CLKPR = 0b10000011; // Clock divider by 8 is set
	SREG|=0x80;			// Enables global interruptions, overrides "sei"
}

ISR (TIMER0_COMPA_vect) //Timer interruption
{
	CNT_500ms--;
	CNT_20ms--;
	if (CNT_500ms == 0)
	{
		if (PORTB|= ~0b01000000)
			PORTB ^= (1<<6);// toogle		
		else
			PORTB ^= ~(1<<6);//toogle the bit
		
		CNT_500ms=100;
	}
	if (CNT_20ms == 0)
	{
		OCR2A++;	
		if(OCR2A == 254)
			OCR2A = 0;
			
		OCR2B++;
		if(OCR2B == 254)
			OCR2B = 0;
			
		CNT_20ms = 10;
	}	

	return 0;
}


int main(void)
{
	init();	
	
    while (1) 
    {
    }
}

