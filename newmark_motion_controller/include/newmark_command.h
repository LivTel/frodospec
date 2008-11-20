/* newmark_command.h
** Newmark Motion Controller library.
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/include/newmark_command.h,v 1.1 2008-11-20 11:35:52 cjm Exp $
*/
#ifndef NEWMARK_COMMAND_H
#define NEWMARK_COMMAND_H

#include "arcom_ess_interface.h"

extern int Newmark_Command_Move(Arcom_ESS_Interface_Handle_T *handle,double position);

extern int Newmark_Command_Home(Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Position_Get(Arcom_ESS_Interface_Handle_T *handle,double *position);
extern int Newmark_Command_Move_Absolute(Arcom_ESS_Interface_Handle_T *handle,double position);
extern int Newmark_Command_Move_Relative(Arcom_ESS_Interface_Handle_T *handle,double position_offset);
extern int Newmark_Command_Abort_Move(Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Err_Get(Arcom_ESS_Interface_Handle_T *handle,int *error_exists);
extern int Newmark_Command_Error_Get(Arcom_ESS_Interface_Handle_T *handle,int *error_code);
extern int Newmark_Command_Error_Reset(Arcom_ESS_Interface_Handle_T *handle);
extern int Newmark_Command_Position_Tolerance_Set(double mm);

#endif
/*
** $Log: not supported by cvs2svn $
*/
