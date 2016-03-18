
// DragonBall Arena 2 has been written by:
//   Matt Brown (Antor), arkaine@sympatico.ca, 2000-2002
// Please follow all previous licenses. Enjoy!


#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

void do_varlimit (CHAR_DATA * ch, char *argument) {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf, "{cVar           Bits           Lower Limit           Upper Limit{x\n\r");
	sendch(buf,ch);
	sprintf(buf, "{y============  ====  ====================  ===================={x\n\r");
	sendch(buf,ch);

	sprintf(buf,  "char           %3d  %20d  %20d\n\r", sizeof(char) * CHAR_BIT, CHAR_MIN, CHAR_MAX);
	sendch(buf,ch); 
	sprintf(buf,  "short int      %3d  %20d  %20d\n\r", sizeof(short int) * CHAR_BIT, SHRT_MIN, SHRT_MAX);
	sendch(buf,ch);
	sprintf(buf,  "int            %3d  %20d  %20d\n\r", sizeof(int) * CHAR_BIT, INT_MIN, INT_MAX);
	sendch(buf,ch);
	sprintf(buf,  "long int       %3d  %20ld  %20ld\n\r", sizeof(long) * CHAR_BIT, LONG_MIN, LONG_MAX);
	sendch(buf,ch);
	sprintf(buf,  "long long int  %3d  %20Ld  %20Ld\n\r", sizeof(long long int) * CHAR_BIT, LONG_LONG_MIN, LONG_LONG_MAX);
	sendch(buf,ch);

	return;
}


