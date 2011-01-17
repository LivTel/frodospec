/* test_setup_dimensions.c
 * $Header: /home/cjm/cvs/frodospec/ccd/test/test_setup_dimensions.c,v 1.6 2011-01-17 10:59:05 cjm Exp $
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
 * This program tests CCD_Setup_Dimensions, which does dimension configuration of the SDSU controller.
 * <pre>
 * test_setup_dimensions [-i[nterface_device] &lt;pci|text&gt;]
 * 	[-xs[ize] &lt;no. of pixels&gt;][-ys[ize] &lt;no. of pixels&gt;]
 * 	[-xb[in] &lt;binning factor&gt;][-yb[in] &lt;binning factor&gt;]
 * 	[-a[mplifier] &lt;left|right|both&gt;]
 * 	[-w[indow] &lt;no&gt; &lt;xstart&gt; &lt;ystart&gt; &lt;xend&gt; &lt;yend&gt;]
 * 	[-t[ext_print_level] &lt;commands|replies|values|all&gt;][-h[elp]]
 * </pre>
 * @author $Author: cjm $
 * @version $Revision: 1.6 $
 */
/* hash definitions */
/**
 * Maximum length of some of the strings in this program.
 */
#define MAX_STRING_LENGTH	(256)
/**
 * Default number of columns in the CCD.
 */
#define DEFAULT_SIZE_X		(2200)
/**
 * Default number of rows in the CCD.
 */
#define DEFAULT_SIZE_Y		(2048)
/**
 * Default amplifier.
 */
#define DEFAULT_AMPLIFIER	(CCD_DSP_AMPLIFIER_BOTH)
/**
 * Default de-interlace type.
 */
#define DEFAULT_DEINTERLACE_TYPE (CCD_DSP_DEINTERLACE_SPLIT_SERIAL)

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: test_setup_dimensions.c,v 1.6 2011-01-17 10:59:05 cjm Exp $";
/**
 * How much information to print out when using the text interface.
 */
static enum CCD_TEXT_PRINT_LEVEL Text_Print_Level = CCD_TEXT_PRINT_LEVEL_ALL;
/**
 * Which interface to communicate with the SDSU controller with.
 */
static enum CCD_INTERFACE_DEVICE_ID Interface_Device = CCD_INTERFACE_DEVICE_NONE;
/**
 * The number of columns in the CCD.
 * @see #DEFAULT_SIZE_X
 */
static int Size_X = DEFAULT_SIZE_X;
/**
 * The number of rows in the CCD.
 * @see #DEFAULT_SIZE_Y
 */
static int Size_Y = DEFAULT_SIZE_Y;
/**
 * The number binning factor in columns.
 */
static int Bin_X = 1;
/**
 * The number binning factor in rows.
 */
static int Bin_Y = 1;
/**
 * The de-interlace type to use.
 */
static enum CCD_DSP_DEINTERLACE_TYPE DeInterlace_Type = DEFAULT_DEINTERLACE_TYPE;
/**
 * The amplifier to use.
 */
static enum CCD_DSP_AMPLIFIER Amplifier = DEFAULT_AMPLIFIER;
/**
 * Window flags specifying which window to use.
 */
static int Window_Flags = 0;
/**
 * Window data.
 */
static struct CCD_Setup_Window_Struct Window_List[CCD_SETUP_WINDOW_COUNT];

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
 * @see #Size_X
 * @see #Size_Y
 * @see #Bin_X
 * @see #Bin_Y
 * @see #Amplifier
 * @see #DeInterlace_Type
 * @see #Window_Flags
 * @see #Window_List
 */
int main(int argc, char *argv[])
{
	CCD_Interface_Handle_T *handle = NULL;
	char device_pathname[256];
	int retval;
	int value,bit_value;

/* parse arguments */
	fprintf(stdout,"Parsing Arguments.\n");
	if(!Parse_Arguments(argc,argv))
		return 1;
/* set text/interface options */
	CCD_Text_Set_Print_Level(Text_Print_Level);
	fprintf(stdout,"Initialise Controller:Using device %d.\n",Interface_Device);
	CCD_Global_Initialise();
	CCD_Global_Set_Log_Handler_Function(CCD_Global_Log_Handler_Stdout);
/* open SDSU connection */
	fprintf(stdout,"Opening SDSU device.\n");
	switch(Interface_Device)
	{
		case CCD_INTERFACE_DEVICE_PCI:
			strcpy(device_pathname,CCD_PCI_DEFAULT_DEVICE_ZERO);
			break;
		case CCD_INTERFACE_DEVICE_TEXT:
			strcpy(device_pathname,"frodospec_ccd_test_setup_dimensions.txt");
			break;
		default:
			fprintf(stderr,"Illegal interface device %d.\n",Interface_Device);
			break;
	}
	retval = CCD_Interface_Open("test_setup_dimensions",NULL,Interface_Device,device_pathname,&handle);
	if(retval == FALSE)
	{
		CCD_Global_Error();
		return 2;
	}
	fprintf(stdout,"SDSU device opened.\n");
/* call CCD_Setup_Dimensions */
	fprintf(stdout,"Calling CCD_Setup_Dimensions:\n");
	fprintf(stdout,"Chip Size:(%d,%d)\n",Size_X,Size_Y);
	fprintf(stdout,"Binning:(%d,%d)\n",Bin_X,Bin_Y);
	fprintf(stdout,"Amplifier:%d:De-Interlace:%d\n",Amplifier,DeInterlace_Type);
	fprintf(stdout,"Window Flags:%d\n",Window_Flags);
	if(!CCD_Setup_Dimensions("test_setup_dimensions",NULL,handle,Size_X,Size_Y,Bin_X,Bin_Y,Amplifier,
				 DeInterlace_Type,Window_Flags,Window_List))
	{
		CCD_Global_Error();
		return 3;
	}
	fprintf(stdout,"CCD_Setup_Dimensions completed\n");
/* close interface to SDSU controller */
	fprintf(stdout,"CCD_Interface_Close\n");
	CCD_Interface_Close("test_setup_dimensions",NULL,&handle);
	fprintf(stdout,"CCD_Interface_Close completed.\n");
	return 0;
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Text_Print_Level
 * @see #Interface_Device
 * @see #Size_X
 * @see #Size_Y
 * @see #Bin_X
 * @see #Bin_Y
 * @see #Amplifier
 * @see #DeInterlace_Type
 * @see #Window_Flags
 * @see #Window_List
 */
static int Parse_Arguments(int argc, char *argv[])
{
	struct CCD_Setup_Window_Struct window;
	int i,retval,window_number;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-amplifier")==0)||(strcmp(argv[i],"-a")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"left")==0)
				{
					Amplifier = CCD_DSP_AMPLIFIER_LEFT;
					DeInterlace_Type = CCD_DSP_DEINTERLACE_SINGLE;
				}
				else if(strcmp(argv[i+1],"right")==0)
				{
					Amplifier = CCD_DSP_AMPLIFIER_RIGHT;
					DeInterlace_Type = CCD_DSP_DEINTERLACE_FLIP;
				}
				else if(strcmp(argv[i+1],"both")==0)
				{
					Amplifier = CCD_DSP_AMPLIFIER_BOTH;
					DeInterlace_Type = CCD_DSP_DEINTERLACE_SPLIT_SERIAL;
				}
				else
				{
					fprintf(stderr,"Parse_Arguments:Illegal Amplifier '%s', "
						"<left|right|both> required.\n",argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:Amplifier requires <left|right|both>.\n");
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
		else if((strcmp(argv[i],"-window")==0)||(strcmp(argv[i],"-w")==0))
		{
			if((i+5)<argc)
			{
				/* window number */
				retval = sscanf(argv[i+1],"%d",&window_number);
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Window Number failed.\n",argv[i+1]);
					return FALSE;
				}
				switch(window_number)
				{
				case 1:
					Window_Flags |= CCD_SETUP_WINDOW_ONE;
					break;
				case 2:
					Window_Flags |= CCD_SETUP_WINDOW_TWO;
					break;
				case 3:
					Window_Flags |= CCD_SETUP_WINDOW_THREE;
					break;
				case 4:
					Window_Flags |= CCD_SETUP_WINDOW_FOUR;
					break;
				default:
					fprintf(stderr,"Parse_Arguments:Window Number %d out of range(1..4).\n",
						window_number);
					return FALSE;
				}
				/* x start */
				retval = sscanf(argv[i+2],"%d",&(window.X_Start));
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Window Start X (%s) failed.\n",
						argv[i+2]);
					return FALSE;
				}
				/* y start */
				retval = sscanf(argv[i+3],"%d",&(window.Y_Start));
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Window Start Y (%s) failed.\n",
						argv[i+3]);
					return FALSE;
				}
				/* x end */
				retval = sscanf(argv[i+4],"%d",&(window.X_End));
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Window End X (%s) failed.\n",
						argv[i+4]);
					return FALSE;
				}
				/* y end */
				retval = sscanf(argv[i+5],"%d",&(window.Y_End));
				if(retval != 1)
				{
					fprintf(stderr,"Parse_Arguments:Parsing Window End Y (%s) failed.\n",
						argv[i+5]);
					return FALSE;
				}
				/* put window into correct list index 1..4 -> 0..3 */
				Window_List[window_number-1] = window;
				i+= 5;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:-window requires 5 argument:%d supplied.\n",argc-i);
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-xsize")==0)||(strcmp(argv[i],"-xs")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Size_X);
				if(retval != 1)
				{
					fprintf(stderr,
						"Parse_Arguments:Parsing X Size %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:size required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-ysize")==0)||(strcmp(argv[i],"-ys")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Size_Y);
				if(retval != 1)
				{
					fprintf(stderr,
						"Parse_Arguments:Parsing Y Size %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:size required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-xbin")==0)||(strcmp(argv[i],"-xb")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Bin_X);
				if(retval != 1)
				{
					fprintf(stderr,
						"Parse_Arguments:Parsing X Bin %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:bin required.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-ybin")==0)||(strcmp(argv[i],"-yb")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Bin_Y);
				if(retval != 1)
				{
					fprintf(stderr,
						"Parse_Arguments:Parsing Y Bin %s failed.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"Parse_Arguments:bin required.\n");
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
	fprintf(stdout,"Test Setup Dimensions:Help.\n");
	fprintf(stdout,"This program tests the CCD_Setup_Dimensions routine, which sets up the SDSU controller dimensions.\n");
	fprintf(stdout,"test_setup_dimensions [-i[nterface_device] <interface device>]\n");
	fprintf(stdout,"\t[-xs[ize] <no. of pixels>][-ys[ize] <no. of pixels>]\n");
	fprintf(stdout,"\t[-xb[in] <binning factor>][-yb[in] <binning factor>]\n");
	fprintf(stdout,"\t[-a[mplifier] <left|right|both>][-w[indow] <no> <xstart> <ystart> <xend> <yend>]\n");
	fprintf(stdout,"\t[-t[ext_print_level] <commands|replies|values|all>][-h[elp]]\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t-interface_device selects the device to communicate with the SDSU controller.\n");
	fprintf(stdout,"\t-help prints out this message and stops the program.\n");
	fprintf(stdout,"\n");
	fprintf(stdout,"\t<interface device> can be either [pci|text].\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.5  2008/11/20 11:34:58  cjm
** *** empty log message ***
**
** Revision 1.4  2006/11/06 16:52:49  eng
** Added includes to fix implicit function declarations.
**
** Revision 1.3  2006/05/16 18:18:29  cjm
** gnuify: Added GNU General Public License.
**
** Revision 1.2  2004/11/04 16:03:58  cjm
** Added Deinterlace-type = FLIP for right amplifier.
**
** Revision 1.1  2002/11/07 19:18:22  cjm
** Initial revision
**
*/
