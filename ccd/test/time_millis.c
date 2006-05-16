/*   
    Copyright 2006, Astrophysics Research Institute, Liverpool John Moores University.

    This file is part of Ccs.

    Ccs is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Ccs is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ccs; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/* time_millis.c -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/test/time_millis.c,v 1.2 2006-05-16 18:18:33 cjm Exp $
*/
/**
 * A little test program to test returning the current system time in milliseconds.
 * This uses POSIX.4.
 * Compile as follows:
 * <pre>
 * cc -I${JNIINCDIR} -I${JNIMDINCDIR} time_millis.c -o time_millis -lrt
 * </pre>
 * @author Chris Mottram
 * @version $Revision: 1.2 $
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
static char rcsid[] = "$Id: time_millis.c,v 1.2 2006-05-16 18:18:33 cjm Exp $";

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
