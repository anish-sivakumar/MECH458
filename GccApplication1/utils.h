/*
 * utils.h
 *
 * Created: 2023-10-18 4:41:53 PM
 *  Author: mech458
 */ 

#ifndef INCFILE1_H_
#define INCFILE1_H_

#include "dc_motor.h"

typedef enum shiftDir
{
	LEFT,
	RIGHT
} shiftDir_t;

void debounceDelay();
void mTimer(int count);
void adc_init();
uint8_t adc_read();

#endif /* INCFILE1_H_ */