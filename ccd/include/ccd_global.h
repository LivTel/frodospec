/* ccd_global.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_global.h,v 0.2 2000-04-13 12:57:23 cjm Exp $
*/

#ifndef CCD_GLOBAL_H
#define CCD_GLOBAL_H
#include "ccd_interface.h"

/**
 * TRUE is the value usually returned from routines to indicate success.
 */
#ifndef TRUE
#define TRUE 1
#endif
/**
 * FALSE is the value usually returned from routines to indicate failure.
 */
#ifndef FALSE
#define FALSE 0
#endif

/**
 * Macro to check whether the paramater is either TRUE or FALSE.
 */
#define CCD_GLOBAL_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))

/**
 * This is the length of error string of modules in the library.
 */
#define CCD_GLOBAL_ERROR_STRING_LENGTH	256
/**
 * This is the number of bytes used to represent one pixel on the CCD. Currently the SDSU CCD Controller
 * returns 16 bit values for pixels, which is 2 bytes.
 */
#define CCD_GLOBAL_BYTES_PER_PIXEL	2

extern void CCD_Global_Initialise(enum CCD_INTERFACE_DEVICE_ID interface_device);
extern void CCD_Global_Error(void);
extern void CCD_Global_Error_String(char *error_string);

/* routine used by other modules error code */
extern  void CCD_Global_Get_Current_Time_String(char *time_string,int string_length);
#endif
