/* stepper.h */
#ifndef STEPPER_H_
#define STEPPER_H_

#include "utils.h"

int lastPos;
int lastStep;

void resetPosition();

void step(int *iPtr, int dir);

void rotate(int deg, int dir);
void rotateTrapezoid(int deg, int dir);
void rotateTrapLut(int stepsToRun, int dir, int inDelay, int outDelay);


void basicAlign(cyl_t cyl_type);


#endif /* STEPPER_H_ */