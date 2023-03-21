/* df1_general.h
** $Header: /home/cjm/cvs/frodospec/df1/include/df1_general.h,v 1.1 2023-03-21 14:34:52 cjm Exp $
*/

#ifndef DF1_GENERAL_H
#define DF1_GENERAL_H

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
#define DF1_ERROR_LENGTH (1024)

/* These constants should be the same as those in ngat.frodospec.df1.Df1Library.java */
/**
 * Value to pass into logging calls, used for all low level serial code logging.
 * The df1 library is allocated bits 16..23.
 * This constant should be the same as that defined in ngat.frodospec.df1.Df1Library.java.
 * @see #Df1_Log
 */
#define DF1_LOG_BIT_SERIAL	(1<<16)
/**
 * Value to pass into logging calls, used for all low level socket code logging.
 * The df1 library is allocated bits 16..23.
 * This constant should be the same as that defined in ngat.frodospec.df1.Df1Library.java.
 * @see #Df1_Log
 */
#define DF1_LOG_BIT_SOCKET	(1<<17)
/**
 * Value to pass into logging calls, used for all DF1 protocol logging.
 * The df1 library is allocated bits 16..23.
 * This constant should be the same as that defined in ngat.frodospec.df1.Df1Library.java.
 * @see #Df1_Log
 */
#define DF1_LOG_BIT_DF1	         (1<<18)
/**
 * Value to pass into logging calls, used for DF1 read/write routine logging.
 * The df1 library is allocated bits 16..23.
 * This constant should be the same as that defined in ngat.frodospec.df1.Df1Library.java.
 * @see #Df1_Log
 */
#define DF1_LOG_BIT_DF1_READ_WRITE (1<<19)

/**
 * Macro to check whether the parameter is either TRUE or FALSE.
 */
#define DF1_IS_BOOLEAN(value)	(((value) == TRUE)||((value) == FALSE))
/**
 * Macro to check whether the parameter is a sign character, i.e. either '+' or '-'.
 */
#define DF1_IS_SIGN(value)	(((value) == '+')||((value) == '-'))

/* external functions */
extern void Df1_Error(void);
extern void Df1_Error_To_String(char *error_string);
extern int Df1_Get_Error_Number(void);
extern void Df1_Get_Current_Time_String(char *time_string,int string_length);
extern void Df1_Log_Format(int level,char *format,...);
extern void Df1_Log(int level,char *string);
extern void Df1_Set_Log_Handler_Function(void (*log_fn)(int level,char *string));
extern void Df1_Set_Log_Filter_Function(int (*filter_fn)(int level,char *string));
extern void Df1_Log_Handler_Stdout(int level,char *string);
extern void Df1_Set_Log_Filter_Level(int level);
extern int Df1_Log_Filter_Level_Absolute(int level,char *string);
extern int Df1_Log_Filter_Level_Bitwise(int level,char *string);
extern char *Df1_Replace_String(char *string,char *find_string,char *replace_string);

/* external variables */
extern int Df1_Error_Number;
extern char Df1_Error_String[];

#endif
