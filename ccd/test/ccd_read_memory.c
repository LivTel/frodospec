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
/* ccd_read_memory.c
 * $Header: /home/cjm/cvs/frodospec/ccd/test/ccd_read_memory.c,v 1.6 2006-11-06 16:52:49 eng Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ccd_dsp.h"
#include "ccd_interface.h"
#include "ccd_text.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"

/**
 * This program reads a memory location from the CCD camera.
 * <pre>
 * ccd_read_memory -b[oard] &lt;interface|timing|utility&gt; -s[pace] &lt;p|r|x|y&gt; -a[ddress] &lt;address&gt;
 * 	-i[nterface_device] &lt;pci|text&gt; -t[ext_print_level] &lt;commands|replies|values|all&gt; -h[elp]
 * </pre>
 * @author $Author: eng $
 * @version $Revision: 1.6 $
 */
/* hash definitions */
/**
 * Maximum length of some of the strings in this program.
 */
#define MAX_STRING_LENGTH	(256)

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: ccd_read_memory.c,v 1.6 2006-11-06 16:52:49 eng Exp $";
/**
 * How much information to print out when using the text interface.
 */
static enum CCD_TEXT_PRINT_LEVEL Text_Print_Level = CCD_TEXT_PRINT_LEVEL_ALL;
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_PCI;
/**
 * The board of the SDSU controller to query.
 */
static enum CCD_DSP_BOARD_ID Board = CCD_DSP_HOST_BOARD_ID;
/**
 * Which part of DSP memory to query.
 */
static enum CCD_DSP_MEM_SPACE Memory_Space = 0;
/**
 * The address in memory to query.
 */
static int Memory_Address = 0;

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive integer if it fails.
 * @see #Text_Print_Level
 * @see #Interface_Device
 * @see #Board
 * @see #Memory_Sapce
 * @see #Memory_Address
 */
int main(int argc, char *argv[])
{
	int retval;

	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_Text_Set_Print_Level(Text_Print_Level);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise(Interface_Device);

	fprintf(stdout,"Opening SDSU device.\n");
	fflush(stdout);
	retval = CCD_Interface_Open();
	if(retval == FALSE)
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"SDSU device opened.\n");
	fflush(stdout);

	fprintf(stdout,"Reading memory at address %#x.\n",Memory_Address);
	retval = CCD_DSP_Command_RDM(Board,Memory_Space,Memory_Address);
	if((retval == 0)&&(CCD_DSP_Get_Error_Number() != 0))
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"Result = %#x\n",retval);
	fprintf(stdout,"CCD_Interface_Close\n");
	CCD_Interface_Close();
	fprintf(stdout,"CCD_Interface_Close completed.\n");
	return retval;
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Text_Print_Level
 * @see #Interface_Device
 * @see #Board
 * @see #Memory_Space
 * @see #Memory_Address
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-address")==0)||(strcmp(argv[i],"-a")==0))
		{
			if((i+1)<argc)
			{
				sscanf(argv[i+1],"%i",&Memory_Address);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:"
					"Address requires a positive integer.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-board")==0)||(strcmp(argv[i],"-b")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"interface")==0)
					Board = CCD_DSP_INTERFACE_BOARD_ID;
				else if(strcmp(argv[i+1],"timing")==0)
					Board = CCD_DSP_TIM_BOARD_ID;
				else if(strcmp(argv[i+1],"utility")==0)
					Board = CCD_DSP_UTIL_BOARD_ID;
				else
				{
					fprintf(stderr,"Parse_Arguments:Board [interface|timing|utility].\n");
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Board [interface|timing|utility].\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-interface_device")==0)||(strcmp(argv[i],"-i")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"text")==0)
					Interface_Device = CCD_INTERFACE_DEVICE_TEXT;
				else if(strcmp(argv[i+1],"pci")==0)
					Interface_Device = CCD_INTERFACE_DEVICE_PCI;
				else
				{
					fprintf(stderr,"Parse_Arguments:Illegal Interface Device '%s'.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Interface Device requires a device.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-help")==0)||(strcmp(argv[i],"-h")==0))
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-space")==0)||(strcmp(argv[i],"-s")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"p")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_P;
				else if(strcmp(argv[i+1],"r")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_R;
				else if(strcmp(argv[i+1],"x")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_X;
				else if(strcmp(argv[i+1],"y")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_Y;
				else
				{
					fprintf(stderr,"Parse_Arguments:Memory Space [p|r|x|y].\n");
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Memory Space [p|r|x|y].\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-text_print_level")==0)||(strcmp(argv[i],"-t")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"commands")==0)
					Text_Print_Level = CCD_TEXT_PRINT_LEVEL_COMMANDS;
				else if(strcmp(argv[i+1],"replies")==0)
					Text_Print_Level = CCD_TEXT_PRINT_LEVEL_REPLIES;
				else if(strcmp(argv[i+1],"values")==0)
					Text_Print_Level = CCD_TEXT_PRINT_LEVEL_VALUES;
				else if(strcmp(argv[i+1],"all")==0)
					Text_Print_Level = CCD_TEXT_PRINT_LEVEL_ALL;
				else
				{
					fprintf(stderr,"Parse_Arguments:Illegal Text Print Level '%s'.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Text Print Level requires a level.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"CCD Read Memory:Help.\n");
	fprintf(stdout,"CCD Read Memory reads a controller memory location.\n");
	fprintf(stdout,"ccd_read_memory [-i[nterface_device] <interface device>]\n");
	fprintf(stdout,"\t[-b[oard] <controller board>][-s[pace] <memory space>]\n");
	fprintf(stdout,"\t[-a[ddress] <memory address>]\n");
	fprintf(stdout,"\t[-t[ext_print_level] <commands|replies|values|all>][-h[elp]]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<interface device> can be either [pci|text].\n");
	fprintf(stdout,"\t<controller board> can be either [interface|timing|utility].\n");
	fprintf(stdout,"\t<memory space> can be either [p|r|x|y].\n");
	fprintf(stdout,"\t<memory address> is a positive integer, either decimal or hexidecimal (0x).\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.5  2006/05/16 18:18:18  cjm
** gnuify: Added GNU General Public License.
**
** Revision 1.4  2002/11/07 19:18:22  cjm
** Changes to make library work with SDSU version 1.7 DSP code.
**
** Revision 1.3  2001/01/18 14:23:16  cjm
** Bugfix for testing for RDM failure.
**
** Revision 1.2  2001/01/17 18:11:44  cjm
** Changed address input so it can use an hex address.
**
** Revision 1.1  2001/01/17 18:04:48  cjm
** Initial revision
**
** Revision 1.0  2000/12/18 17:25:31  cjm
** Initial revision
**
*/
