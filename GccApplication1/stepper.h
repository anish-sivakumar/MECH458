/* stepper.h */
#ifndef STEPPER_H_
#define STEPPER_H_

#include "utils.h"

int last_state;

void resetPosition();

void rotate(int deg, int dir);
void rotateTrapezoid(int deg, int dir);

void basicAlign(cyl_t cyl_type);


#endif /* STEPPER_H_ */