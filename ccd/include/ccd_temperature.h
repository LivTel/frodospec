/* ccd_temperature.h -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_temperature.h,v 0.1 2000-01-25 15:03:32 cjm Exp $
*/
#ifndef CCD_TEMPERATURE_H
#define CCD_TEMPERATURE_H

extern int CCD_Temperature_Get(double *temperature);
extern int CCD_Temperature_Set(double target_temperature);
extern int CCD_Temperature_Get_Error_Number(void);
extern void CCD_Temperature_Error(void);
extern void CCD_Temperature_Error_String(char *error_string);

/*
** $Log: not supported by cvs2svn $
*/
#endif
