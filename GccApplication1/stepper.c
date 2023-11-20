/* stepper driver */

#include "stepper.h" 
#include "utils.h"
#include "lcd.h"
#include <avr/io.h>

/*takes in last step number, degrees we want to turn, and the direction (0 = cw and 1 = ccw)
and returns the last step we ran*/

void resetPosition()
{
	last_state%=4;
}

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
		     return -1; //something went wrong
		
		}//switch

		stepsCount++;
		
		//delay 20ms for coils to re magnetize
		mTimer(12);
	}//while
	
	last_state = i;
}//rotate

void basic_align(cyl_t cyl_type)
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
	rotate((target - last_state + 200)%200, 0);
}
