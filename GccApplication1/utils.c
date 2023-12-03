/*
 * CFile1.c
 *
 * Created: 2023-10-18 4:42:45 PM
 *  Author: mech458
 */ 

#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <avr/io.h> // the header of I/O port
#include <avr/interrupt.h>

#include "utils.h"
#include "lcd.h"

// goes high when ADC is done 
volatile unsigned int ADC_result_flag;

// Calibration Settings
#define BLACK_THRESH 968
#define WHITE_THRESH 960
#define STEEL_THRESH 850
#define ALUM_THRESH 400
uint16_t adc_total_min = 0;
uint16_t adc_total_max = 0;

void displayCalibration(uint16_t adc_min)
{
	// If this is the first pass, set both total min and total max
	if(adc_total_min == 0 && adc_total_max == 0)
	{
		adc_total_max = adc_min;
		adc_total_min = adc_min;
	}
	// Otherwise, update the total min and total max values accordingly
	else if (adc_min > adc_total_max)
	{
		adc_total_max = adc_min;
	}
	else if (adc_min < adc_total_min)
	{
		adc_total_min = adc_min;
	}
	LCDWriteIntXY(0,0,adc_total_max,5);
	LCDWriteIntXY(6,0,adc_total_min,5);
}

cyl_t getCylType(uint16_t adc_min)
{
	if (adc_min < ALUM_THRESH)
	{
		return ALUM;
	}
	else if (adc_min < STEEL_THRESH)
	{
		return STEEL;
	}
	else if (adc_min < WHITE_THRESH)
	{
		return WHITE;
	}
	else if (adc_min < BLACK_THRESH)
	{
		return BLACK;
	}
	else 
	{
		return DISCARD;
	}
}

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

void timerInit()
{
	// mTimer and dTimer  init
	TCCR1B	|= _BV(CS11); //set timer prescaler to 8. 8Mhz/8 = 1MHz timer freq
	TCCR1B	|= _BV(WGM12); //CTC Mode with TOP being OCR1A value
	TCNT1	= 0x0000; //start counting at zero
	TIFR1	|= _BV(OCF1A); //set the output compare flag bit in the timer interrupt flag register.
	
	// stepTimer init
	TCCR3A = 0;  // Set TCCR3A to 0 for normal operation
	TCCR3B = 0;
	TCCR3B	|= _BV(CS31); //set timer prescaler to 8. 8Mhz/8 = 1MHz timer freq
	TCCR3B	|= _BV(WGM32); //CTC Mode with TOP being OCR3A value
	TCNT3	= 0x0000; // start counting at zero
	// Dont enable interrupt yet
}

void mTimer(uint32_t count)
{
	OCR1A	= 0x03E8; //output compare register to 1ms
	//run the timer (.1ms) "count" times
	uint32_t i = 0;
	while(i < count)
	{
		//if timer (.1ms) has overflowed
		if ((TIFR1 & 0x02) == 0x02)
		{
			TIFR1 |= _BV(OCF1A); //restart timer
			i++;
		}
	}
}

void dTimer(uint32_t count)
{
	OCR1A	= 0x00A; //output compare register to 100cycles/.1ms
	
	//run the timer (.1ms) "count" times
	uint32_t i = 0;
	while(i < count)
	{
		//if timer (.1ms) has overflowed
		if ((TIFR1 & 0x02) == 0x02)
		{
			TIFR1 |= _BV(OCF1A); //restart timer
			i++;
		}
	}
}

void adcInit()
{	
	// config ADC 
	// by default, the ADC input (analog input is set to be ADC0 / PORTF0
	ADCSRA |= _BV(ADEN); // enable ADC
	
	ADCSRA |= _BV(ADIE); // enable interrupt of ADC
	
	ADMUX |= _BV(REFS0); // Read Technical Manual & Complete Comment
	
	// make sure adc result flag is not set
	ADC_result_flag = 0;
}

uint16_t adcRead()
{
	
	// trigger ADC read
	ADCSRA |= _BV(ADSC);
	
	// wait on ADC result flag
	while (ADC_result_flag != 1);
	
	// reset adc result flag
	ADC_result_flag = 0;
	
	// return the adc result
	return ADCW;
}

//ISRs

ISR(ADC_vect)
{
	ADC_result_flag = 1;
}

void eiInit()
{
	EIMSK |= (_BV(INT0 ) | _BV(INT1 ) | _BV(INT2 ) | _BV(INT3 )); // enable INT 0-3
	EICRA |= (_BV(ISC01) | _BV(ISC00) | _BV(ISC11) | _BV(ISC10)); // rising edge interrupt for 0 and 1
	EICRA |= (_BV(ISC21) | _BV(ISC31)); // falling edge for 2 and 3
}