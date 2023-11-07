/*
 * utils.h
 *
 * Created: 2023-10-18 4:41:53 PM
 *  Author: mech458
 */ 




#ifndef INCFILE1_H_
#define INCFILE1_H_

typedef enum shiftDir
{
	LEFT,
	RIGHT
} shiftDir_t;

void debounceDelay();
void mTimer(int count);




#endif /* INCFILE1_H_ */