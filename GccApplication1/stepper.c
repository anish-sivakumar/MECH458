/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include <avr/io.h>

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
	last_state%=4;
}

void rotateTrapezoid(int stepsToRun, int dir) 
{
	int stepsCount = 0;
	int i = last_state;
	int delay;
	int speed = 45; //[30,100] steps/sec = 1/delay
	
	//profile parameters
	int maxSpeed = 160; // 8.3ms delay
	int minSpeed = 40;
	int upRate = 6;
	int downRate = 15;
	
	stepperPhase phase = UP;
	
	int constSteps = stepsToRun - 28; //28 based off maxSpeed = 160; minSpeed = 40;  upRate = 6;  downRate = 15;
	int constCount = 0;
	
	while(stepsCount < stepsToRun)
	{	
		//increment or decrement i & handle overflow
		if (dir == 0) //cw
		{
			i = (i+1)%200;
		} else if (dir == 1) //ccw
		{
			i = (i == 0) ? 199 : i - 1;
		}
		
		switch (i%4)
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
	
	last_state = i;
}//rotate

void rotate(int stepsToRun, int dir) 
{
	//int stepsToRun = (int)deg/1.8; 
	int stepsCount = 0;
	int i = last_state;

	while(stepsCount < stepsToRun)
	{	
		//increment or decrement i & handle overflow
		if (dir == 0) //cw
		{
			i = (i+1)%200;
		} else if (dir == 1) //ccw
		{
			i = (i == 0) ? 199 : i - 1;
		}
		
		switch (i%4)
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
		
		//delay 20ms for coils to re magnetize
		mTimer(20);
	}//while
	
	last_state = i;
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
	int rotationCw = (target - last_state + 200)%200;
	if (rotationCw <= 100) 
	{
		rotate(rotationCw, 0); // fastest is CW
	}
	else
	{
		rotate(200 - rotationCw, 1); // fastest is CCW
	}
}
