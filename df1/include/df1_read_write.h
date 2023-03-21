/* df1_read_write.h
** FrodoSpec Micrologix 1100 PLC df1 library
** $Header: /home/cjm/cvs/frodospec/df1/include/df1_read_write.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/
#ifndef DF1_READ_WRITE_H
#define DF1_READ_WRITE_H
#include "df1.h"

extern int Df1_Write_Boolean(Df1_Interface_Handle_T *handle,int plctype, char *straddress, int value);
extern int Df1_Read_Boolean(Df1_Interface_Handle_T *handle,int plctype, char *straddress, int *value);
extern int Df1_Write_Integer(Df1_Interface_Handle_T *handle,int plctype, char *straddress,word value);
extern int Df1_Read_Integer(Df1_Interface_Handle_T *handle,int plctype, char *straddress,word *value);
extern int Df1_Write_Float(Df1_Interface_Handle_T *handle,int plctype, char *straddress,float value);
extern int Df1_Read_Float(Df1_Interface_Handle_T *handle,int plctype, char *straddress,float *value);

#endif
/*
** $Log: not supported by cvs2svn $
*/
