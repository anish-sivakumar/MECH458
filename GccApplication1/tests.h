/*
 * tests.h
 *
 * Created: 2023-12-03 11:19:35 AM
 *  Author: chygg
 */ 


#ifndef TESTS_H_
#define TESTS_H_

#include "utils.h"
#include "stepper.h"
#include "lcd.h"

stepperContinueTest()
{	
	LCDWriteIntXY(0,0,stepper.pos,3);
	mTimer(2000);
	
	while (1)
	{
		
		stepper.dir = 0;
		rotateTrapLut(100,0);
		LCDWriteIntXY(0,0,stepper.pos,3);

		mTimer(1000);
		rotateTrapLut(100,2000);
		LCDWriteIntXY(0,0,stepper.pos,3);
		mTimer(1000);
		
		int rotationCw = (200 - stepper.pos)%200;
		LCDWriteIntXY(0,0,stepper.pos,3);
		rotateTrapLut(rotationCw, 0);
		
		LCDWriteIntXY(0,0,stepper.pos,3);
		
		mTimer(2000);
		
		stepper.dir = 1;
		rotateTrapLut(100,0);
		mTimer(1000);
		rotateTrapLut(100,2000);
		mTimer(1000);
		int rotationCCw = (200 - stepper.pos)%200;
		rotateTrapLut(200 - rotationCw, 0);
		LCDWriteIntXY(0,0,stepper.pos,3);

		mTimer(2000);
	}

	
}




#endif /* TESTS_H_ */