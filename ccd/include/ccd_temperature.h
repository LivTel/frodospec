/* ccd_temperature.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_temperature.h,v 0.3 2002-11-07 19:16:51 cjm Exp $
*/
#ifndef CCD_TEMPERATURE_H
#define CCD_TEMPERATURE_H

extern int CCD_Temperature_Get(double *temperature);
extern int CCD_Temperature_Get_Utility_Board_ADU(int *adu);
extern int CCD_Temperature_Set(double target_temperature);
extern int CCD_Temperature_Get_Heater_ADU(int *heater_adu);
extern int CCD_Temperature_Get_Error_Number(void);
extern void CCD_Temperature_Error(void);
extern void CCD_Temperature_Error_String(char *error_string);

/*
** $Log: not supported by cvs2svn $
** Revision 0.2  2001/07/13 09:48:54  cjm
** Added CCD_Temperature_Get_Heater_ADU.
**
** Revision 0.1  2000/01/25 15:03:32  cjm
** initial revision (PCI version).
**
*/
#endif
