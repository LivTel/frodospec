/* ccd_exposure.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_exposure.h,v 0.3 2000-06-20 12:53:37 cjm Exp $
*/
#ifndef CCD_EXPOSURE_H
#define CCD_EXPOSURE_H
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
#include <time.h>
#include "ccd_global.h"

extern void CCD_Exposure_Initialise(void);
extern int CCD_Exposure_Expose(int open_shutter,struct timespec start_time,int exposure_time,char *filename);
extern int CCD_Exposure_Bias(char *filename);
extern int CCD_Exposure_Flush_CCD(void);
extern int CCD_Exposure_Open_Shutter(void);
extern int CCD_Exposure_Close_Shutter(void);
extern int CCD_Exposure_Pause(void);
extern int CCD_Exposure_Resume(void);
extern void CCD_Exposure_Abort(void);
extern void CCD_Exposure_Abort_Readout(void);
extern int CCD_Exposure_Read_Out_CCD(char *filename);
extern int CCD_Exposure_Get_Error_Number(void);
extern void CCD_Exposure_Error(void);
extern void CCD_Exposure_Error_String(char *error_string);
extern void CCD_Exposure_Warning(void);

#endif
