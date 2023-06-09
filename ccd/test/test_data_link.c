/* test_data_link.c
 * $Header: /home/cjm/cvs/frodospec/ccd/test/test_data_link.c,v 1.6 2013-07-29 13:08:55 cjm Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ccd_dsp.h"
#include "ccd_global.h"
#include "ccd_interface.h"
#include "ccd_pci.h"
#include "ccd_text.h"

/**
 * This program tests the data link to a board in the SDSU controller.
 * <pre>
 * test_data_link -b[oard] &lt;interface|timing|utility&gt; -v[alue] &lt;test data value&gt;
 * 	-i[nterface_device] &lt;pci|text&gt; [-device_pathname </dev/astropciN>] 
 *      -t[ext_print_level] &lt;commands|replies|values|all&gt; -h[elp]
 * </pre>
 * @author $Author: cjm $
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
static char rcsid[] = "$Id: test_data_link.c,v 1.6 2013-07-29 13:08:55 cjm Exp $";
/**
 * How much information to print out when using the text interface.
 */
static enum CCD_TEXT_PRINT_LEVEL Text_Print_Level = CCD_TEXT_PRINT_LEVEL_ALL;
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_PCI;
/**
 * The pathname of the device to contact.
 */
static char Device_Pathname[MAX_STRING_LENGTH] = "";
/**
 * The board of the SDSU controller to query.
 */
static enum CCD_DSP_BOARD_ID Board = CCD_DSP_HOST_BOARD_ID;
/**
 * The test data link value.
 */
static int Value = 0;

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
 * @see #Device_Pathname
 * @see #Board
 * @see #Value
 */
int main(int argc, char *argv[])
{
	CCD_Interface_Handle_T *handle = NULL;
	int retval;

	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_Text_Set_Print_Level(Text_Print_Level);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise();

	fprintf(stdout,"Opening SDSU device.\n");
	fflush(stdout);
	if(strlen(Device_Pathname) == 0)
	{
		switch(Interface_Device)
		{
			case CCD_INTERFACE_DEVICE_PCI:
				strcpy(Device_Pathname,CCD_PCI_DEFAULT_DEVICE_ZERO);
				break;
			case CCD_INTERFACE_DEVICE_TEXT:
				strcpy(Device_Pathname,"frodospec_ccd_test_data_link.txt");
				break;
			default:
				fprintf(stderr,"Illegal interface device %d.\n",Interface_Device);
				break;
		}
	}
	retval = CCD_Interface_Open("test_data_link","-",Interface_Device,Device_Pathname,&handle);
	if(retval == FALSE)
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"SDSU device opened.\n");
	fflush(stdout);

	fprintf(stdout,"Testing data link to %d with  %#x.\n",Board,Value);
	retval = CCD_DSP_Command_TDL("test_data_link","-",handle,Board,Value);
	if((retval == 0)&&(CCD_DSP_Get_Error_Number() != 0))
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"Result = %#x\n",retval);
	fprintf(stdout,"CCD_Interface_Close\n");
	CCD_Interface_Close("test_data_link","-",&handle);
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
 * @see #Device_Pathname
 * @see #Board
 * @see #Value
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-board")==0)||(strcmp(argv[i],"-b")==0))
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
		else if((strcmp(argv[i],"-device_pathname")==0))
		{
			if((i+1)<argc)
			{
				strncpy(Device_Pathname,argv[i+1],MAX_STRING_LENGTH-1);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Device Pathname requires a device.\n");
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
		else if((strcmp(argv[i],"-value")==0)||(strcmp(argv[i],"-v")==0))
		{
			if((i+1)<argc)
			{
				sscanf(argv[i+1],"%i",&Value);
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:"
					"Value requires an integer.\n");
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
	fprintf(stdout,"Test Data Link:Help.\n");
	fprintf(stdout,"This program tests the data link to a board in the SDSU controller.\n");
	fprintf(stdout,"test_data_link [-i[nterface_device] <interface device>][-device_pathname </dev/astropciN>]\n");
	fprintf(stdout,"\t[-b[oard] <controller board>][-v[alue] <value>]\n");
	fprintf(stdout,"\t[-t[ext_print_level] <commands|replies|values|all>][-h[elp]]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<interface device> can be either [pci|text].\n");
	fprintf(stdout,"\t<controller board> can be either [interface|timing|utility].\n");
	fprintf(stdout,"\t<value> is a positive integer, either decimal or hexidecimal (0x).\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.5  2011/01/17 10:59:05  cjm
** API change to allow class and source information to be passed into the
** CCD library to enhance logging.
**
** Revision 1.4  2008/11/20 11:34:58  cjm
** *** empty log message ***
**
** Revision 1.3  2006/11/06 16:52:49  eng
** Added includes to fix implicit function declarations.
**
** Revision 1.2  2006/05/16 18:18:24  cjm
** gnuify: Added GNU General Public License.
**
** Revision 1.1  2002/11/07 19:18:22  cjm
** Initial revision
**
*/
