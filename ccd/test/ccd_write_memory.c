/* ccd_write_memory.c  -*- mode: Fundamental;-*-
 * $Header: /home/cjm/cvs/frodospec/ccd/test/ccd_write_memory.c,v 1.1 2001-01-18 10:51:26 cjm Exp $
 */
#include <stdio.h>
#include <time.h>
#include "ccd_dsp.h"
#include "ccd_interface.h"
#include "ccd_text.h"

/**
 * This program writes a memory location to the CCD camera.
 * <pre>
 * ccd_write_memory -board <board> -space <memory space> -address <address> -value <value>
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
static char rcsid[] = "$Id: ccd_write_memory.c,v 1.1 2001-01-18 10:51:26 cjm Exp $";
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_PCI;
/**
 * The board of the SDSU controller.
 */
static enum CCD_DSP_BOARD_ID Board = 0;
/**
 * Which part of DSP memory.
 */
static enum CCD_DSP_MEM_SPACE Memory_Space = 0;
/**
 * The address in memory.
 */
static int Memory_Address = 0;
/**
 * The to set the memory address to.
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
 */
int main(int argc, char *argv[])
{
	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	CCD_Text_Set_Print_Level(CCD_TEXT_PRINT_LEVEL_COMMANDS);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise(Interface_Device);

	fprintf(stdout,"Opening SDSU device.\n");
	CCD_Interface_Open();
	fprintf(stdout,"Writing %#x at address %d:%#x.\n",Value,Memory_Space,Memory_Address);
	if(CCD_DSP_Command_WRM(Board,Memory_Space,Memory_Address,Value) != CCD_DSP_DON)
	{
		CCD_DSP_Error();
		return 1;
	}
	fprintf(stdout,"CCD_Interface_Close\n");
	CCD_Interface_Close();
	fprintf(stdout,"CCD_Interface_Close completed.\n");
	return 0;
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
 * @see #Value
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
				sscanf(argv[i+1],"%i",&Memory_Address);
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
		else if(strcmp(argv[i],"-value")==0)
		{
			if((i+1)<argc)
			{
				sscanf(argv[i+1],"%i",&Value);
				i++;
			}
			else
				fprintf(stderr,"Parse_Arguments:"
					"Value requires an integer.\n");
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
	fprintf(stdout,"CCD Write Memory:Help.\n");
	fprintf(stdout,"CCD Write Memory writes a value to a controller memory location.\n");
	fprintf(stdout,"ccd_write_memory [-interface_device <interface device>][-help]\n");
	fprintf(stdout,"\t[-board <controller board>][-space <memory space>]\n");
	fprintf(stdout,"\t[-address <memory address>][-value <value>]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<interface device> can be either [pci|text].\n");
	fprintf(stdout,"\t<controller board> can be either [interface|timing|utility].\n");
	fprintf(stdout,"\t<memory space> can be either [x|y].\n");
	fprintf(stdout,"\t<memory address> is a positive integer, and can be decimal or hexidecimal(0x).\n");
	fprintf(stdout,"\t<value> is an integer, and can be decimal or hexidecimal (0x).\n");
}

/*
** $Log: not supported by cvs2svn $
**
*/
