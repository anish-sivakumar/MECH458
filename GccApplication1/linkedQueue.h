#ifndef LINKED_QUEUE_H_
#define LINKED_QUEUE_H_


/* Type definitions */
typedef struct {
	char itemCode; 	/* stores a number describing the element */
} element;

typedef struct link{
	element		e;
	struct link *next;
} link;

/* Constant declations */
/* 	E.g. #define STAGE0 0 ... sets STAGE0 = 0 

	Constant declarations can make you code much more readable, and will make life easier when required
	you need to enter in a constant value through many places in your code such as PI, or the GOLDEN NUMBER
	etc.
*/


/* Subroutine headers */
/* 	List the top line of your subroutine here. 
	WARNING: Make sure you put a semi-colon after each line, if you fail to do this it will make your life
	miserable to try and figure out where your bug is
*/
//MAKE sure there are semi colons at the end of these if you change them!!!

void	initLink	(link **newLink);
void 	lq_setup	(link **h, link **t);
void 	lq_clear	(link **h, link **t);
void 	lq_push		(link **h, link **t, link **nL);
void 	lq_pop		(link **h, link **poppedLink);
element lq_first	(link **h);
char 	lq_isEmpty		(link **h);
int 	lq_size		(link **h, link **t);

#endif // LINKED_QUEUE_H_

