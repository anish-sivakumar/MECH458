/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include "linkedQueue.h"
#include <avr/io.h>

#define accelLutSz 24
#define decelLutSz 6
const int accel[accelLutSz]	= {1200, 1049, 945, 868, 807, 758, 717, 682, 652, 625, 602, 581, 562, 545, 529, 515, 501, 489, 478, 467, 457, 448, 439, 435};
const int decel[decelLutSz]	= {1200, 807, 652, 602, 562, 529};

int savedDir = 0;
int willContinue = 0;

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
		PORTL = 0x10101010;

	}
	sei();
}


//// returns the out
//void rotateTrapezoid(int stepsToRun, int dir) 
//{
	//int stepsCount = 0;
	//int i = lastStep;
	//int delay;
	//
	///*
	//int speed = 45; //steps/sec = 1/delay
	//
	////profile parameters
	//int maxSpeed = 160; // 8.3ms delay
	//int minSpeed = 40;
	//int upRate = 6;
	//int downRate = 15;
	//*/
	//
	//
	//int speed = 90; //steps/sec = 1/delay
	//
	////profile parameters
	//int maxSpeed = 280; // 8.3ms delay
	//int minSpeed = 80;
	//int upRate = 10;
	//int downRate = 30;
	//
	//
	//stepperPhase phase = UP;
	//
	//int constSteps = stepsToRun - (maxSpeed-minSpeed)/upRate - (maxSpeed-minSpeed)/downRate; //28 based off maxSpeed = 160; minSpeed = 40;  upRate = 6;  downRate = 15;
	//int constCount = 0;
	//
	//while(stepsCount < stepsToRun)
	//{	
		////increment or decrement i & handle overflow
		//if (dir == 0) //cw
		//{
			//i = (i+1)%4;
		//} else if (dir == 1) //ccw
		//{
			//i = (i == 0) ? 3 : i - 1;
		//}
		//
		//switch (i)
		//{
			////assuming PORTA bits mean this: (e1,l1,l2,e2,l3,l4, zero, zero)
			//case 0:
			  //PORTA = 0b11011000;
		      //break;
//
		    //case 1:
              //PORTA = 0b10111000;
		      //break;
//
		    //case 2:
		      //PORTA = 0b10110100;
		      //break;
//
		    //case 3:
		      //PORTA = 0b11010100;
		      //break;
//
		    //default:
		     //return; //something went wrong
		//
		//}//switch
//
		//stepsCount++;
		//
		//switch (phase)
		//{
			//case UP:
				//speed = speed + upRate;
				//
				////exit
				//if (speed >= maxSpeed)
				//{
					//phase = HI;
				//}
				//
				//break;
			//case DOWN:
				//speed = speed - downRate;
				//
				////exit
				//if (speed <= minSpeed)
				//{
					//phase = LO;
				//}
				//
				//break;	
			//case HI:
				//speed = maxSpeed;
				//constCount++;
				//
				////exit
				//if (constCount >= constSteps)
				//{
					//phase = DOWN;
				//}
				//break;
			//case LO:
				////low speed until steps are complete
				//break;
		//}
		//
		////calculate delay
		//delay = 10000/speed; //in 10ms
		//dTimer(delay);
	//}//while
	//
	//lastStep = i;
	//lastPos = dir ? 
		//((lastPos - stepsToRun + 200) % 200) : 
		//(lastPos + stepsToRun) % 200;
//}//rotate

void rotate(int stepsToRun, int dir) 
{
	int stepsCount = 0;
	stepper.dir = dir;
	
	while(stepsCount < stepsToRun)
	{	
		step();
		stepsCount++;
		mTimer(20);
	}//while
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
		while(decel[decelIdx] < outDelay && decelIdx > 0)
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
		// step 
		step();
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
	PORTL = 0xff;

	if (stepper.continues)
	{
		step();
	}
}

void smartAlign(cyl_t firstCyl, link **h, link **t)
{
	int target;
	int secTarget;
	int exitSpeed;
	int exitPos;
	int prevDir;
	int rotationCw;
	int secRotationCw;
	int dir;
	int secDir;
	int stepsToTarget;
	uint16_t stepsToRun;
	cyl_t secCyl;
		
	//cyl_t thirdCyl = (*h)->next->e.itemCode;
	
	
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
		return;
	}
	
	int qSize = lqSize(h,t);
	if (qSize >= 1)
	{
		
		secCyl = (*h)->e.itemCode;
		
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
			return;
		}
		
	}
	
	rotationCw = (target - lastPos + 200)%200;
	
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
	
	//determine direction and steps to turn
	if (dir == 0)
	{
		stepsToTarget = rotationCw;
		stepsToRun = stepsToTarget;
		
		if(qSize >= 1)
		{
			secRotationCw = (secTarget - (lastPos + stepsToTarget)%200 + 200)%200; // number to steps to second target
		}
	}
	else
	{
		
		stepsToTarget = 200 - rotationCw;
		stepsToRun = stepsToTarget;
		
		if(qSize >= 1)
		{
			secRotationCw = (secTarget - (lastPos - stepsToTarget) + 200)%200; // number to steps to second target
		}
	}
	
	//determine second direction
	if(qSize >= 1)
	{
		if (secRotationCw <= 110 && dir == 0)
		{
			secDir = 0;
		}
		else if (secRotationCw >= 90 && dir == 1)
		{
			secDir = 1;
		}
	}
	
	// determine steps to run and exit speed
	if (dir == secDir && qSize >=1) 
	{
		willContinue = 1;
		savedDir = secDir;
		if (dir == 0) // if going CW twice
		{
			stepsToRun = stepsToTarget; //-25
		} else 
		{
			stepsToRun = stepsToTarget; //+25
		}
		
		exitSpeed = 999; // PLACEHOLDER
		
		//propagate direction
		
		
		/*
		// determine exit speed
		if (thirdCyl == secCyl) // if the second and third objects are the same, slow exitSpeed
		{
			exitSpeed = 555; // PLACEHOLDER
		} else { 
			exitSpeed = 999; // PLACEHOLDER
		}
		*/
		
	} else
	{
		exitSpeed = 0;
		willContinue = 0;
	}
	
	LCDWriteIntXY(0,1,dir,1); //000
	LCDWriteIntXY(3,1,secDir,1); //682
	LCDWriteIntXY(5,1,stepsToRun,3); //682
	LCDWriteIntXY(9,1,willContinue,1); //000
	LCDWriteIntXY(12,1,savedDir,1); //682
	dTimer(30000);
	
	//LCDWriteIntXY(0,1,exitSpeed,3); //000
	//LCDWriteIntXY(5,1,stepsToRun,3); //682
	//dTimer(30000);
	
	rotate(stepsToRun, dir); // and exitSpeed
	//rotate(100,1);
}

