/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include "linkedQueue.h"
#include <avr/io.h>

typedef enum stepperPhase
{
	UP,
	UPROUND,
	DOWN,
	DOWNROUND,
	HI,
	LO
}stepperPhase;

/*takes in last step number, degrees we want to turn, and the direction (0 = cw and 1 = ccw)
and returns the last step we ran*/

void resetPosition()
{
	lastPos%=4;
}

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
	
	
	int speed = 90; // steps/sec = 1/delay
	
	//profile parameters
	int maxSpeed = 220; // 8.3ms delay
	int minSpeed = 80;
	int upRate = 10;
	int downRate = 30;
	int upRateRound = 0; // amount to decrease upRate by (the value is ++ to damp)
	int downRateRound = downRate - 5; //amount to decrease downRate by (the value is -- to accel)
	int cornerSpeed = maxSpeed - 40;
	
	stepperPhase phase = UP;
	
	int constSteps = stepsToRun - (maxSpeed-minSpeed)/upRate - (maxSpeed-minSpeed)/downRate - 6; //
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
				if (speed >= cornerSpeed)
				{
					phase = UPROUND;
				}
				
				break;
			
			case UPROUND:
				upRateRound = upRateRound + 1;
				
				if (upRate - upRateRound <= 3){ //so we don't decelerate to slowly
					upRateRound = upRate - 3;					
				}
				
				speed = speed + (upRate - upRateRound);
								
				//exit
				if (speed >= maxSpeed)
				{
					phase = HI;
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
			
			case DOWNROUND:
				downRateRound = downRateRound - 3;
								
				speed = speed - (downRate - downRateRound);
				
				//exit
				if (speed <= cornerSpeed) //until we hit downRate
				{ 
					phase = DOWN;
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
			
			case LO:
				//low speed until steps are complete
				break;
		}
		
		//calculate delay
		delay = 10000/speed; //in 10ms
		dTimer(delay);
	}//while
	
	lastStep = i;
	lastPos = dir ? ((lastPos - stepsToRun + 200) % 200) : (lastPos + stepsToRun) % 200;
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
		rotateTrapezoid(rotationCw, 0); // fastest is CW
	}
	else
	{
		rotateTrapezoid(200 - rotationCw, 1); // fastest is CCW
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
	int stepsToRun;
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
	
	//determine direction and steps to turn
	if (rotationCw <= 100)
	{
		stepsToTarget = rotationCw;
		stepsToRun = stepsToTarget;
		dir = 0;
		
		if(qSize >= 1)
		{
			secRotationCw = (secTarget - (lastPos + stepsToTarget)%200 + 200)%200; // number to steps to second target
		}
	}
	else
	{
		stepsToTarget = 200 - rotationCw;
		stepsToRun = stepsToTarget;
		dir = 1;
		
		if(qSize >= 1)
		{
			secRotationCw = (secTarget - (lastPos - stepsToTarget) + 200)%200; // number to steps to second target
		}
	}
	
	if(qSize >= 1)
	{
		if (secRotationCw <= 100)
		{
			secDir = 0;
		}
		else
		{
			secDir = 1;
		}
	}
	
	  
	LCDWriteIntXY(0,1,dir,1); //000
	LCDWriteIntXY(5,1,secDir,1); //682
		
	// determine arguments to rotate function
	if (dir == secDir && qSize >=1) 
	{
		
		if (dir == 0) // if going CW twice
		{
			stepsToRun = stepsToTarget; //-25
		} else 
		{
			stepsToRun = stepsToTarget; //+25
		}
		
		exitSpeed = 999; // PLACEHOLDER
		
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
	}
	
	//LCDWriteIntXY(0,1,exitSpeed,3); //000
	//LCDWriteIntXY(5,1,stepsToRun,3); //682
	
	
	rotate(stepsToRun, dir); // and exitSpeed
}

