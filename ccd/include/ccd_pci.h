/* ccd_pci.h  -*- mode: Fundamental;-*-
** $Header: /home/cjm/cvs/frodospec/ccd/include/ccd_pci.h,v 0.1 2000-01-25 15:03:32 cjm Exp $
*/

#ifndef CCD_PCI_H
#define CCD_PCI_H

/**
 * Default string for the name of the first device.
 */
#define CCD_PCI_DEFAULT_DEVICE_ZERO		"/dev/astropci0"
/**
 * Default string for the name of the second device.
 */
#define CCD_PCI_DEFAULT_DEVICE_ONE		"/dev/astropci1"

/* The next block of hash definitions are PCI interface meta commands for passing to the HCVR
** Host Command Vector Register */
/* Should this be an enum ? */
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command resets the SDSU controller.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_RESET_CONTROLLER 		(0x807D)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command loads a DSP application from on board ROM to one of the controller boards.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_LOAD_APPLICATION		(0x807F)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command makes the PCI board issue a manual command to another board, using arguments already set up.
 * This enables the software to send low-level DSP commands direct to the relevant board. (e.g. SGN).
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_MANUAL_COMMAND 		(0x8081)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command reads the PCI boards status word.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_READ_PCI_STATUS 		(0x8083)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command tests the data links to specified board to test the communication path.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_TEST_DATA_LINK 		(0x8085)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command reads a memory location from a specified board and memory argument. The destination board is specified
 * with CCD_PCI_IOCTL_SET_DESTINATION. The type of memory should be written to argument 1 using
 * CCD_PCI_IOCTL_SET_ARG1. The address should be written to argument 2 using CCD_PCI_IOCTL_SET_ARG2.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 * @see #CCD_PCI_IOCTL_SET_DESTINATION
 * @see #CCD_PCI_IOCTL_SET_ARG1
 * @see #CCD_PCI_IOCTL_SET_ARG2
 */
#define CCD_PCI_HCVR_READ_MEMORY 		(0x8087)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command writes to a memory location on a specified board and memory type. The destination board is specified
 * with CCD_PCI_IOCTL_SET_DESTINATION. The type of memory should be written to argument 1 using
 * CCD_PCI_IOCTL_SET_ARG1. The address should be written to argument 2 using CCD_PCI_IOCTL_SET_ARG2. The value
 * to write at the address should be written to argument 3 using CCD_PCI_IOCTL_SET_ARG3.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 * @see #CCD_PCI_IOCTL_SET_DESTINATION
 * @see #CCD_PCI_IOCTL_SET_ARG1
 * @see #CCD_PCI_IOCTL_SET_ARG2
 * @see #CCD_PCI_IOCTL_SET_ARG
 */
#define CCD_PCI_HCVR_WRITE_MEMORY 		(0x8089)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command is used to indicate a PCI DSP file download is to follow.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_PCI_DOWNLOAD 		(0x808B)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command turns on the power to the analog boards in a controlled manner. It is equivalent to issuing
 * a PON command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_POWER_ON 			(0x808D)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command turns off the power to the analog boards in a controlled manner. It is equivalent to issuing
 * a POF command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_POWER_OFF 			(0x808F)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command sets the bias voltages by reading the voltages codes on the timing board Y: memory area and
 * writing them to the video processor. It is equivalent to sending the SBV DSP command to the timing board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_SET_BIAS_VOLTAGES 		(0x8091)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command clears the image from the CCD by executing Y:4 parrallel clock shifts on the timing board. It is
 * equivalent to the CLR DSP command sent to the timing board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_CLEAR_ARRAY 		(0x8093)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This stops the timing board being in idle mode. It is equivalent to issuing a STP DSP command to the timing board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_STOP_IDLE_MODE 		(0x8095)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command puts the timing board in idle mode. This puts the clocks in the readout sequence, but doesn't
 * transfer any data, to stop the CCD building up charge. It is equivalent to issuing an IDL DSP command to the
 * timing board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_RESUME_IDLE_MODE 		(0x8097)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command gets the current time the utility board has had the shutter open for as a result of a SEX command
 * (the current length of the exposure). It is equivalent to the WRM DSP command to the utility board at
 * memory location Y:23.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_READ_EXPOSURE_TIME 	(0x8099)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the SDSU controller to start an exposure. It is equivalent to issuing a SEX command to the
 * utility board. Note a CCD_PCI_HCVR_READ_IMAGE needs to issued after this command to cause the 
 * controller to read the image out.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 * @see #CCD_PCI_HCVR_READ_IMAGE
 */
#define CCD_PCI_HCVR_START_EXPOSURE 		(0x809B)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the timing board to read out the image data from the CCD.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_READ_IMAGE 		(0x809D)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the timing board and PCI interface to abort the readout of an image on the CCD.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_ABORT_READOUT 		(0x809F)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes an exposure in progress to be aborted. It is equivalent to issuing a AEX DSP command to the
 * utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_ABORT_EXPOSURE 		(0x80A1)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the utility board to pause an exposure in progress. It is equivalent to issuing a 
 * PEX DSP command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_PAUSE_EXPOSURE 		(0x80A3)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the utility board to resume an exposure in progress. It is equivalent to issuing a 
 * REX DSP command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_RESUME_EXPOSURE 		(0x80A5)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the utility board to open the shutter. It is equivalent to issuing a 
 * OSH command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_OPEN_SHUTTER 		(0x80A9)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command causes the utility board to close the shutter. It is equivalent to issuing a 
 * CSH command to the utility board.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_CLOSE_SHUTTER 		(0x80AB)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command sets the target temperature on the utility board. The desired temperature (in adu counts) should
 * be written to the argument 1 register using CCD_PCI_IOCTL_SET_ARG1 before this command is invoked. 
 * It is equivalent to writing the adu counts
 * to the Y:28 memory location on the utility board using the WRM DSP command.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_SET_ARRAY_TEMPERATURE 	(0x80AD)
/**
 * HCVR (Host Command Vector Register) command. Used as an ioctl request argument for the CCD_PCI_IOCTL_SET_HCVR
 * ioctl request.
 * This command reads the current array temperature from the utility board. It is equivalent to issuing a RDM
 * DSP command to the utility board with memory location Y:12.
 * @see #CCD_PCI_IOCTL_SET_HCVR
 */
#define CCD_PCI_HCVR_READ_ARRAY_TEMPERATURE 	(0x80AF)

/* The next block of hash definitions are PCI ioctl request numbers */
/* Should this be an enum ? */

/**
 * ioctl request code for the SDSU controller PCI interface.
 * Get the current value of the manual command register.
 */
#define CCD_PCI_IOCTL_GET_CMDR 			(0x0)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Get the current reply from the reply buffer.
 */
#define CCD_PCI_IOCTL_GET_REPLY 		(0x3)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Get the current value of the PCI DSP Host Control Register.
 */
#define CCD_PCI_IOCTL_GET_HCTR 			(0x5)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Get the current value of the PCI DSP Host Status Register.
 */
#define CCD_PCI_IOCTL_GET_HSTR 			(0x6)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * This command gets the progress of the current read image command.
 */
#define CCD_PCI_IOCTL_GET_PROGRESS 		(0x7)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Returns the configuration space register values.
 */
#define CCD_PCI_IOCTL_GET_CONFIG_INFO 		(0x314)

/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the value of the manual command register.
 */
#define CCD_PCI_IOCTL_SET_CMDR 			(0x100)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of argument register 1.
 */
#define CCD_PCI_IOCTL_SET_ARG1 			(0x105)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of argument register 2.
 */
#define CCD_PCI_IOCTL_SET_ARG2 			(0x106)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of argument register 3.
 */
#define CCD_PCI_IOCTL_SET_ARG3 			(0x107)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of argument register 4.
 */
#define CCD_PCI_IOCTL_SET_ARG4 			(0x108)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of argument register 5.
 */
#define CCD_PCI_IOCTL_SET_ARG5 			(0x109)
/**
 * ioctl request code for the SDSU controller PCI interface.
 */
#define CCD_PCI_IOCTL_SET_ARGS 			(0x110)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current value of the manual command register.
 */
#define CCD_PCI_IOCTL_SET_DESTINATION 		(0x111)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the value of the HCTR (Host Interface Control Register).
 */
#define CCD_PCI_IOCTL_SET_HCTR 			(0x115)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the value of the HCVR (Host Command Vector Register).
 */
#define CCD_PCI_IOCTL_SET_HCVR 			(0x117)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the current exposure time in the camera timing table.
 */
#define CCD_PCI_IOCTL_SET_EXPTIME 		(0x119)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the number of array columns in the camera timing table.
 */
#define CCD_PCI_IOCTL_SET_NCOLS 		(0x120)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the number of array rows in the camera timing table.
 */
#define CCD_PCI_IOCTL_SET_NROWS 		(0x121)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set the address of the image data buffers.
 */
#define CCD_PCI_IOCTL_SET_IMAGE_BUFFERS 	(0x122)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Set utility board option word.
 */
#define CCD_PCI_IOCTL_SET_UTIL_OPTIONS 		(0x123)
/**
 * ioctl request code for the SDSU controller PCI interface.
 * Clears the PCI reply buffer. It is initialised to -1.
 */
#define CCD_PCI_IOCTL_CLEAR_REPLY 		(0x301)

/* external routines */
extern void CCD_PCI_Initialise(void);
extern int CCD_PCI_Open(void);
extern int CCD_PCI_Command(int request,int *argument);
extern int CCD_PCI_Get_Reply_Data(void *data,int byte_count);
extern int CCD_PCI_Close(void);
extern int CCD_PCI_Get_Error_Number(void);
extern void CCD_PCI_Error(void);
extern void CCD_PCI_Error_String(char *error_string);
extern void CCD_PCI_Warning(void);

#endif
