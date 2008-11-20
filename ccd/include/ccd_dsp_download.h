/* ccd_dsp_download.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_dsp_download.h,v 1.3 2008-11-20 11:34:52 cjm Exp $
*/
#ifndef CCD_DSP_DOWNLOAD_H
#define CCD_DSP_DOWNLOAD_H
#include "ccd_global.h"
#include "ccd_dsp.h"
#include "ccd_interface.h"

extern int CCD_DSP_Download_Initialise(void);
extern int CCD_DSP_Download(CCD_Interface_Handle_T* handle,enum CCD_DSP_BOARD_ID board_id,char *filename);
extern int CCD_DSP_Download_Get_Error_Number(void);
extern void CCD_DSP_Download_Error(void);
extern void CCD_DSP_Download_Error_String(char *error_string);

#endif
