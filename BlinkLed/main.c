/*
 * BlinkLed.c
 *
 * Created: 22/04/2018 19:37:06
 * Author : Andre Cachopas
 * Fuses Calculator: http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega644
 * Helper for UART Implementation: https://forum.allaboutcircuits.com/threads/atmega-644-uart-strange-output.108189/
 * 
 */ 


//Defines

#ifndef F_CPU
#define F_CPU 8000000UL //8MHz
#endif

# define USART_BAUDRATE 9600
# define BAUD_PRESCALE ((( F_CPU) / (16UL * USART_BAUDRATE) )  - 1)		//BaudRate Prescaller calculation
# define sei()  __asm__ __volatile__ ("sei" ::: "memory")				//Not needed.

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/delay.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>

//Global vars
unsigned short int CNT_500ms = 1;
unsigned short int CNT_50ms = 1;
unsigned char buffer_rx[4];

//Functions declaration
void USART_Transmit( unsigned char data );

//Actual Code

//Initializes the whole HW.
void init(void)
{
	///////////////// deactivates the JTAG (its activated in the fuses) ////////////////
	MCUCR=(1<<JTD);
	MCUCR=(1<<JTD);									//We have to do it twice!	

	/////////////// Ports /////////////////////////
	//DDR as 1 set the PIN as Output. 
	DDRD = 0b11111110;
	DDRB = 0b11100001;								//PB0,PB1,PB2,PB3
	PORTB = 0b00000000;

	////////////////// Configuring the main timer in the TCCR0 for 500ms CNT_500ms //////////////
	OCR0A = 150;
	TCCR0A = 0b00000010;							//WGM01=1
	TCCR0B = 0b00000100;							// 5ms prescaller 64
	TIMSK0 = 0b00000010;

	////////////////// 8Bit PWM PD6 & PD7 In OC2 ///////////////
	TCCR2A = 0b10100101;
	TCCR2B = 0b00000011;							//WGM22 oFF, we have CTC mode of operation
	//Initial value of the OCR2
	OCR2A = 50;
	OCR2B = 147;
	
	///////////// 8Bit PWM PD5 In OC1 //////
	TCCR1A = 0b10100001;
	TCCR1B = 0b00001001;							//WGM22 oFF, we have CTC mode of operation
	TCCR1C = 0b11000000;
	//Setting the initial OCR1 Value, In this Port we only use one!
	OCR1A = 235;
	OCR1B = 235;	
	
	//// UART Config Assync Mode! //////
	UCSR0A = 0;
	UCSR0B = 0b10011000;
	UCSR0C = 0b00000110; // Set format: 8data, 1stop bit 
	UBRR0H = (unsigned char)(BAUD_PRESCALE>>8);
	UBRR0L = (unsigned char)BAUD_PRESCALE;
	//UCSR0B = (1<<RXEN0)|(1<<TXEN0);				// Enable receiver and transmitter function, blocking calls, Not used.
	 	
	CLKPR = 0b10000011;								// Clock divider by 8 is set, Not sure if we need this.
	SREG |= 0x80;									// Enables global interruptions, overrides "sei"
}

//This function is an Interruption. Its called when the Timer 0 reaches the configurated value.
ISR (TIMER0_COMPA_vect) //Timer interruption
{
	CNT_500ms--;
	CNT_50ms--;
	if (CNT_500ms == 0)
	{
		if (PORTB|= ~0b01000000)
			PORTB ^= (1<<6);// toggle		
		else
			PORTB ^= ~(1<<6);//toggle the bit
		
		CNT_500ms = 150;
	}
	if (CNT_50ms == 0)
	{
		OCR2A++;
		if(OCR2A == 254)
			OCR2A = 0;
		
		OCR2B++;
		if(OCR2B == 254)
			OCR2B = 0;
		
		OCR1B++;
		if(OCR1B == 254)
			OCR1B = 0;
			
		OCR1A++;
		if(OCR1A == 254)
			OCR1A = 0;
			
		CNT_50ms = 15;
	}	
}

//This function is called when we receive something in the UART Port
ISR(USART0_RX_vect)
{	
	buffer_rx[0] = UDR0;	
	USART_Transmit(buffer_rx[0]);	
}

//This function is used to read the available data from USART. This function will wait until data is
//available. Not used!
unsigned char USART_Receive( void ) // Extracted function from the datasheet
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) )
	;
	/* Get and return received data from buffer */
	return UDR0;
}

//This function writes the given "data" to the USART which then transmit it via TX line
void USART_Transmit( unsigned char data ) // Extracted function from the datasheet
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
}


int main(void)
{
	init();	
	
    while (1) 
    {		 
    }
}

