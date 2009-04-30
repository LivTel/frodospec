/* ccd_dsp_private.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_dsp_private.h,v 1.1 2009-04-30 14:17:24 cjm Exp $
*/

#ifndef CCD_DSP_PRIVATE_H
#define CCD_DSP_PRIVATE_H

#include <pthread.h> /* pthread_mutex_t */

/**
 * Structure used to hold local per-handle data to ccd_dsp.
 * <dl>
 * <dt>Abort</dt> <dd>Whether it has been requested to abort the current operation.</dd>
 * <dt>Mutex</dt> <dd>Optionally compiled mutex locking for sending commands and getting replies from the 
 *    controller.</dd>
 * </dl>
 */
struct CCD_DSP_Struct
{
      volatile int Abort; /* This is volatile as a different thread may change this variable. */
#ifdef CCD_DSP_MUTEXED
      pthread_mutex_t Mutex;
#endif
};

#endif

/*
** $Log: not supported by cvs2svn $
*/
