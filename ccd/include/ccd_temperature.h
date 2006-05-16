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
/* ccd_temperature.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_temperature.h,v 0.4 2006-05-16 14:15:33 cjm Exp $
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
