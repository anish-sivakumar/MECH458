/*
 * utils.h
 *
 * Created: 2023-10-18 4:41:53 PM
 *  Author: mech458
 */ 

#ifndef UTILS_H_
#define UTILS_H_

#include "dcMotor.h"

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

void displayCalibration(uint16_t adc_min);
cyl_t getCylType(uint16_t adc_min);
void debounceDelay();
void dTimer(uint32_t count);
void mTimer(uint32_t count);
void adcInit();
uint16_t adcRead();

#endif /* UTILS_H_ */