/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include <avr/io.h>

/*takes in last step number, degrees we want to turn, and the direction (0 = cw and 1 = ccw)
and returns the last step we ran*/
int rotate(int lastState, int stepsToRun, int dir) 
{
	//int stepsToRun = (int)deg/1.8; 
	int stepsCount = 0;
	int i = lastState;

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
			  PORTA = 0b11000000;
		      break;

		    case 1:
              PORTA = 0b00011000;
		      break;

		    case 2:
		      PORTA = 0b10100000;
		      break;

		    case 3:
		      PORTA = 0b00010100;
		      break;

		    default:
		     return -1; //something went wrong
		
		}//switch

		stepsCount++;
		
		//delay 20ms for coils to re magnetize
		mTimer(20);
	}//while
	
	lastState = i;
	return lastState;

}//rotate

int rotate2(int lastState, int stepsToRun, int dir)
{
	//int stepsToRun = (int)deg/1.8;
	int stepsCount = 0;
	int i = lastState;

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
			return -1; //something went wrong
			
		}//switch

		stepsCount++;
		
		//delay 20ms for coils to re magnetize
		mTimer(20);
	}//while
	
	lastState = i;
	return lastState;

}//rotate