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
/* posix_time.c -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/test/posix_time.c,v 1.2 2006-05-16 18:18:21 cjm Exp $
*/
/**
 * A little test program to prototype getting the current system time to as high a resolution as possible.
 * This uses POSIX.4.
 * Compile as follows:
 * <pre>
 * cc posix_time.c -o posix_time -lrt
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
#include <errno.h>
#include <time.h>

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: posix_time.c,v 1.2 2006-05-16 18:18:21 cjm Exp $";

/* external routines */
/**
 * Main program. Gets and displays the resolution of the POSIX.4 Realtime clock.
 */
int main(int argc,char *argv[])
{
	struct timespec current_time[10];
	struct timespec realtime_res;
	struct timespec sleep_time;
	char buff[256];
	int retval,error_number,milliseconds,i;

	retval = clock_getres(CLOCK_REALTIME,&realtime_res);
	if(retval < 0)
	{
		error_number = errno;
		fprintf(stderr,"posix_time:get resolution failed:%d\n",error_number);
		return 1;
	}
	fprintf(stdout,"clock resolution:%d seconds:%d nanoseconds = %.3f milliseconds.\n",realtime_res.tv_sec,
		realtime_res.tv_nsec,(((double)realtime_res.tv_nsec)/1000000.0));
	for(i=0;i<10;i++)
	{
		clock_gettime(CLOCK_REALTIME,&(current_time[i]));
	/* sleep for  */
		sleep_time.tv_sec = 0;
		/*sleep_time.tv_nsec = 1000000;*/ /* 1 millisecond */
		/*sleep_time.tv_nsec = 10000000;*/ /* 100th second */
		sleep_time.tv_nsec = 1; /* 1 nanosecond */
		nanosleep(&sleep_time,NULL);
	}
	for(i=0;i<10;i++)
	{
		fprintf(stdout,"real time[%d]:%d seconds:%d nanoseconds.\n",i,current_time[i].tv_sec,
			current_time[i].tv_nsec);
		cftime(buff,"%Y-%m-%dT%H:%M:%S.",&current_time[i].tv_sec);
		milliseconds = (((double)current_time[i].tv_nsec)/1000000.0);
		fprintf(stdout,"time[%d]:%s%03d\n",i,buff,milliseconds);
	}
	return 0;
}
/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2001/02/01 11:16:25  cjm
** Initial revision
**
*/
