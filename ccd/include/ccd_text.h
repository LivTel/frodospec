/*   
    Copyright 2006, Astrophysics Research Institute, Liverpool John Moores University.

    This file is part of Ccs.

    Ccs is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Ccs is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ccs; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/* ccd_text.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_text.h,v 0.4 2006-05-16 14:15:34 cjm Exp $
*/

#ifndef CCD_TEXT_H
#define CCD_TEXT_H

#include <sys/types.h>

/* These enum definitions should match with those in CCDLibrary.java */
/**
 * Level number passed to CCD_Text_Set_Print_Level to set the amount of information to print out. One of:
 * <ul>
 * <li>CCD_TEXT_PRINT_LEVEL_COMMANDS - Print out commands only.
 * <li>CCD_TEXT_PRINT_LEVEL_REPLIES - Print out commands' replies as well.
 * <li>CCD_TEXT_PRINT_LEVEL_VALUES - Print out parameter values as well.
 * <li>CCD_TEXT_PRINT_LEVEL_ALL - Print out everything.
 * </ul>
 * @see #CCD_Text_Set_Print_Level
 */
enum CCD_TEXT_PRINT_LEVEL
{
	CCD_TEXT_PRINT_LEVEL_COMMANDS=0,CCD_TEXT_PRINT_LEVEL_REPLIES=1,CCD_TEXT_PRINT_LEVEL_VALUES=2,
	CCD_TEXT_PRINT_LEVEL_ALL=3
};

/**
 * Macro to check whether the level is a legal level of print level.
 */
#define CCD_TEXT_IS_TEXT_PRINT_LEVEL(level)	(((level) == CCD_TEXT_PRINT_LEVEL_COMMANDS)|| \
	((level) == CCD_TEXT_PRINT_LEVEL_REPLIES)||((level) == CCD_TEXT_PRINT_LEVEL_VALUES)|| \
	((level) == CCD_TEXT_PRINT_LEVEL_ALL))

/* configuration of this device interface */
extern void CCD_Text_Set_Print_Level(enum CCD_TEXT_PRINT_LEVEL level);
extern void CCD_Text_Set_File_Pointer(FILE *fp);

/* implementation of device interface */
extern void CCD_Text_Initialise(void);
extern int CCD_Text_Open(void);
extern int CCD_Text_Memory_Map(int buffer_size);
extern int CCD_Text_Memory_UnMap(void);
extern int CCD_Text_Command(int request,int *argument);
extern int CCD_Text_Command_List(int request,int *argument_list,int argument_count);
extern int CCD_Text_Get_Reply_Data(unsigned short **data);
extern int CCD_Text_Close(void);
extern int CCD_Text_Get_Error_Number(void);
extern void CCD_Text_Error(void);
extern void CCD_Text_Error_String(char *error_string);
extern void CCD_Text_Warning(void);

#endif
