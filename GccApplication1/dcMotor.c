/*
 * dc_motor.c
 *
 * Created: 2023-10-25 3:57:22 PM
 *  Author: mech458
 */ 

#include <avr/io.h> // the header of I/O port

#include "dcMotor.h"
#include "utils.h"

motorDir_t current_dir; // global to store direction

void pwmInit()
{
	  // Configure TCCR0A and TCCR0B registers for fast PWM mode with TOP = 0xFF (255)
	  TCCR0A = (1 << WGM01) | (1 << WGM00);

	  // Set compare match output mode to clear on match
	  TCCR0A |= (1 << COM0A1);

	  // Set prescaler to 8
	  TCCR0B |= (1 << CS01); // datasheet: f0Cnx = fclk / (N*256), where N = prescale value of 1,8,64,256, or 1024)

}

void pwmSet(uint8_t compareVal)
{
	// Set the value of Output Compare Register A (OCRA) for the provided duty cycle
	OCR0A = compareVal; 
}

void motorJog(motorDir_t dir, uint8_t compareVal) //direction (ccw - fwd)
{
	
	pwmSet(compareVal);
	PORTB = (PORTB & 0b11110000) | 0b00001110; //INb HI and enables HI
	
}

void motorBrake() //break HI
{
	PORTB = (PORTB & 0b11110000) | 0b00001111;	//INa/INb = 1/1 enables HI
}

