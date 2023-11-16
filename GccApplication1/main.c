/* ##################################################################
# PROJECT: MECH 458 Final Project
# GROUP: 3
# NAME 1: First Name, Last Name, Student ID
# NAME 2: First Name, Last Name, Student ID
# DESC: This program does… [write out a brief summary]
# DATA
# REVISED ############################################################### */

#include <util/delay_basic.h>
#include <avr/interrupt.h>
#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <stdio.h> 
#include <avr/io.h> // the header of I/O port

// Custom includes
#include "utils.h"
#include "stepper.h"
#include "dc_motor.h"
#include "lcd.h"


// Object calibration values
#define OBJECT_THRESH 1012
#define BLACK_THRESH 995
#define WHITE_THRESH 950
#define STEEL_THRESH 900
#define ALUM_THRESH 500

typedef enum FSM_state
{
	POLLING,
	STEPPER_CONTROL,
	PAUSE,
	END
}FSM_state_t;

FSM_state_t state = POLLING;
volatile motor_dir_t motor_dir;
volatile unsigned int motor_pwr; //1 is on, 0 is off
volatile uint8_t pwm_setting = 0x80;

volatile unsigned int rampdown = 0;

// ADC classification variables
uint16_t adc_val = 0;
uint16_t adc_min = 0;
uint16_t adc_total_min = 0;
uint16_t adc_total_max = 0; 
int obj_detect = 0;

/* ################## MAIN ROUTINE ################## */

int main(int argc, char *argv[]){
	
	// Pin config
	DDRA = 0b11111111; // PORTA = output
	DDRB = 0b11111111; // PORTB = output
	DDRC = 0b11111111; // PORTC = output
	DDRD = 0b00000000; // PORTD = input
	DDRL = 0b11111111; // PORTL = output

	// Set initial outputs
	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00; 
		
	// slow clock to 8MHz
	CLKPR = 0x01; 
	
	// Disable all interrupts
	cli();
	
	// Perform component initialization while interrupts are disabled	
	pwm_init(); 
	adc_init(); 
	EI_init();
	
	// Re-enable all interrupts
	sei();		
	
	// Set defaults
	motor_dir = forward;
	motor_pwr = 1;
	
	// Set up LCD

	InitLCD(LS_BLINK|LS_ULINE);
	LCDClear();
	
	///////////////////////////
	// Main FSM control loop //
	///////////////////////////
	while(1)
	{
		switch (state)
		{
			case POLLING:
				/* 
					In this state we perform the following tasks:
					- The reflection sensor is polled at a constant rate
					- Objects are added and removed from the object queue
					- The DC motor is run at max speed
					- The stepper motor can be run at a constant speed 
					
					Exit conditions:
					If the beam sensor is triggered and the plate is not in the correct position:
						-- Compute stepper command
						-> Go to STEPPER_CONTROL
					If the pause button is pressed:
						-> Go to PAUSE
					If the object queue is empty and the rampdown flag has been set:
						-> Go to END	
				*/
				
				motor_jog(forward,0x80);
				
				// Get ADC reading
				adc_val = adc_read();
				
				// Object processing logic
				if (adc_val < OBJECT_THRESH)
				{
					// Were detecting an object
					obj_detect = 1;
					if (adc_val < adc_min)
					{
						adc_min = adc_val;
					}
				}
				else if (obj_detect)
				{
					// Object has finished passing through, process it
					
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
					
					// Reset values
					obj_detect = 0;
					adc_min = 0xFFFF;
				}
				
				LCDWriteIntXY(0,1,adc_val,5);

				// Check rampdown flag
				if (rampdown)
				{
					state = END;
				}
				
				// Hardcode a delay 
				mTimer(10);
				
				break;
			
			case STEPPER_CONTROL:
				/* 
					In this state we only focus on controlling the stepper 
					as efficiently as possible. 
					
					Exit conditions:
					If the stepper is in position to drop the next object:
						-> Go to POLLING
					If the pause button is pressed:
						-> Go to PAUSE
					
				*/
				
				rotate(33);
				rotate(200);
				mTimer(2000);
				break;
				
			case PAUSE:
				/*
					Upon entering this state, both motors are stopped and 
					the previous state is saved. We exit if the pause button 
					is pressed again
					
					Exit conditions:
					If the pause button is pressed and we entered from POLLING:
						-> Go to POLLING
					If the pause button is pressed and we entered from STEPPER_CONTROL:
						-> Go to STEPPER_CONTROL
				*/
				break;
			
			case END:
				/*
					In this state, motors are turned off and the program terminates.
				*/
				
				motor_brake();
				return (0); 
			
		}
		
	}
	return (0); 

}



//  switch 0 - rampdown
ISR(INT0_vect)
{ 
	rampdown = 1;
}

//  switch 1 - on/off
ISR(INT1_vect)
{ // when there is a rising edge
	if (motor_pwr == 0)
	{
		motor_jog(forward, pwm_setting);
		motor_pwr = 1;
	} 
	else if (motor_pwr == 1)
	{
		motor_brake();
		motor_pwr = 0;
	}
}

