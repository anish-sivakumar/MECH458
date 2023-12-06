/* ##################################################################
# PROJECT: MECH 458 Final Project
# GROUP: 3
# NAME 1: First Name, Last Name, Student ID
# NAME 2: First Name, Last Name, Student ID
# DESC: This program does� [write out a brief summary]
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
#include "dcMotor.h"
#include "lcd.h"
#include "linkedQueue.h"


// Calibration mode enable switch, uncomment to calibrate system
// #define CALIBRATION_MODE

// Stepper test mode 
// #define STEPPER_TEST

#ifdef STEPPER_TEST
#include "tests.h"
#endif

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
volatile motorDir_t motorDir;
volatile uint8_t motorPwm = 0x90;

// Stepper settings
int stepperLastPos = 0;

// ADC classification variables
uint16_t adcVal = OBJECT_THRESH;
uint16_t adcMin = OBJECT_THRESH;
uint16_t adcReadings = 0;
int objDetect = 0;
cyl_t cylType = DISCARD;

// Belt end detect flag
volatile int edFlag = 0;

// Hall effect flag
volatile int heFlag = 0;

// Rampdown flag
volatile int rdFlag = 0;

// Pause flag
volatile int pFlag = 0;

// Drop timer flag
volatile int dFlag = 1;

/* ################## MAIN ROUTINE ################## */

int main(int argc, char *argv[]){
	
	// Pin config
	DDRA = 0b11111111; // PORTA = output
	DDRB = 0b11111111; // PORTB = output
	DDRC = 0b11111111; // PORTC = output
	DDRD = 0b00000000; // PORTD = input
	DDRE = 0b00000000; // PORTE = input
	DDRL = 0b11111111; // PORTL = output

	// Set initial outputs
	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00; 
	
		
	// slow clock to 8MHz
	CLKPR = 0x80;
	CLKPR = 0x01; 
	
	// Disable all interrupts
	cli();
	
	// Perform component initialization while interrupts are disabled	
	timerInit();
	pwmInit(); 
	adcInit(); 
	eiInit();
	// Re-enable all interrupts
	sei();		
	
	// Set up LCD
	InitLCD(LS_BLINK|LS_ULINE);
	LCDClear();
	
	// Set defaults
	motorDir = forward;
	
	// Object queue variables
	link *qHead, *qTail;
	link *newLink, *poppedLink;
	cyl_t processedCount[4] = {};
	lqSetup(&qHead, &qTail);	
	
	// calibrate the stepper
	while(heFlag == 0)
	{
		rotate(1,0);
	}
	heFlag = 0;
	resetPosition();
	
	// testing routines
	#ifdef STEPPER_TEST
	stepperContinueTest();
	#endif 
		
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
				motorJog(forward,motorPwm);
				
				// Get ADC reading
				adcVal = adcRead();
				
				// Object processing logic
				if ((adcVal < OBJECT_THRESH) && (PINE & 0x10) ) //AND OR-bit
				{
					// Were detecting an object
					objDetect = 1;
					if (adcVal < adcMin)
					{
						adcMin = adcVal;
					}
					adcReadings++;
					
				}
				else if (objDetect)
				{
					// Object has finished passing through, process it
					#ifdef CALIBRATION_MODE
						displayCalibration(adcMin, adcReadings);
						
					#else
						cylType = getCylType(adcMin);
						if (cylType != DISCARD && adcReadings > 5)
						{
							//LCDWriteIntXY(8,1,adcMin,4)
							initLink(&newLink);
							newLink->e.itemCode = cylType;
							lqPush(&qHead, &qTail, &newLink);

						}
					#endif
					
					// Reset values
					objDetect = 0;
					adcMin = 0xFFFF;
					adcReadings = 0;
				}
				
				//LCDWriteIntXY(0,1,adcVal,5);
				
				// Check rampdown flag
				if (rdFlag)
				{
					LCDWriteStringXY(12,0,"RDWN");
					if (lqIsEmpty(&qHead))
					{
						// Wait a bit to let the last piece fall
						mTimer(300);
						state = END;
					}
				}
				
				#ifndef CALIBRATION_MODE
				// Check end of belt flag
				if (edFlag)
				{
					
					state = STEPPER_CONTROL;
					motorBrake();
					edFlag = 0;					
					continue;
				}
				
				// Print some debug stuff 
				LCDWriteIntXY(8,0,lqSize(&qHead,&qTail),2);
				
				if (qHead != NULL)
				{
					LCDWriteIntXY(0,0,qHead->e.itemCode,1);
				}
				else
				{
					LCDWriteIntXY(0,0,4,1);
				}
				#endif
				
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
				
				lqPop(&qHead, &qTail, &poppedLink);
				
				// Wait if dFlag not set (drop timer to finish)
				while(dFlag == 0); 
				
				dFlag = smartAlign(poppedLink->e.itemCode, &qHead, &qTail); 
				
				LCDWriteIntXY(14,1,dFlag,2);
				if (dFlag == 0){ //if we are rotating
					PORTL = 0b000000000;
					// Enable Timer 4 
					TCCR4B |= (1 << CS42);
				}
				
			
				processedCount[poppedLink->e.itemCode]++;
				//LCDWriteIntXY(4,0,poppedLink->e.itemCode,1);
				free(poppedLink);
				motorJog(motorDir, motorPwm);
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
				motorBrake();
				LCDClear();
				LCDWriteStringXY(0,0,"BL: WH: AL: ST:");
				LCDWriteIntXY(0 ,1,processedCount[BLACK],3);
				LCDWriteIntXY(4 ,1,processedCount[WHITE],3);
				LCDWriteIntXY(8 ,1,processedCount[ALUM ],3);
				LCDWriteIntXY(12,1,processedCount[STEEL],3);
				return (0); 	
		}
	}
	return (0); 
}

//  switch 0 - rampdown
ISR(INT0_vect)
{ 
	rdFlag = 1;
}

//  switch 1 - pause
ISR(INT1_vect)
{ 
	pFlag = 1;
}

// End of Belt Detect
// There is probably a better way to do this transition, just keeping it simple for now

ISR(INT2_vect)
{
	edFlag = 1;
}

// Hall-Effect Sensor for stepper calibration

ISR(INT3_vect)
{
	heFlag = 1;
}

int on = 1;

// Timer 4 overflow interrupt service routine sets the drop flag
ISR(TIMER4_COMPA_vect) {
	dFlag = 1;
	TCCR4B |= (0 << CS42); //disable timer
	
	PORTL = 0b11111111;
	/*
	if (on == 1){
		PORTL = 0b11111111;
		on = 0;
	}
	else {
		PORTL = 0x00;
		on = 1;
	}
	*/

}
