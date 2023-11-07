/* ##################################################################
# MILESTONE: 1
# PROGRAM: 1
# PROJECT: Lab1 Demo
# GROUP: 3
# NAME 1: First Name, Last Name, Student ID
# NAME 2: First Name, Last Name, Student ID
# DESC: This program does… [write out a brief summary]
# DATA
# REVISED ############################################################### */
#include <util/delay_basic.h>
#include "utils.h"
#include "stepper.h"
#include "dc_motor.h"
#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <avr/io.h> // the header of I/O port

/* ################## MAIN ROUTINE ################## */

int main(int argc, char *argv[]){
	DDRA = 0b11111111; //setting PORTA to input
	PORTA = 0b00000000;
	
	DDRB = 0b11111111;// set up portB to output
	PORTB = 0x00;
	
	DDRC = 0b11111111; //setting PORTC to output
	PORTC = 0x00; //setting PORTC outputs to off
	
	DDRL = 0b11111111; // Sets all pins on PORTL to output
	
	CLKPR = 0x01; // slow clock to 8MHz
	
	// set up PWM
	pwm_init();
	    
	//dc motor test
	//PORTB = 0b00001111; 
	//while(1);
	motor_jog(forward, 0x80); //0x80 is 50% duty cycle
	mTimer(50000);
	motor_brake();
	
	//Stepper Demo Code
	/*
    int k = rotate(0,100,0);
    mTimer(2000);
    // rotate 30 deg
	k = rotate(k,17,0);
    mTimer(1000);
    // rotate 60 deg
    k = rotate(k,33,0);
    mTimer(1000);
    // rotate 180 deg
    k = rotate(k,100,0);
	mTimer(2000);
	// rotate 30 deg
	k = rotate(k,17,1);
	mTimer(1000);
	// rotate 60 deg
	k = rotate(k,33,1);
	mTimer(1000);
	// rotate 180 deg
	k = rotate(k,100,1);
	mTimer(1000);
	*/
	
	//LAB4A: lcd: fwd,rev, duty cycle on lcd
	
	return (0); 

}

