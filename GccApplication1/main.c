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
#include "utils.h"
#include "stepper.h"
#include "dc_motor.h"
#include "lcd.h"
#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <avr/io.h> // the header of I/O port

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
	uint8_t pwm_setting = 0x00;
	
	// Set up LCD
	InitLCD(LS_BLINK|LS_ULINE);
	LCDClear();
	
	////////////////////////////
	// Main FSM control loop ///
	////////////////////////////
	state = STEPPER_CONTROL; // for testing
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
						-> Go to STEPPER_CONTROL
					If the pause button is pressed:
						-> Go to PAUSE
					If the object queue is empty and the rampdown flag has been set:
						-> Go to END	
				*/
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
				rotate2(33);
				rotate2(200)
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
				
				return (0); 
			
		}
		
	}
	return (0); 

}

//  switch 0 - direction
ISR(INT0_vect)
{ // when there is a rising edge
	if (motor_dir == forward)
	{
		motor_dir = reverse;		
	} else if (motor_dir == reverse)
	{
		motor_dir = forward;		
	}
}

//  switch 1 - on/off
ISR(INT1_vect)
{ // when there is a rising edge
	if (motor_pwr == 0)
	{
		motor_pwr = 1;
	} else if (motor_pwr == 1)
	{
		motor_pwr = 0;
	}

}

