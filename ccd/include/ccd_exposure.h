/* ccd_exposure.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_exposure.h,v 0.1 2000-01-25 15:03:32 cjm Exp $
*/
#ifndef CCD_EXPOSURE_H
#define CCD_EXPOSURE_H

#include "ccd_global.h"

extern void CCD_Exposure_Initialise(void);
extern int CCD_Exposure_Expose(int open_shutter,int readout_ccd,int msecs,char *filename);
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
