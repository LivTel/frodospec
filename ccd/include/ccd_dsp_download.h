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
/* ccd_dsp_download.h
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_dsp_download.h,v 1.2 2006-05-16 14:15:26 cjm Exp $
*/
#ifndef CCD_DSP_DOWNLOAD_H
#define CCD_DSP_DOWNLOAD_H
#include "ccd_global.h"
#include "ccd_dsp.h"

extern int CCD_DSP_Download_Initialise(void);
extern int CCD_DSP_Download(enum CCD_DSP_BOARD_ID board_id,char *filename);
extern int CCD_DSP_Download_Get_Error_Number(void);
extern void CCD_DSP_Download_Error(void);
extern void CCD_DSP_Download_Error_String(char *error_string);

#endif
