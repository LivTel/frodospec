/* ccd_text.c -*- mode: Fundamental;-*-
** low level ccd library
** $Header: /home/cjm/cvs/frodospec/ccd/c/ccd_text.c,v 0.3 2000-01-28 16:19:39 cjm Exp $
*/
/**
 * ccd_text.c implements a virtual interface that prints out all commands that are sent to the SDSU CCD Controller
 * and emulates appropriate replies to requests.
 * @author SDSU, Chris Mottram
 * @version $Revision: 0.3 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for nanosleep.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for nanosleep.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include "ccd_global.h"
#include "ccd_dsp.h"
#include "ccd_pci.h"
#include "ccd_text.h"

/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: ccd_text.c,v 0.3 2000-01-28 16:19:39 cjm Exp $";

/* #defines */
/**
 * Number of PCI argument registers.
 */
#define TEXT_ARGUMENT_COUNT	(5)

/* structures */
/**
 * Structure holding data that the PCI interface would normally know about. This includes
 * the driver request being processed, the HCVR value, values held in the argument registers.
 * <dl>
 * <dt>Ioctl_Request</dt> <dd>The ioctl request.</dd>
 * <dt>HCVR_Command</dt> <dd>The last value put in the HCVR.</dd>
 * <dt>Destination</dt> <dd>The last destination put into the board destination register. Note
 * 	this does not include the number of arguments, see below.</dd>
 * <dt>Argument_List</dt> <dd>The current values of the PCI argument registers. An array of length 
 * 	TEXT_ARGUMENT_COUNT.</dd>
 * <dt>Argument_Count</dt> <dd>The number of arguments in the argument list, 
 * 	set as part of setting a destination.</dd>
 * <dt>Reply</dt> <dd>What we think the reply value should be.</dd>
 * <dt>Exposure_Time</dt> <dd>Set when the exposure time is set.</dd>
 * <dt>Elapsed_Time</dt> <dd>Variable used to simulate exposure time. Reset to zero each time exposure time is set,
 * 	incremented by 1000 each time it is returned when getting exposure time.</dd>
 * </dl>
 * @see #TEXT_ARGUMENT_COUNT
 */
struct Text_Struct
{
	int Ioctl_Request;
	int HCVR_Command;
	int Manual_Command;
	enum CCD_DSP_BOARD_ID Destination;
	int Argument_List[TEXT_ARGUMENT_COUNT];
	int Argument_Count;
	int Reply;
	int Exposure_Time;
	int Elapsed_Time;
};

/**
 * Structure that holds information related to HCVR (Host Command Vector Register) and manual commands.
 * <dl>
 * <dt>Command</dt> <dd>The HCVR or Manual Command number.</dd>
 * <dt>Name</dt> <dd>A textual name for the command.</dd>
 * <dt>Reply</dt> <dd>The default reply value for the command.</dd>
 * <dt>Function</dt> <dd>A function pointer to call to do some special processing (usually relating to setting
 * 	up the reply value in some way).</dd>
 * </dl>
 */
struct Text_Command_Struct
{
	int Command;
	char Name[64];
	int Reply;
	void (*Function)(void);
};

/**
 * Structure that holds information on DSP memory locations and their value.
 * <dl>
 * <dt>Board_Id</dt> <dd>The board the memory location is on.</dd>
 * <dt>Mem_Space</dt> <dd>The memory space the memory location is on.</dd>
 * <dt>Address</dt> <dd>The address of the memory location.</dd>
 * <dt>Value</dt> <dd>The value contained at the memory location.</dd>
 * </dl>
 */
struct Memory_Struct
{
	int Board_Id;
	int Mem_Space;
	int Address;
	int Value;
};

/* external variables */

/* internal routines */
static void Text_Get_Reply(int *argument);
static void Text_HCVR(int hcvr_command);
static void Text_Manual(int manual_command);
static void Text_Destination(int destination_number);
static void Text_HCVR_Test_Data_Link(void);
static void Text_HCVR_Read_Memory(void);
static void Text_HCVR_Read_Exposure_Time(void);
static void Text_HCVR_Start_Exposure(void);

/* local variables */
/**
 * Variable holding error code of last operation performed by ccd_text.
 */
static int Text_Error_Number = 0;
/**
 * Local variable holding description of the last error that occured.
 */
static char Text_Error_String[CCD_GLOBAL_ERROR_STRING_LENGTH] = "";
/**
 * File pointer to where the prints should be sent to.
 */
static FILE *Text_File_Ptr = stdout;
/**
 * Local variable for deciding how detailed the print information is.
 */
static enum CCD_TEXT_PRINT_LEVEL Text_Print_Level = CCD_TEXT_PRINT_LEVEL_COMMANDS;
/**
 * Local variable holding data normally held by the PCI registers.
 * @see #Text_Struct
 */
static struct Text_Struct Text_Data;
/**
 * A list of all the HCVR commands the text driver can process. A Text description is given, the
 * default reply value to set the reply buffer to, and a function pointer to call for cases where the
 * return value must be calculated in some way.
 * @see #Text_Command_Struct
 * @see #HCVR_COMMAND_COUNT
 */
static struct Text_Command_Struct Text_HCVR_Command_List[] = 
{
	{CCD_PCI_HCVR_RESET_CONTROLLER,"Reset Controller",CCD_DSP_SYR,NULL},
	{CCD_PCI_HCVR_LOAD_APPLICATION,"Load Application",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_MANUAL_COMMAND,"Manual Command",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_READ_PCI_STATUS,"Read PCI Status",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_TEST_DATA_LINK,"Test Data Link",0,Text_HCVR_Test_Data_Link},
	{CCD_PCI_HCVR_READ_MEMORY,"Read Memory",0,Text_HCVR_Read_Memory},
	{CCD_PCI_HCVR_WRITE_MEMORY,"Write Memory",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_PCI_DOWNLOAD,"Download PCI code",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_POWER_ON,"Power On",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_POWER_OFF,"Power Off",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_SET_BIAS_VOLTAGES,"Set Bias Voltages",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_CLEAR_ARRAY,"Clear Array",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_STOP_IDLE_MODE,"Stop Idling",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_RESUME_IDLE_MODE,"Resume Idling",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_READ_EXPOSURE_TIME,"Read Exposure Time",0,Text_HCVR_Read_Exposure_Time},
	{CCD_PCI_HCVR_START_EXPOSURE,"Start Exposure",CCD_DSP_DON,Text_HCVR_Start_Exposure},
	{CCD_PCI_HCVR_READ_IMAGE,"Readout Image",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_ABORT_READOUT,"Abort Readout",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_ABORT_EXPOSURE,"Abort Exposure",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_PAUSE_EXPOSURE,"Pause Exposure",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_RESUME_EXPOSURE,"Resume Exposure",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_OPEN_SHUTTER,"Open Shutter",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_CLOSE_SHUTTER,"Close Shutter",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_SET_ARRAY_TEMPERATURE,"Set Array Temperature",CCD_DSP_DON,NULL},
	{CCD_PCI_HCVR_READ_ARRAY_TEMPERATURE,"Read Array Temperature",0xC60,NULL}/* 3168 - -127 C (-102) */
};

/**
 * Hash Definition with the count of HCVR commands in the Text_HCVR_Command_List.
 * @see #Text_HCVR_Command_List
 */
#define HCVR_COMMAND_COUNT (sizeof(Text_HCVR_Command_List)/sizeof(Text_HCVR_Command_List[0]))
/**
 * A list of all the Manual commands the text driver can process. A Text description is given, the
 * default reply value to set the reply buffer to, and a function pointer to call for cases where the
 * return value must be calculated in some way.
 * @see #Text_Command_Struct
 * @see #MANUAL_COMMAND_COUNT
 */
static struct Text_Command_Struct Text_Manual_Command_List[] = 
{
	{CCD_DSP_SGN,"Set Gain",CCD_DSP_DON,NULL}
};

/**
 * Hash Definition with the count of Manual commands in the Text_Manual_Command_List.
 * @see #Text_Manual_Command_List
 */
#define MANUAL_COMMAND_COUNT (sizeof(Text_Manual_Command_List)/sizeof(Text_Manual_Command_List[0]))

/**
 * A list of DSP memory locations and the values in them. This is queried by commands such as Read Memory
 * to return sensible reply value for read memory requests.
 * @see #Memory_Struct
 */
static struct Memory_Struct Memory_List[] =
{
	{CCD_DSP_INTERFACE_BOARD_ID,CCD_DSP_MEM_SPACE_X,1,1}
};

/**
 * Hash Definition with the count of Memory locations in the Memory_List.
 * @see #Memory_List
 */
#define MEMORY_COUNT (sizeof(Memory_List)/sizeof(Memory_List[0]))

/* -------------------------------------------------------------------
** external functions 
** ------------------------------------------------------------------- */
/* device driver config functions */
/**
 * This routine is called to set the amount of information that gets printed out for all the data
 * that is received about a request.
 * @param level The level of printout to print - one of <a href="#CCD_TEXT_PRINT_LEVEL">CCD_TEXT_PRINT_LEVEL</a>:
 * 	CCD_TEXT_PRINT_LEVEL_COMMANDS,
 * 	CCD_TEXT_PRINT_LEVEL_REPLIES,
 * 	CCD_TEXT_PRINT_LEVEL_VALUES,
 * 	CCD_TEXT_PRINT_LEVEL_ALL.
 */
void CCD_Text_Set_Print_Level(enum CCD_TEXT_PRINT_LEVEL level)
{
	Text_Error_Number = 0;
	/* ensure level is a legal print level */
	if(!CCD_TEXT_IS_TEXT_PRINT_LEVEL(level))
	{
		Text_Error_Number = 5;
		sprintf(Text_Error_String,"CCD_Text_Set_Print_Level:Illegal value:level '%d'",level);
		CCD_Text_Error();
		return;
	}
	Text_Print_Level = level;
}

/**
 * This routine sets the file pointer to the passed in argument, to re-direct the output of the text driver
 * to the specified file.
 * @param fp A valid non-NULL file pointer.
 */
void CCD_Text_Set_File_Pointer(FILE *fp)
{
	Text_File_Ptr = fp;
}

/* device driver implementation functions */
/**
 * This routine should be called at startup. 
 * In a real driver it will initialise the connection information ready for the device to be opened.
 * This routine just prints a message and initialises the Text_Data structure.
 * @see ccd_interface.html#CCD_Interface_Initialise
 * @see #Text_Data
 */
void CCD_Text_Initialise(void)
{
	int i;

	Text_Error_Number = 0;
	Text_Data.Ioctl_Request = 0;
	Text_Data.HCVR_Command = 0;
	Text_Data.Manual_Command = 0;
	Text_Data.Destination = 0;
	Text_Data.Argument_Count = 0;
	for(i=0;i<TEXT_ARGUMENT_COUNT;i++)
		Text_Data.Argument_List[i] = 0;
	Text_Data.Reply = -1;
	if(Text_Print_Level == CCD_TEXT_PRINT_LEVEL_ALL)
		fprintf(Text_File_Ptr,"CCD_Text_Initialise\n");
}

/**
 * This routine is called to open the device for communication. In this driver it just
 * prints out a message. 
 * @return Returns TRUE if a device can be opened, otherwise it returns FALSE. Currently, the device can
 * 	always be opened.
 * @see ccd_interface.html#CCD_Interface_Open
 */
int CCD_Text_Open(void)
{
	Text_Error_Number = 0;
	if(Text_Print_Level == CCD_TEXT_PRINT_LEVEL_ALL)
		fprintf(Text_File_Ptr,"CCD_Text_Open\n");
	return TRUE;
}

/**
 * This routine will send a command to the controller. 
 * It uses the internal reply memory which will have previously been setup with the data to send.
 * This driver just prints out the command to be sent.
 * @param request The type of request sent.
 * @param argument A pointer to the argument(s) to the request.
 * @return Returns TRUE if the command was sent to the device driver successfully, FALSE if an error occured.
 * 	In this driver it always return TRUE.
 * @see ccd_interface.html#CCD_Interface_Command
 */
int CCD_Text_Command(int request,int *argument)
{
	char ch,command_word_count;
	int i,j,is_command,paramater_count;
	char board_string[4][16] = {"Host","Interface","Timing","Utility"};

	Text_Error_Number = 0;
	if(Text_Print_Level == CCD_TEXT_PRINT_LEVEL_ALL)
	{
		if(argument != NULL)
			fprintf(Text_File_Ptr,"ioctl(%#x,%#x)\n",request,*argument);
		else
			fprintf(Text_File_Ptr,"ioctl(%#x,NULL)\n",request);
	}
/* set Text_Data Ioctl_Request */
	Text_Data.Ioctl_Request = request;
	switch(request)
	{
		case CCD_PCI_IOCTL_GET_CMDR:
			fprintf(Text_File_Ptr,"Request:Get Manual Command Register:");
			break;
		case CCD_PCI_IOCTL_GET_REPLY:
			fprintf(Text_File_Ptr,"Request:Get Reply:");
			if(argument != NULL)
				Text_Get_Reply(argument);
			else
				fprintf(Text_File_Ptr,"Reply not filled in:argument was NULL:");
			break;
		case CCD_PCI_IOCTL_GET_HCTR:
			fprintf(Text_File_Ptr,"Request:Get Host Control Register:");
			break;
		case CCD_PCI_IOCTL_GET_PROGRESS:
			fprintf(Text_File_Ptr,"Request:Get Readout Progress:");
			break;
		case CCD_PCI_IOCTL_GET_CONFIG_INFO:
			fprintf(Text_File_Ptr,"Request:Get PCI Config Info:");
			break;
		case CCD_PCI_IOCTL_SET_CMDR:
			fprintf(Text_File_Ptr,"Request:Set Manual Command Register:");
			Text_Data.Manual_Command = *argument;
			if(argument != NULL)
				Text_Manual(*argument);
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARG1:
			fprintf(Text_File_Ptr,"Request:Set Argument 1:");
			if(argument != NULL)
			{
				Text_Data.Argument_List[0] = (*argument);
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%#x:",(*argument));
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARG2:
			fprintf(Text_File_Ptr,"Request:Set Argument 2:");
			if(argument != NULL)
			{
				Text_Data.Argument_List[1] = (*argument);
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%#x:",(*argument));
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARG3:
			fprintf(Text_File_Ptr,"Request:Set Argument 3:");
			if(argument != NULL)
			{
				Text_Data.Argument_List[2] = (*argument);
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%#x:",(*argument));
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARG4:
			fprintf(Text_File_Ptr,"Request:Set Argument 4:");
			if(argument != NULL)
			{
				Text_Data.Argument_List[3] = (*argument);
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%#x:",(*argument));
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARG5:
			fprintf(Text_File_Ptr,"Request:Set Argument 5:");
			if(argument != NULL)
			{
				Text_Data.Argument_List[4] = (*argument);
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%#x:",(*argument));
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_ARGS:
			fprintf(Text_File_Ptr,"Request:Set Argument S?:");
			break;
		case CCD_PCI_IOCTL_SET_DESTINATION:
			fprintf(Text_File_Ptr,"Request:Set Destination Board:");
			if(argument != NULL)
				Text_Destination((*argument));
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_HCTR:
			fprintf(Text_File_Ptr,"Request:Set HCTR (Host Interface Control Register):");
			break;
		case CCD_PCI_IOCTL_SET_HCVR:
			fprintf(Text_File_Ptr,"Request:Set HCVR (Host Command Vector Register):");
			Text_Data.HCVR_Command = *argument;
			if(argument != NULL)
				Text_HCVR(*argument);
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_EXPTIME:
			fprintf(Text_File_Ptr,"Request:Set Exposure Time:");
			if(argument != NULL)
			{
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%d millseconds:",(*argument));
				Text_Data.Exposure_Time = (*argument);
				Text_Data.Reply = CCD_DSP_DON;
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_NCOLS:
			fprintf(Text_File_Ptr,"Request:Set Number of Columns:");
			if(argument != NULL)
			{
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%d columns:",(*argument));
				Text_Data.Reply = CCD_DSP_DON;
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_NROWS:
			fprintf(Text_File_Ptr,"Request:Set Number of Rows:");
			if(argument != NULL)
			{
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%d rows:",(*argument));
				Text_Data.Reply = CCD_DSP_DON;
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_SET_IMAGE_BUFFERS:
			fprintf(Text_File_Ptr,"Request:Set address of the image data buffers:");
			break;
		case CCD_PCI_IOCTL_SET_UTIL_OPTIONS:
			fprintf(Text_File_Ptr,"Request:Set Utility options:");
			if(argument != NULL)
			{
				if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
					fprintf(Text_File_Ptr,"%d:",(*argument));
				Text_Data.Reply = CCD_DSP_DON;
			}
			else
				fprintf(Text_File_Ptr,"NULL Argument:");
			break;
		case CCD_PCI_IOCTL_CLEAR_REPLY:
			fprintf(Text_File_Ptr,"Request:Clear Reply Memory:");
			/* clear reply sets the reply memory to -1 */
			Text_Data.Reply = -1;
			break;
		default:
			fprintf(Text_File_Ptr,"Unknown Request");
			break;
	}
	fprintf(Text_File_Ptr,"\n");
	fflush(Text_File_Ptr);
	return TRUE;
}

/**
 * This routine emulates getting reply data from the SDSU CCD Controller. The reply data is stored in
 * the data paramater, up to byte_count bytes of it. This allows the routine to read an arbitary amount of data 
 * (an image for instance).
 * @param data An area of memory previously allocated to store the reply data. The area must be able to store
 * 	at least byte_count bytes. In this routine the image is filled with bytes that are ASCII characters
 * 	so the resultant image can be printed out.
 * @param byte_count The number of bytes to read in from the interface to the data area.
 * @return The routine returns the number of bytes actually read from the interface. In this routine that is
 * 	always byte_count.
 * @see ccd_interface.html#CCD_Interface_Get_Reply_Data
 */
int CCD_Text_Get_Reply_Data(char *data,int byte_count)
{
	int i;

	Text_Error_Number = 0;
	if(Text_Print_Level == CCD_TEXT_PRINT_LEVEL_ALL)
		fprintf(Text_File_Ptr,"CCD_Text_Reply_Data\n");
	/* if data is null we can't put information in it */
	if(data == NULL)
	{
		Text_Error_Number = 1;
		sprintf(Text_Error_String,"CCD_Text_Get_Reply_Data:data is NULL");
		return -1;
	}
	/* range check byte_count */
	if(byte_count <= 0)
	{
		Text_Error_Number = 2;
		sprintf(Text_Error_String,"CCD_Text_Get_Reply_Data:byte_count is too small:%d.",byte_count);
		return -1;
	}
	/* fill data with return values - in this case it happens to be ASCII characters we can print out! */
	i=0;
	while((i<byte_count)&&(!CCD_DSP_Get_Abort()))
	{
		data[i] = (i%(128-32))+32;
		i++;
	}
	return byte_count;
}

/**
 * This routine will close the interface.
 * @return The routine returns the return value from the close routine it called. This will normally be TRUE
 * 	if the device was successfully closed, or FALSE if it failed in some way. In this device, it always
 * 	returns TRUE.
 * @see ccd_interface.html#CCD_Interface_Close
 */
int CCD_Text_Close(void)
{
	Text_Error_Number = 0;
	if(Text_Print_Level == CCD_TEXT_PRINT_LEVEL_ALL)
		fprintf(Text_File_Ptr,"CCD_Text_Close\n");
	return TRUE;
}

/**
 * Get the current value of ccd_text's error number.
 * @return The current value of ccd_text's error number.
 */
int CCD_Text_Get_Error_Number(void)
{
	return Text_Error_Number;
}

/**
 * The error routine that reports any errors occuring in ccd_text in a standard way.
 */
void CCD_Text_Error(void)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Text_Error_Number == 0)
		sprintf(Text_Error_String,"Logic Error:No Error defined");
	fprintf(stderr,"CCD_Text:Error(%d) : %s\n",Text_Error_Number,Text_Error_String);
	Text_Error_Number = 0;
}

/**
 * The error routine that reports any errors occuring in ccd_text in a standard way. This routine places the
 * generated error string at the end of a passed in string argument.
 * @param error_string A string to put the generated error in. This string should be initialised before
 * being passed to this routine. The routine will try to concatenate it's error string onto the end
 * of any string already in existance.
 */
void CCD_Text_Error_String(char *error_string)
{
	/* if the error number is zero an error message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an error to display */
	if(Text_Error_Number == 0)
		sprintf(Text_Error_String,"Logic Error:No Error defined");
	sprintf(error_string+strlen(error_string),"CCD_Text:Error(%d) : %s\n",Text_Error_Number,Text_Error_String);
	Text_Error_Number = 0;
}

/**
 * The warning routine that reports any warnings occuring in ccd_text in a standard way.
 */
void CCD_Text_Warning(void)
{
	/* if the error number is zero an warning message has not been set up
	** This is in itself an error as we should not be calling this routine
	** without there being an warning to display */
	if(Text_Error_Number == 0)
		sprintf(Text_Error_String,"Logic Error:No Warning defined");
	fprintf(stderr,"CCD_Text:Warning(%d) : %s\n",Text_Error_Number,Text_Error_String);
	Text_Error_Number = 0;
}

/* -------------------------------------------------------------------
** 	Internal routines 
** ------------------------------------------------------------------- */
/**
 * Routine called to perform an CCD_PCI_IOCTL_GET_REPLY ioctl request.
 * This involves setting the value at the argument address to the current value of Text_Data.Reply
 * which should have been setup correcly by the last command.
 * Some printing out also takes place here. The spacial case return values CCD_DSP_DON, CCD_DSP_ERR and
 * CCD_DSP_SYR are checked for special printouts.
 * @see ccd_pci.html#CCD_PCI_IOCTL_GET_REPLY
 * @see ccd_dsp.html#CCD_DSP_DON
 * @see ccd_dsp.html#CCD_DSP_ERR
 * @see ccd_dsp.html#CCD_DSP_SYR
 */
static void Text_Get_Reply(int *argument)
{
	struct timespec delay_timespec;
	int finished = -1;

/* wait for a bit - 100th of a second - emulate time taken for reply to appear */
	delay_timespec.tv_sec = 0;
	delay_timespec.tv_nsec = 10000000;
	while(finished != 0)
	{
		finished = nanosleep(&delay_timespec,&delay_timespec);
	}
/* set reply value */
	(*argument) = Text_Data.Reply;
/* if it's a standard reply print out a text representation. */
	if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_REPLIES)
	{
		switch(Text_Data.Reply)
		{
			case CCD_DSP_DON:
				fprintf(Text_File_Ptr,"DON:");
				break;
			case CCD_DSP_ERR:
				fprintf(Text_File_Ptr,"ERR:");
				break;
			case CCD_DSP_SYR:
				fprintf(Text_File_Ptr,"SYR:");
				break;
			default:
				fprintf(Text_File_Ptr,"%#x:",(*argument));
				break;
		}/* end switch on reply value */
	}/* end if printing replies */
}

/**
 * Internal routine which prints information about the HCVR command passed in.
 * This uses the Text_HCVR_Command_List to determines what the command is.
 * It also performs any operations as a result of this command using a function pointer in the list.
 * @param hcvr_command The value to put into the HCVR register.
 * @see #Text_HCVR_Command_List
 */
static void Text_HCVR(int hcvr_command)
{
	int i,found;

	i=0;
	found = FALSE;
	while((i<HCVR_COMMAND_COUNT)&&(found == FALSE))
	{
		found = (Text_HCVR_Command_List[i].Command == hcvr_command);
		if(!found)
			i++;
	}
	if(found)
	{
		if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_COMMANDS)
			fprintf(Text_File_Ptr,":%s:",Text_HCVR_Command_List[i].Name);
		Text_Data.Reply = Text_HCVR_Command_List[i].Reply;
		if(Text_HCVR_Command_List[i].Function != NULL)
			Text_HCVR_Command_List[i].Function();
	}
	else
		fprintf(Text_File_Ptr,":Unknown HCVR command:");
}

/**
 * Internal routine which prints information about the Manual command passed in.
 * This uses the Text_Manual_Command_List to determines what the command is.
 * It also performs any operations as a result of this command using a function pointer in the list.
 * @param manual_command The value to put into the Manual register.
 * @see #Text_Manual_Command_List
 */
static void Text_Manual(int manual_command)
{
	int i,found;

	i=0;
	found = FALSE;
	while((i<MANUAL_COMMAND_COUNT)&&(found == FALSE))
	{
		found = (Text_Manual_Command_List[i].Command == manual_command);
		if(!found)
			i++;
	}
	if(found)
	{
		if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_COMMANDS)
			fprintf(Text_File_Ptr,":%s:",Text_Manual_Command_List[i].Name);
		Text_Data.Reply = Text_Manual_Command_List[i].Reply;
		if(Text_Manual_Command_List[i].Function != NULL)
			Text_Manual_Command_List[i].Function();
	}
	else
		fprintf(Text_File_Ptr,":Unknown Manual command:");
}

/**
 * Routine to convert a CCD_PCI_IOCTL_SET_DESTINATION board word into a textual description string.
 * @param destination_number The argument to the CCD_PCI_IOCTL_SET_DESTINATION ioctl request.
 * @see ccd_pci.html#CCD_PCI_IOCTL_SET_DESTINATION
 */
static void Text_Destination(int destination_number)
{
	char *Board_Name_List[] = {"Host","Interface","Timing board","Utility board"};

	Text_Data.Destination = destination_number&0xFFFF;
	Text_Data.Argument_Count = (destination_number>>16)&0xFFFF;
	if(Text_Print_Level >= CCD_TEXT_PRINT_LEVEL_VALUES)
		fprintf(Text_File_Ptr,":%s:Number of Arguments:%d:",Board_Name_List[Text_Data.Destination],
			Text_Data.Argument_Count);
}

/**
 * Function invoked from Text_HCVR when a TEST_DATA_LINK command is sent to the driver.
 * This tests the driver by sending argument 1 to the required board, which should return the value.
 * Hence this function sets Text_Data.Reply to Text_Data.Argument_List[0].
 * @see #Text_HCVR
 * @see ccd_pci.html#CCD_PCI_HCVR_TEST_DATA_LINK
 */
static void Text_HCVR_Test_Data_Link(void)
{
	Text_Data.Reply = Text_Data.Argument_List[0];
}

/**
 * Routine invoked from Text_HCVR when a Read Memory command is sent to the driver.
 * This routine needs to get the relevant memory address we are reading (board/memory space/address)
 * and return a suitable value for some cases. This uses the Memory_List defined above.
 * @see #Text_HCVR
 * @see #Memory_List
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_MEMORY
 */
static void Text_HCVR_Read_Memory(void)
{
	int i;

	for(i=0;i<MEMORY_COUNT;i++)
	{
		if((Text_Data.Destination == Memory_List[i].Board_Id)&&
			(Text_Data.Argument_List[0] == Memory_List[i].Mem_Space)&&
			(Text_Data.Argument_List[1] == Memory_List[i].Address))
		{
			Text_Data.Reply = Memory_List[i].Value;
		}
	}
}

/**
 * Function invoked from Text_HCVR when a READ_EXPOSURE_TIME command is sent to the driver.
 * This should return the elapsed time of exposure.
 * We simulate this by incrementing the Elapsed_Time variable in Text_Data by 1000 milliseconds each time this
 * function is called. Text_Data.Reply is set to this new value.
 * @see #Text_HCVR
 * @see ccd_pci.html#CCD_PCI_HCVR_READ_EXPOSURE_TIME
 */
static void Text_HCVR_Read_Exposure_Time(void)
{
	Text_Data.Elapsed_Time += 1000;
	Text_Data.Reply = Text_Data.Elapsed_Time;
}

/**
 * Function invoked from Text_HCVR when a START_EXPOSURE command is sent to the driver.
 * Text_Data sets the reply value to CCD_DSP_DON.
 * We reset the Text_Data.Elapsed_Time variable to 0 so that the read exposure time command can increment it.
 * @see #Text_HCVR
 * @see ccd_pci.html#CCD_PCI_HCVR_START_EXPOSURE
 */
static void Text_HCVR_Start_Exposure(void)
{
	Text_Data.Elapsed_Time = 0;
}

/*
** $Log: not supported by cvs2svn $
** Revision 0.2  2000/01/26 14:02:39  cjm
** Added low level printout for regression testing against voodoo.
**
** Revision 0.1  2000/01/25 14:57:27  cjm
** initial revision (PCI version).
**
*/
