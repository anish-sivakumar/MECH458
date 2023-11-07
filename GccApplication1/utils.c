/*
 * CFile1.c
 *
 * Created: 2023-10-18 4:42:45 PM
 *  Author: mech458
 */ 

#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <avr/io.h> // the header of I/O port

#include "utils.h"

void debounceDelay()
{
	// capture the button state
	uint8_t buttonStateMask = PINA & 0b00000100;
	
	// wait for the button state to change
	while ((PINA & 0b00000100) == buttonStateMask);
	
	// wait 20ms for debounce
	mTimer(20);
	
	return;
}

void mTimer(int count)
{
	CLKPR = 0x80;
	CLKPR = 0x01; // slow clock to 8MHz
	
	DDRL = 0xff; //set all PORTL pins to output
	
	TCCR1B	|= _BV(CS11); //set timer prescaler to 8. 8Mhz/8 = 1MHz timer freq
	TCCR1B	|= _BV(WGM12); //CTC Mode with TOP being OCR1A value
	OCR1A	= 0x03E8; //output compare register to 1000cycles/1ms
	TCNT1	= 0x0000; //start counting at zero
	//TIMSK1	= TIMSK1 | 0b00000010; //enable output compare interrupt
	TIFR1	|= _BV(OCF1A); //set the output compare flag bit in the timer interrupt flag register.
	//	 OCF1A is cleared by writing one to it. Handled in hardware
	
	
	//run the timer (1ms) "count" times
	int i = 0;
	while(i < count)
	{
		//if timer (1ms) has overflowed+
		
		if ((TIFR1 & 0x02) == 0x02)
		{
			
			TIFR1 |= _BV(OCF1A); //restart timer
			i++;
		}
	}
}

void delaynus(int n) // delay microsecond
{
	int k;
	for(k=0;k<n;k++)
	_delay_loop_1(1);
}
void delaynms(int n) // delay millisecond
{
	int k;
	for(k=0;k<n;k++)
	delaynus(1000);
}
