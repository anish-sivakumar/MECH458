/* stepper.h */
#ifndef STEPPER_H_
#define STEPPER_H_

#include "utils.h"
#include <avr/interrupt.h>


typedef struct stepper
{
	int pos;		// the current stepper position
	int step;		// the last stepper phase, from 0 to 3
	int dir;		// the direction the stepper is turning
	int delay;		// the current delay setting of the stepper. 0 means off.
	int continues;	// flag to state if the stepper is to be run asynchronously
	int syncReq;
}stepper_t;

stepper_t stepper;


void resetPosition();
void step();
void stepperIntDisable();
void stepperSetContinue(int continues, uint16_t delay);
void rotate(int deg, int dir);
void rotateTrapezoid(int deg, int dir);
void rotateTrapLut(int stepsToRun, uint16_t outDelay);

void basicAlign(cyl_t cyl_type);


#endif /* STEPPER_H_ */