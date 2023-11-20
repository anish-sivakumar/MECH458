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
#include "linkedQueue.h"

// Calibration mode enable switch, uncomment to calibrate system
//#define CALIBRATION_MODE

// Object detection threshold value
#define OBJECT_THRESH 990

typedef enum FSM_state
{
	POLLING,
	STEPPER_CONTROL,
	PAUSE,
	END
}FSM_state_t;

// Main FSM state variable
FSM_state_t state = POLLING;

// Motor settings
volatile motor_dir_t motor_dir;
volatile int motor_pwr; //1 is on, 0 is off
volatile uint8_t pwm_setting = 0x80;

// Stepper settings
int last_step = 0;

// ADC classification variables
uint16_t adc_val = 0;
uint16_t adc_min = 1024;
int obj_detect = 0;
cyl_t cyl_type = DISCARD;

// Belt end detect flag
volatile int end_detect = 0;

// Hall effect flag
volatile int heFlag = 0;

// Rampdown flag
volatile int rampdown = 0;

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
	
	// Set up LCD
	InitLCD(LS_BLINK|LS_ULINE);
	LCDClear();
	
	// Set defaults
	motor_dir = forward;
	motor_pwr = 1;
	
	// Object queue variables
	link *qHead, *qTail;
	link *newLink, *poppedLink;
	cyl_t processedCount[4] = {};
	lq_setup(&qHead, &qTail);	
	
	// calibrate the stepper
	while(heFlag == 0)
	{
		rotate(1,0);
	}
	heFlag = 0;
	resetPosition();
	
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
					#ifdef CALIBRATION_MODE
						display_calibration(adc_min);
						
					#else
						cyl_type = get_cyl_type(adc_min);
						if (cyl_type != DISCARD)
						{
							LCDWriteIntXY(8,1,adc_min,4)
							initLink(&newLink);
							newLink->e.itemCode = cyl_type;
							lq_push(&qHead, &qTail, &newLink);

						}
					#endif
					
					// Reset values
					obj_detect = 0;
					adc_min = 0xFFFF;
				}
				
				LCDWriteIntXY(0,1,adc_val,5);
				
				// Check end of belt flag
				if (end_detect)
				{
					state = STEPPER_CONTROL;
					motor_brake();
					end_detect = 0;
					continue;
				}

				// Check rampdown flag
				if (rampdown)
				{
					state = END;
				}
				
				// Print some debug stuff 
				if (qHead != NULL)
				{
					LCDWriteIntXY(0,0,qHead->e.itemCode,1);
				}
				
				// Hardcode a delay 
				mTimer(1);
				
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
				lq_pop(&qHead, &poppedLink);
				basic_align(poppedLink->e.itemCode);
				processedCount[poppedLink->e.itemCode]++;
				LCDWriteIntXY(8,0,poppedLink->e.itemCode,1);
				free(poppedLink);
				state = POLLING;

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

// End of Belt Detect
// There is probably a better way to do this transition, just keeping it simple for now

ISR(INT2_vect)
{
	end_detect = 1;
}

// Hall-Effect Sensor for stepper calibration
ISR(INT3_vect)
{
	heFlag = 1;
}

