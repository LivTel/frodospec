/* test_abort.c
 * $Header: /home/cjm/cvs/frodospec/ccd/test/test_abort.c,v 1.5 2011-01-17 10:59:05 cjm Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ccd_dsp.h"
#include "ccd_dsp_download.h"
#include "ccd_interface.h"
#include "ccd_pci.h"
#include "ccd_text.h"
#include "ccd_temperature.h"
#include "ccd_setup.h"

/**
 * This program tests the aborting of exposures, eiher whilst exposing or reading out. We get the HSTR
 * register to determine whether the controller is currently reading out, the Exposure Status in libccd
 * is no good as the exposure is running in a separate executable in this case.
 * <pre>
 * test_abort -i[nterface_device] &lt;pci|text&gt; 
 * 	-t[ext_print_level] &lt;commands|replies|values|all&gt; -help
 * </pre>
 * @author $Author: cjm $
 * @version $Revision: 1.5 $
 */
/* hash definitions */
/**
 * Maximum length of some of the strings in this program.
 */
#define MAX_STRING_LENGTH	(256)
/**
 * Bits used when getting the HSTR status.
 * Copied from ccd_exposure.c
 */
#define EXPOSURE_HSTR_HTF_BITS				(0x38)
/**
 * When bits 3 and 5 in the HSTR are set, the SDSU controllers are reading out.
 * Copied from ccd_exposure.c
 */
#define EXPOSURE_HSTR_READOUT				(0x5)

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: test_abort.c,v 1.5 2011-01-17 10:59:05 cjm Exp $";
/**
 * How much information to print out when using the text interface.
 */
static enum CCD_TEXT_PRINT_LEVEL Text_Print_Level = CCD_TEXT_PRINT_LEVEL_ALL;
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_NONE;

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
 * @see #EXPOSURE_HSTR_HTF_BITS
 * @see #EXPOSURE_HSTR_READOUT
 */
int main(int argc, char *argv[])
{
	CCD_Interface_Handle_T *handle = NULL;
	char device_pathname[256];
	int retval,status;

	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
	/* initialise */
	CCD_Text_Set_Print_Level(Text_Print_Level);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise();
	/* open interface */
	fprintf(stdout,"Opening SDSU device.\n");
	switch(Interface_Device)
	{
		case CCD_INTERFACE_DEVICE_PCI:
			strcpy(device_pathname,CCD_PCI_DEFAULT_DEVICE_ZERO);
			break;
		case CCD_INTERFACE_DEVICE_TEXT:
			strcpy(device_pathname,"frodospec_ccd_test_abort.txt");
			break;
		default:
			fprintf(stderr,"Illegal interface device %d.\n",Interface_Device);
			break;
	}
	retval = CCD_Interface_Open("test_sbort","-",Interface_Device,device_pathname,&handle);
	if(retval == FALSE)
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"SDSU device opened.\n");
	fflush(stdout);
	/* get Host Status */
	fprintf(stdout,"Getting HSTR.\n");
	if(!CCD_DSP_Command_Get_HSTR("test_sbort","-",handle,&status))
	{
		CCD_Global_Error();
		return 1;
	}
	fprintf(stdout,"HSTR is %#x.\n",status);
	status = (status & EXPOSURE_HSTR_HTF_BITS) >> 3;
	fprintf(stdout,"HSTR is now %#x, READOUT is %#x.\n",status,EXPOSURE_HSTR_READOUT);
	/* If we are not reading out... */
	if(status != EXPOSURE_HSTR_READOUT)
	{
		/* abort a running exposure */
		fprintf(stdout,"About to Abort Exposure.\n");
		retval = CCD_DSP_Command_AEX("test_sbort","-",handle);
		if(retval != CCD_DSP_DON)
		{
			CCD_Global_Error();
			return 1;
		}
		fprintf(stdout,"Abort Exposure Successfully Completed.\n");
	}
	else
	{
		/* abort a readout */
		fprintf(stdout,"About to Abort Readout.\n");
		retval = CCD_DSP_Command_ABR("test_sbort","-",handle);
		if(retval != CCD_DSP_DON)
		{
			CCD_Global_Error();
			return 1;
		}
		fprintf(stdout,"Abort Readout Successfully Completed.\n");
	}
	fprintf(stdout,"CCD_Interface_Close\n");
	CCD_Interface_Close("test_sbort","-",&handle);
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
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-interface_device")==0)||(strcmp(argv[i],"-i")==0))
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
	fprintf(stdout,"Test Abort:Help.\n");
	fprintf(stdout,"This program aborts an exposure or readout currently underway on the SDSU controller board.\n");
	fprintf(stdout,"test_abort [-i[nterface_device] <pci|text>]\n");
	fprintf(stdout,"\t[-t[ext_print_level] <commands|replies|values|all>][-h[elp]]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-text_print_level selects the amaount of data to print out from the text device.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.4  2008/11/20 11:34:58  cjm
** *** empty log message ***
**
** Revision 1.3  2006/11/06 16:52:49  eng
** Added includes to fix implicit function declarations.
**
** Revision 1.2  2006/05/16 18:18:21  cjm
** gnuify: Added GNU General Public License.
**
** Revision 1.1  2002/11/07 19:18:22  cjm
** Initial revision
**
*/
