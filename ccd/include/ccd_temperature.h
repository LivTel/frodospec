/* ccd_temperature.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_temperature.h,v 0.6 2011-01-17 10:58:44 cjm Exp $
*/
#ifndef CCD_TEMPERATURE_H
#define CCD_TEMPERATURE_H
#include "ccd_interface.h"

extern int CCD_Temperature_Get(char *class,char *source,CCD_Interface_Handle_T* handle,double *temperature);
extern int CCD_Temperature_Get_Utility_Board_ADU(char *class,char *source,CCD_Interface_Handle_T* handle,int *adu);
extern int CCD_Temperature_Set(char *class,char *source,CCD_Interface_Handle_T* handle,double target_temperature);
extern int CCD_Temperature_Get_Heater_ADU(char *class,char *source,CCD_Interface_Handle_T* handle,int *heater_adu);
extern int CCD_Temperature_Get_Error_Number(void);
extern void CCD_Temperature_Error(void);
extern void CCD_Temperature_Error_String(char *error_string);

/*
** $Log: not supported by cvs2svn $
** Revision 0.5  2008/11/20 11:34:52  cjm
** *** empty log message ***
**
** Revision 0.4  2006/05/16 14:15:33  cjm
** gnuify: Added GNU General Public License.
**
** Revision 0.3  2002/11/07 19:16:51  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 0.2  2001/07/13 09:48:54  cjm
** Added CCD_Temperature_Get_Heater_ADU.
**
** Revision 0.1  2000/01/25 15:03:32  cjm
** initial revision (PCI version).
**
*/
#endif
