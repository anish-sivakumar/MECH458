/*
 * dc_motor.h
 *
 * Created: 2023-10-25 3:57:46 PM
 *  Author: mech458
 */ 

#include <stdlib.h> // the header of the general-purpose standard library of C programming language
#include <stdint.h>

#ifndef DC_MOTOR_H_
#define DC_MOTOR_H_

typedef enum motorDir
{
	forward,
	reverse,
	brake,
	invalid
}motorDir_t;

void pwmInit();
void pwmSet(uint8_t compareVal);

void motorJog(motorDir_t dir, uint8_t compareVal);
void motorBrake();

#endif /* DC_MOTOR_H_ */