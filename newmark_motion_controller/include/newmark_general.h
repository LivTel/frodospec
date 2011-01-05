/* newmark_general.h
** $Header: /home/cjm/cvs/frodospec/newmark_motion_controller/include/newmark_general.h,v 1.3 2011-01-05 14:15:50 cjm Exp $
*/

#ifndef NEWMARK_GENERAL_H
#define NEWMARK_GENERAL_H
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
 * How long the error string is.
 */
#define NEWMARK_ERROR_LENGTH (1024)

/**
 * Macro to check whether the parameter is either TRUE or FALSE.
 */
#define NEWMARK_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))

/* external functions */
extern void Newmark_Error(void);
extern void Newmark_Error_To_String(char *error_string);
extern int Newmark_Get_Error_Number(void);
extern void Newmark_Get_Current_Time_String(char *time_string,int string_length);
extern void Newmark_Log_Format(char *class,char *source,int level,char *format,...);
extern void Newmark_Log(char *class,char *source,int level,char *string);
extern void Newmark_Set_Log_Handler_Function(void (*log_fn)(char *class,char *source,int level,char *string));
extern void Newmark_Set_Log_Filter_Function(int (*filter_fn)(char *class,char *source,int level,char *string));
extern void Newmark_Log_Handler_Stdout(char *class,char *source,int level,char *string);
extern void Newmark_Set_Log_Filter_Level(int level);
extern int Newmark_Log_Filter_Level_Absolute(char *class,char *source,int level,char *string);
extern int Newmark_Log_Filter_Level_Bitwise(char *class,char *source,int level,char *string);
extern int Newmark_Add_To_String(char **string,char *add_string);

/* external variables */
extern int Newmark_Error_Number;
extern char Newmark_Error_String[];

#endif
/*
** $Log: not supported by cvs2svn $
** Revision 1.2  2009/02/05 11:41:12  cjm
** Swapped Bitwise for Absolute logging levels.
**
** Revision 1.1  2008/11/20 11:35:52  cjm
** Initial revision
**
*/

