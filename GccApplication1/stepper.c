/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include <avr/io.h>

#define accelLutSz 24
#define decelLutSz 6
const int accel[accelLutSz]	= {1200, 1049, 945, 868, 807, 758, 717, 682, 652, 625, 602, 581, 562, 545, 529, 515, 501, 489, 478, 467, 457, 448, 439, 435};
const int decel[decelLutSz]	= {1200, 807, 652, 602, 562, 529};

typedef enum stepperPhase
{
	UP,
	DOWN,
	HI,
	LO
}stepperPhase;

/*takes in last step number, degrees we want to turn, and the direction (0 = cw and 1 = ccw)
and returns the last step we ran*/

void resetPosition()
{
	lastPos%=4;
}

void step(int *iPtr, int dir)
{
	int i = *iPtr;
	//increment or decrement i & handle overflow
	if (dir == 0) //cw
	{
		i = (i+1)%4;
	} else if (dir == 1) //ccw
	{
		i = (i == 0) ? 3 : i - 1;
	}
	
	switch (i)
	{
		//assuming PORTA bits mean this: (e1,l1,l2,e2,l3,l4, zero, zero)
		case 0:
		PORTA = 0b11011000;
		break;

		case 1:
		PORTA = 0b10111000;
		break;

		case 2:
		PORTA = 0b10110100;
		break;

		case 3:
		PORTA = 0b11010100;
		break;

		default:
		return; //something went wrong
		
	}//switch
	
	*iPtr = i;
}

// returns the out
void rotateTrapezoid(int stepsToRun, int dir) 
{
	int stepsCount = 0;
	int i = lastStep;
	int delay;
	
	/*
	int speed = 45; //steps/sec = 1/delay
	
	//profile parameters
	int maxSpeed = 160; // 8.3ms delay
	int minSpeed = 40;
	int upRate = 6;
	int downRate = 15;
	*/
	
	
	int speed = 90; //steps/sec = 1/delay
	
	//profile parameters
	int maxSpeed = 280; // 8.3ms delay
	int minSpeed = 80;
	int upRate = 10;
	int downRate = 30;
	
	
	stepperPhase phase = UP;
	
	int constSteps = stepsToRun - (maxSpeed-minSpeed)/upRate - (maxSpeed-minSpeed)/downRate; //28 based off maxSpeed = 160; minSpeed = 40;  upRate = 6;  downRate = 15;
	int constCount = 0;
	
	while(stepsCount < stepsToRun)
	{	
		//increment or decrement i & handle overflow
		if (dir == 0) //cw
		{
			i = (i+1)%4;
		} else if (dir == 1) //ccw
		{
			i = (i == 0) ? 3 : i - 1;
		}
		
		switch (i)
		{
			//assuming PORTA bits mean this: (e1,l1,l2,e2,l3,l4, zero, zero)
			case 0:
			  PORTA = 0b11011000;
		      break;

		    case 1:
              PORTA = 0b10111000;
		      break;

		    case 2:
		      PORTA = 0b10110100;
		      break;

		    case 3:
		      PORTA = 0b11010100;
		      break;

		    default:
		     return; //something went wrong
		
		}//switch

		stepsCount++;
		
		switch (phase)
		{
			case UP:
				speed = speed + upRate;
				
				//exit
				if (speed >= maxSpeed)
				{
					phase = HI;
				}
				
				break;
			case DOWN:
				speed = speed - downRate;
				
				//exit
				if (speed <= minSpeed)
				{
					phase = LO;
				}
				
				break;	
			case HI:
				speed = maxSpeed;
				constCount++;
				
				//exit
				if (constCount >= constSteps)
				{
					phase = DOWN;
				}
				break;
			case LO:
				//low speed until steps are complete
				break;
		}
		
		//calculate delay
		delay = 10000/speed; //in 10ms
		dTimer(delay);
	}//while
	
	lastStep = i;
	lastPos = dir ? 
		((lastPos - stepsToRun + 200) % 200) : 
		(lastPos + stepsToRun) % 200;
}//rotate

void rotate(int stepsToRun, int dir) 
{
	//int stepsToRun = (int)deg/1.8; 
	int stepsCount = 0;
	int i = lastPos;

	while(stepsCount < stepsToRun)
	{	
		//increment or decrement i & handle overflow
		if (dir == 0) //cw
		{
			i = (i+1)%4;
		} else if (dir == 1) //ccw
		{
			i = (i == 0) ? 3 : i - 1;
		}
		
		switch (i)
		{
			//assuming PORTA bits mean this: (e1,l1,l2,e2,l3,l4, zero, zero)
			case 0:
			  PORTA = 0b11011000;
		      break;

		    case 1:
              PORTA = 0b10111000;
		      break;

		    case 2:
		      PORTA = 0b10110100;
		      break;

		    case 3:
		      PORTA = 0b11010100;
		      break;

		    default:
		     return; //something went wrong
		
		}//switch

		stepsCount++;
		
		//delay for coils to re magnetize
		mTimer(20);
	}//while
		
	lastStep = i;
	lastPos = dir ?
		((lastPos - stepsToRun + 200) % 200) :
		(lastPos + stepsToRun) % 200;
}//rotate


void rotateTrapLut(int stepsToRun, int dir, int inDelay, int outDelay)
{
	int stepsCount = 0;
	int i = lastStep;
	int accelIdx = 0;
	int decelIdx = 5;
	int accelEnd, decelBegin;
	
	
	// start at the appropriate speed
	accelIdx = 0;
	while(accel[accelIdx] > inDelay && accelIdx < accelLutSz - 1)
	{
		++accelIdx;
	}
	
	// end at the appropriate speed
	decelIdx = 5;
	while(decel[decelIdx] < outDelay && decelIdx > 0)
	{
		--decelIdx;
	}
	decelBegin = stepsToRun - accelIdx;

	// calculate phase transition indexes
	int accelSteps = (accelLutSz - accelIdx) + (decelIdx + 1);
	accelEnd = (stepsToRun > accelSteps) ? (accelLutSz - 1) : accelLutSz - (decelIdx + 1);
	decelBegin = stepsToRun - (decelIdx + 1);
	
	stepperPhase sPhase = UP;
	
	while (stepsCount < stepsToRun)
	{
		// step 
		step(&i,dir);
		stepsCount++;
		
		// perform delay
		int delay = accel[accelIdx];
		dTimer(delay);
		
		// update lutIdx and sPhase
		switch(sPhase)
		{
			case UP: 
				accelIdx++;
				break;
			case DOWN:
				accelIdx--;
				break;
		}
		
		if (stepsCount == decelBegin)
		{
			sPhase = DOWN;
		}
		else if (stepsCount == accelEnd)
		{
			sPhase = HI;
		}
		
		
	}
	
	lastStep = i;
	lastPos = dir ?
		((lastPos - stepsToRun + 200) % 200) :
		(lastPos + stepsToRun) % 200;
}

void basicAlign(cyl_t cyl_type)
{	
	int target;
	switch (cyl_type)
	{	
		case BLACK:
			target = 0;
			break;
			
		case ALUM:
			target = 50;
			break;
			
		case WHITE:
			target = 100;
			break;
			
		case STEEL:
			target = 150;
			break;
			
		case DISCARD: // something went wrong
			return;
	}
	int rotationCw = (target - lastPos + 200)%200;
	if (rotationCw <= 100) 
	{
		rotateTrapLut(rotationCw, 0, 9999, 9999); // fastest is CW
	}
	else
	{
		rotateTrapLut(200 - rotationCw, 1, 9999, 9999); // fastest is CCW
	}
}
