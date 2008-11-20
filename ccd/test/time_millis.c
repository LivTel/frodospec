/* time_millis.c 
** $Header: /home/cjm/cvs/frodospec/ccd/test/time_millis.c,v 1.3 2008-11-20 11:34:58 cjm Exp $
*/
/**
 * A little test program to test returning the current system time in milliseconds.
 * This uses POSIX.4.
 * Compile as follows:
 * <pre>
 * cc -I${JNIINCDIR} -I${JNIMDINCDIR} time_millis.c -o time_millis -lrt
 * </pre>
 * @author Chris Mottram
 * @version $Revision: 1.3 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for time.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <jni.h>

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: time_millis.c,v 1.3 2008-11-20 11:34:58 cjm Exp $";

/* external routines */
/**
 * Main program. Gets and displays the resolution of the POSIX.4 Realtime clock.
 */
int main(int argc,char *argv[])
{
	struct timespec current_time;
	jlong retval;

	clock_gettime(CLOCK_REALTIME,&(current_time));
	retval = ((jlong)current_time.tv_sec)*((jlong)1000L);
	retval += ((jlong)current_time.tv_nsec)/((jlong)1000000L);
	fprintf(stdout,"jlong milliseconds:%lld\n",retval);

	return 0;
}
