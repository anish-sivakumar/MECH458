/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include "linkedQueue.h"
#include <avr/io.h>

#define accelLutSz 24
#define decelLutSz 6
const int accel[accelLutSz]	= {1200, 1049, 945, 868, 807, 758, 717, 682, 652, 625, 602, 581, 562, 545, 529, 515, 501, 489, 478, 467, 457, 448, 439, 435};
const int decel[decelLutSz]	= {0, 4, 8, 10, 12, 14};

int savedDir = 0;
int willContinue = 0;
int skipAlign = 0;

typedef enum stepperPhase
{
	UP,
	DOWN,
	HI,
	LO
}stepperPhase;

void resetPosition()
{
	stepper.pos = 0;
}

void step()
{
	//increment or decrement i & handle overflow
	if (stepper.dir == 0) //cw
	{
		stepper.step = (stepper.step + 1) % 4;
		stepper.pos  = (stepper.pos + 1) % 200;
	} else if (stepper.dir == 1) //ccw
	{
		stepper.step = (stepper.step + 3) % 4;
		stepper.pos  = (stepper.pos + 199) % 200;

	}
	
	switch (stepper.step)
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

}

void stepperIntDisable()
{
	cli();
	TIMSK3  &= ~_BV(OCIE3A);  // disable stepTimer interrupt
	sei();
}

void stepperSetContinue(int continues, uint16_t delay)
{
	cli();
	if (continues)
	{
		stepper.continues = 1;
		stepper.delay = delay;
		if (delay < 6553)
		{
			OCR3A = 10 * delay;
		}
		else // requested delay is too large, set the slowest possible speed = 65ms
		{
			OCR3A = 0xff;
		}
		TCNT3 = 0;
		TIMSK3  |= _BV(OCIE3A); // enable stepTimer interrupt
	}
	else
	{
		stepper.continues = 0;
		stepper.delay = 0;
		TIMSK3  &= ~_BV(OCIE3A);  // disable stepTimer interrupt

	}
	sei();
}

void rotate(int stepsToRun, int dir) 
{
	int stepsCount = 0;
	stepper.dir = dir;
	
	while(stepsCount < stepsToRun)
	{	
		step();
		stepsCount++;
		mTimer(20);
	}
}//rotate


void rotateTrapLut(int stepsToRun, uint16_t outDelay)
{
	// disable async stepper control 
	stepperIntDisable();
	
	int stepsCount = 0;
	int accelIdx = 0;
	int decelIdx = 5;
	int accelEnd, decelBegin;
	
	
	// start at the appropriate speed
	accelIdx = 0;
	if(stepper.delay != 0)
	{
		while(accel[accelIdx] > stepper.delay && accelIdx < accelLutSz - 1)
		{
			++accelIdx;
		}
	}
	
	// end at the appropriate speed
	decelIdx = 5;
	if(outDelay != 0)
	{
		while(accel[decel[decelIdx]] < outDelay && decelIdx > 0)
		{
			--decelIdx;
		}
	}

	// calculate phase transition indexes
	int accelSteps = (accelLutSz - accelIdx) + (decelIdx + 1);
	accelEnd = (stepsToRun > accelSteps) ? (accelLutSz - 1) : accelLutSz - (decelIdx + 1);
	decelBegin = stepsToRun - (decelIdx + 1);
	
	stepperPhase sPhase = UP;
	
	while (stepsCount < stepsToRun)
	{
		// perform delay
		int delay = accel[accelIdx];
		dTimer(delay);
		
		// step 
		step();
		stepsCount++;
		
		// update lutIdx and sPhase
		switch(sPhase)
		{
			case UP: 
				accelIdx++;
				break;
			case DOWN:
				accelIdx = decel[decelIdx];
				decelIdx--;
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
	
	// update stepper values
	if (outDelay != 0)
	{
		stepperSetContinue(1,outDelay);
	}
	else
	{
		stepperSetContinue(0,0);
	}
	
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
	int rotationCw = (target - stepper.pos + 200)%200;
	if (rotationCw <= 100) 
	{
		stepper.dir = 0;
		rotateTrapLut(rotationCw, 0); // fastest is CW
	}
	else
	{	stepper.dir = 1;
		rotateTrapLut(200 - rotationCw, 0); // fastest is CCW
	}
	
	stepper.continues = 0;
}

ISR(TIMER3_COMPA_vect)
{
	if (stepper.continues)
	{
		step();
	}
	
	if (stepper.syncReq)
	{
		TIMSK3  &= ~_BV(OCIE3A);  // disable stepTimer interrupt
		stepper.syncReq = 0;
	}
}

//returns whether or not to wait for the drop to finish
int smartAlign(cyl_t firstCyl, link **h, link **t)
{
	int retVal = 0;
	int target;
	int secTarget;
	uint16_t exitSpeed;
	int rotationCw;
	int secRotationCw;
	int dir;
	int secDir; //add to stepper obj as nextDir
	uint16_t stepsToRun;
	cyl_t secCyl;
	int qSize = lqSize(h,t);
	cyl_t thirdCyl;

	if(skipAlign)
	{
		skipAlign = 0;
		retVal = 1; //set timer
	} 
	else 
	{
		// assign position to type
		switch (firstCyl)
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
			return retVal;
		}
	
		//determine first direction
		rotationCw = (target - stepper.pos + 200)%200;
		if (willContinue == 1)//priority
		{
			if (savedDir == 1)
			{
				dir = 1;
			} else
			{
				dir = 0;
			}
		} else // if not , calculate dir
		{
			if (rotationCw <= 100)
			{
				dir = 0;
			} else
			{
				dir = 1;
			}
		}
		
		//first steps to turn
		if (dir == 0)
		{
			stepsToRun = rotationCw;
		}
		else
		{
			stepsToRun = 200 - rotationCw;
		}
		
		
		if (qSize >= 1)
		{
			secCyl = (lqFirst(h)).itemCode;
			// assign position to type
			switch (secCyl)
			{
				case BLACK:
				secTarget = 0;
				break;
			
				case ALUM:
				secTarget = 50;
				break;
			
				case WHITE:
				secTarget = 100;
				break;
			
				case STEEL:
				secTarget = 150;
				break;
			
				case DISCARD: // something went wrong
				return retVal;
			}
		
			//determine second direction
			secRotationCw = (secTarget - target + 200)%200; // number to steps to second target
		
			if (secRotationCw > 110)
			{
				secDir = 1;
			} else if(secRotationCw < 90)
			{
				secDir = 0;			
			} else
			{
				secDir = dir;
			}
				
			// set delay for next cyl
			if (target == secTarget)
			{
				retVal = 1;
				skipAlign = 1;
				exitSpeed = 0;
				willContinue = 0;
			}
			else
			{
				retVal = 0;
				
				// set exit speed and position
				if (dir == secDir) 
				{
					willContinue = 1;
					savedDir = secDir;
					stepsToRun = stepsToRun - 25; // for border
		
					exitSpeed = 1200; //outDelay
			
				} else
				{
					exitSpeed = 0;
					willContinue = 0;
				}
			}
			
		
		} else //if empty queue
		{ 
			exitSpeed = 0;
			willContinue = 0;
		}
		
		stepper.dir = dir;
		rotateTrapLut(stepsToRun, exitSpeed);// i want the ISR to stop every time rotate() is called. 
	}
	
	return retVal;
}