/* ccd_read_memory.c  -*- mode: Fundamental;-*-
 * $Header: /home/cjm/cvs/frodospec/ccd/test/ccd_read_memory.c,v 1.1 2001-01-17 18:04:48 cjm Exp $
 */
#include <stdio.h>
#include <time.h>
#include "ccd_dsp.h"
#include "ccd_interface.h"
#include "ccd_text.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"

/**
 * This program reads a memory location from the CCD camera.
 * <pre>
 * ccd_read_memory -board <board> -space <memory space> -address <address>
 * </pre>
 * @author $Author: cjm $
 * @version $Revision: 1.1 $
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
static char rcsid[] = "$Id: ccd_read_memory.c,v 1.1 2001-01-17 18:04:48 cjm Exp $";
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_PCI;
/**
 * The board of the SDSU controller to query.
 */
static enum CCD_DSP_BOARD_ID Board = CCD_DSP_UTIL_BOARD_ID;
/**
 * Which part of DSP memory to query.
 */
static enum CCD_DSP_MEM_SPACE Memory_Space = CCD_DSP_MEM_SPACE_Y;
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
 */
int main(int argc, char *argv[])
{
	int retval;

	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_Text_Set_Print_Level(CCD_TEXT_PRINT_LEVEL_COMMANDS);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise(Interface_Device);

	fprintf(stdout,"Opening SDSU device.\n");
	CCD_Interface_Open();
	fprintf(stdout,"Reading memory.\n");
	retval = CCD_DSP_Command_RDM(Board,Memory_Space,Memory_Address);
	if((retval == 0)&&(CCD_DSP_Get_Error_Number != 0))
	{
		CCD_DSP_Error();
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
		if(strcmp(argv[i],"-address")==0)
		{
			if((i+1)<argc)
			{
				Memory_Address = atoi(argv[i+1]);
				i++;
			}
			else
				fprintf(stderr,"Parse_Arguments:"
					"Address requires a positive integer.\n");
		}
		else if(strcmp(argv[i],"-board")==0)
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
					fprintf(stderr,"Parse_Arguments:Board [interface|timing|utility].\n");
				i++;
			}
			else
				fprintf(stderr,"Parse_Arguments:Board [interface|timing|utility].\n");
		}
		else if(strcmp(argv[i],"-interface_device")==0)
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"text")==0)
					Interface_Device = CCD_INTERFACE_DEVICE_TEXT;
				else if(strcmp(argv[i+1],"pci")==0)
					Interface_Device = CCD_INTERFACE_DEVICE_PCI;
				else
					fprintf(stderr,"Parse_Arguments:Illegal Interface Device '%s'.\n",argv[i+1]);
				i++;
			}
			else
				fprintf(stderr,"Parse_Arguments:Interface Device requires a device.\n");
		}
		else if(strcmp(argv[i],"-help")==0)
		{
			Help();
			exit(0);
		}
		else if(strcmp(argv[i],"-space")==0)
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"x")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_X;
				else if(strcmp(argv[i+1],"y")==0)
					Memory_Space = CCD_DSP_MEM_SPACE_Y;
				else
					fprintf(stderr,"Parse_Arguments:Memory Space [x|y].\n");
				i++;
			}
			else
				fprintf(stderr,"Parse_Arguments:Memory Space [x|y].\n");
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
	fprintf(stdout,"ccd_read_memory [-interface_device <interface device>][-help]\n");
	fprintf(stdout,"\t[-board <controller board>][-space <memory space>]\n");
	fprintf(stdout,"\t[-address <memory address>]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<interface device> can be either [pci|text].\n");
	fprintf(stdout,"\t<controller board> can be either [interface|timing|utility].\n");
	fprintf(stdout,"\t<memory space> can be either [x|y].\n");
	fprintf(stdout,"\t<memory address> can be a positive integer.\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.0  2000/12/18 17:25:31  cjm
** Initial revision
**
*/
