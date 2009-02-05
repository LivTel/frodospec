/* ccd_global.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_global.h,v 0.9 2009-02-05 11:40:43 cjm Exp $
*/

#ifndef CCD_GLOBAL_H
#define CCD_GLOBAL_H
#include "ccd_interface.h"

/* hash defines */
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
 * Macro to check whether the parameter is either TRUE or FALSE.
 */
#define CCD_GLOBAL_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))

/**
 * This is the length of error string of modules in the library.
 */
#define CCD_GLOBAL_ERROR_STRING_LENGTH	256
/**
 * This is the number of bytes used to represent one pixel on the CCD. Currently the SDSU CCD Controller
 * returns 16 bit values for pixels, which is 2 bytes. The library will currently only compile when this
 * is two, as some parts assume 16 bit values.
 */
#define CCD_GLOBAL_BYTES_PER_PIXEL	2

/**
 * The number of nanoseconds in one second. A struct timespec has fields in nanoseconds.
 */
#define CCD_GLOBBAL_ONE_SECOND_NS	(1000000000)
/**
 * The number of nanoseconds in one millisecond. A struct timespec has fields in nanoseconds.
 */
#define CCD_GLOBAL_ONE_MILLISECOND_NS	(1000000)
/**
 * The number of milliseconds in one second.
 */
#define CCD_GLOBAL_ONE_SECOND_MS	(1000)
/**
 * The number of nanoseconds in one microsecond.
 */
#define CCD_GLOBAL_ONE_MICROSECOND_NS	(1000)

/* external functions */

extern void CCD_Global_Initialise(void);
extern void CCD_Global_Error(void);
extern void CCD_Global_Error_String(char *error_string);

/* routine used by other modules error code */
extern void CCD_Global_Get_Current_Time_String(char *time_string,int string_length);

/* logging routines */
extern void CCD_Global_Log_Format(int level,char *format,...);
extern void CCD_Global_Log(int level,char *string);
extern void CCD_Global_Set_Log_Handler_Function(void (*log_fn)(int level,char *string));
extern void CCD_Global_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string));
extern void CCD_Global_Log_Handler_Stdout(int level,char *string);
extern void CCD_Global_Set_Log_Filter_Level(int level);
extern int CCD_Global_Log_Filter_Level_Absolute(int level,char *string);
extern int CCD_Global_Log_Filter_Level_Bitwise(int level,char *string);

/* readout process priority and memory locking */
extern int CCD_Global_Increase_Priority(void);
extern int CCD_Global_Decrease_Priority(void);
extern int CCD_Global_Memory_Lock(unsigned short *image_data,int image_data_size);
extern int CCD_Global_Memory_UnLock(unsigned short *image_data,int image_data_size);
extern int CCD_Global_Memory_Lock_All(void);
extern int CCD_Global_Memory_UnLock_All(void);

#endif
