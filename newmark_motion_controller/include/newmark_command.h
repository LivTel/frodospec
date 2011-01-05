/* newmark_command.h
** Newmark Motion Controller library.
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/include/newmark_command.h,v 1.2 2011-01-05 14:15:50 cjm Exp $
*/
#ifndef NEWMARK_COMMAND_H
#define NEWMARK_COMMAND_H

#include "arcom_ess_interface.h"

extern int Newmark_Command_Move(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,double position);

extern int Newmark_Command_Home(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Position_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,
					double *position);
extern int Newmark_Command_Move_Absolute(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,
					 double position);
extern int Newmark_Command_Move_Relative(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,
					 double position_offset);
extern int Newmark_Command_Abort_Move(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Err_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,int *error_exists);
extern int Newmark_Command_Error_Get(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle,int *error_code);
extern int Newmark_Command_Error_Reset(char *class,char *source,Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Position_Tolerance_Set(char *class,char *source,double mm);

#endif
/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2008/11/20 11:35:52  cjm
** Initial revision
**
*/
