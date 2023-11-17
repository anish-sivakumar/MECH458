/*
 * utils.h
 *
 * Created: 2023-10-18 4:41:53 PM
 *  Author: mech458
 */ 

#ifndef UTILS_H_
#define UTILS_H_

#include "dc_motor.h"

typedef enum shiftDir
{
	LEFT,
	RIGHT
} shiftDir_t;

typedef enum cyl
{	// Defined in order of dish spinning clockwise
	BLACK = 0,
	ALUM,
	WHITE,
	STEEL,
	DISCARD // invalid object detected
}cyl_t;

void display_calibration(uint16_t adc_min);
cyl_t get_cyl_type(uint16_t adc_min);
void debounceDelay();
void mTimer(int count);
void adc_init();
uint16_t adc_read();

#endif /* UTILS_H_ */