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

//Code defines:
#define UART_BUFFER_SIZE  5												//all commands must have 5 chars, but only 4 are used.

//Includes
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>

//Global vars
unsigned short int CNT_500ms = 1;
unsigned short int CNT_50ms = 1;
unsigned char buffer_rx[UART_BUFFER_SIZE];
unsigned short int buffer_position = 0;

//Functions declaration
void USART_Transmit( unsigned char data );
short process_command();
void clear_buffer();
void send_string(unsigned char* string);


//Sets the colors and returns the actual value
short int set_red(unsigned short int value);
short int set_blue(unsigned short int value);
short int set_green(unsigned short int value);

//Actual Code

//Initializes the whole HW.
void init_hw(void)
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
	TCCR2B = 0b00000001;							//WGM22 oFF, we have CTC mode of operation
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
	//CNT_50ms--;
	if (CNT_500ms == 0)
	{
		if (PORTB|= ~0b01000000)
			PORTB ^= (1<<6);// toggle		
		else
			PORTB ^= ~(1<<6);//toggle the bit
		
		CNT_500ms = 150;
	}
	//if (CNT_50ms == 0)
	//{
		//OCR2A++;
		//if(OCR2A == 254)
			//OCR2A = 0;
		//
		//OCR2B++;
		//if(OCR2B == 254)
			//OCR2B = 0;
		//
		//OCR1B++;
		//if(OCR1B == 254)
			//OCR1B = 0;
			//
		//OCR1A++;
		//if(OCR1A == 254)
			//OCR1A = 0;
			//
		//CNT_50ms = 15;
	//}	
}

//This function is called when we receive something in the UART Port
ISR(USART0_RX_vect)
{	
	// avoid memory overrun.
	if(buffer_position >= UART_BUFFER_SIZE)
	{
		buffer_position = 0;		//resets the comunication
		return;
	}

	//store and increment
	buffer_rx[buffer_position++] = UDR0;

	USART_Transmit(buffer_rx[buffer_position - 1]);

	//if (buffer_position == UART_BUFFER_SIZE)//UART_MIN_SIZE)
	if (process_command() != 0)	
	{
		send_string("error\0");
		//send_string(buffer_rx);
		clear_buffer();
	}	
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

//Processes the UART command
short process_command()
{
	if (buffer_position <= 0)
		return 0;

	unsigned short int color = 0;

	switch(buffer_rx[0])
	{
		case 'o':
		case 'O':				//turns off the LED's
			if(buffer_rx[1] == 0xff)
				return 0;

			if(buffer_rx[1] != 'f' && buffer_rx[1] != 'F' )
				return -1;

			if(buffer_rx[2] == 0xff)
				return 0;

			if(buffer_rx[2] != 'f' && buffer_rx[2] != 'F')
				return -1;

			set_blue(0);
			set_green(0);
			set_red(0);
			//when we are ready we clear the buffer
			clear_buffer();

			break;
		case'W':
		case'w':					//Full bright.
			if(buffer_rx[1] == 0xff)
				return 0;

			set_blue(255);
			set_green(255);
			set_red(255);
			clear_buffer();			//when we are ready we clear the buffer
			
			break;		
		case '0':					//emergency turn off!
			if(buffer_rx[1] == 0xff)
				return 0;
				
			set_blue(0);
			set_green(0);
			set_red(0);
			clear_buffer();
			
			break;
		case 'r':
		case 'R':					//sets the red color.
			if(buffer_rx[1] == 0xff)
				return 0;
			if(buffer_rx[2] == 0xff)
				return 0;
			if(buffer_rx[3] == 0xff)
				return 0;
			
			color = (buffer_rx[1] - 0x30) * 100 + (buffer_rx[2] - 0x30) * 10 + (buffer_rx[3] - 0x30);
			set_red(color);
			clear_buffer();			//when we are ready we clear the buffer
			
			break;
		case 'G':
		case 'g':					//sets the green color
			if(buffer_rx[1] == 0xff)
			return 0;
			if(buffer_rx[2] == 0xff)
			return 0;
			if(buffer_rx[3] == 0xff)
			return 0;
			
			color = (buffer_rx[1] - 0x30) * 100 + (buffer_rx[2] - 0x30) * 10 + (buffer_rx[3] - 0x30);
			set_green(color);
			clear_buffer();			//when we are ready we clear the buffer
			
			break;
		case 'b':
		case 'B':					//sets the blue color
			if(buffer_rx[1] == 0xff)
			return 0;
			if(buffer_rx[2] == 0xff)
			return 0;
			if(buffer_rx[3] == 0xff)
			return 0;
			
			color = (buffer_rx[1] - 0x30) * 100 + (buffer_rx[2] - 0x30) * 10 + (buffer_rx[3] - 0x30);
			set_blue(color);
			clear_buffer();			//when we are ready we clear the buffer

			break;
	}
	return 0;
}


int main(void)
{
	clear_buffer();
	init_hw();		

    while (1) 
    {		 
    }
}

//Helper functions

short int set_red(unsigned short int value)
{
	if(value < 0 || value > 0xFF)
		return 0;

	OCR2A = value;
	
	return OCR2A;
}

short int set_blue(unsigned short int value)
{
	if(value < 0 || value > 0xFF)
		return 0;

	OCR1A = value;
	OCR1B = value;

	return OCR1B;	
}

short int set_green(unsigned short int value)
{
	if(value < 0 || value > 0xFF)
		return 0;

	OCR2B = value;
	
	return OCR2B;
}

//sends a string to the UART
void send_string(unsigned char* string)
{
	while (*string != '\0')
	{
		USART_Transmit(*string++);
	}
}

//clears the UART buffer
void clear_buffer()
{
	unsigned short i;
	buffer_position = 0;
	for (i = 0; i < UART_BUFFER_SIZE; i++)
		buffer_rx[i] = 0xff;
}

