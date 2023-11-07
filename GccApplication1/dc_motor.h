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

typedef enum motor_dir
{
	forward,
	reverse,
	brake,
	invalid
}motor_dir_t;

void pwm_init();
void pwm_set(uint8_t compare_val);

void motor_jog(motor_dir_t dir, uint8_t compare_val);
void motor_brake();

#endif /* DC_MOTOR_H_ */