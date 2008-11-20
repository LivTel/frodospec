// CCDLibrary.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ccd/CCDLibrary.java,v 1.1 2008-11-20 11:34:28 cjm Exp $
package ngat.frodospec.ccd;

import java.lang.*;
import java.util.List;
import java.util.Vector;
import ngat.util.logging.*;

/**
 * This class supports an interface to the SDSU CCD Controller library, for controlling the FrodoSpec CCD.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class CCDLibrary
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: CCDLibrary.java,v 1.1 2008-11-20 11:34:28 cjm Exp $");
	// ccd_dsp.h
	/* These constants should be the same as those in ccd_dsp.h */
	/**
	 * Set Gain parameter, to set gain to 1.
	 * @see #setup
	 */
	public final static int DSP_GAIN_ONE = 			        0x1;
	/**
	 * Set Gain parameter, to set gain to 2.
	 * @see #setup
	 */
	public final static int DSP_GAIN_TWO = 	 		        0x2;
	/**
	 * Set Gain parameter, to set gain to 4.75.
	 * @see #setup
	 */
	public final static int DSP_GAIN_FOUR = 			0x5;
	/**
	 * Set Gain parameter, to set gain to 9.5.
	 * @see #setup
	 */
	public final static int DSP_GAIN_NINE = 			0xa;

	/* These constants should be the same as those in ccd_dsp.h */
	/**
	 * Set Output Source parameter, to make the controller read out images from the left amplifier.
	 * @see #setupDimensions
	 */
	public final static int DSP_AMPLIFIER_LEFT 	=		0x5f5f4c;
	/**
	 * Set Output Source parameter, to make the controller read out images from the right amplifier.
	 * @see #setupDimensions
	 */
	public final static int DSP_AMPLIFIER_RIGHT	=		0x5f5f52;
	/**
	 * Set Output Source parameter, to make the controller read out images from both amplifiers.
	 * @see #setupDimensions
	 */
	public final static int DSP_AMPLIFIER_BOTH 	=		0x5f4c52;

	/* These constants should be the same as those in ccd_dsp.h */
	/**
	 * De-interlace type. This setting does no deinterlacing, as the CCD was read out from a single readout.
	 * @see #setupDimensions
	 */
	public final static int DSP_DEINTERLACE_SINGLE = 		0;
	/**
	 * De-interlace type. This setting flips the output image in X, if the CCD was readout from the
	 * "wrong" amplifier, i.e. to ensure east is to the left.
	 * @see #setupDimensions
	 */
	public final static int DSP_DEINTERLACE_FLIP = 		         1;
	/**
	 * De-interlace type. This setting deinterlaces split parallel readout.
	 * @see #setupDimensions
	 */
	public final static int DSP_DEINTERLACE_SPLIT_PARALLEL = 	  2;
	/**
	 * De-interlace type. This setting deinterlaces split serial readout.
	 * @see #setupDimensions
	 */
	public final static int DSP_DEINTERLACE_SPLIT_SERIAL =  	  3;
	/**
	 * De-interlace type. This setting deinterlaces split quad readout.
	 * @see #setupDimensions
	 */
	public final static int DSP_DEINTERLACE_SPLIT_QUAD = 	          4;

// ccd_exposure.h
	/* These constants should be the same as those in ccd_exposure.h */
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_NONE               = 0;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_WAIT_START         = 1;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_PRE_EXPOSE_READOUT = 2;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_EXPOSE             = 3;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_PRE_READOUT        = 4;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_READOUT            = 5;
	/**
	 * Exposure status number.
	 * @see #getExposureStatus
	 */
	public final static int EXPOSURE_STATUS_POST_READOUT       = 6;
// ccd_global.h
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_SETUP       = (1<<8);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_EXPOSURE    = (1<<9);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_DSP         = (1<<12);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_INTERFACE   = (1<<13);
	/**
	 * Logging filter bit.
	 * @see #setLogFilterLevel
	 */
	public final static int LOG_BIT_GLOBAL      = (1<<14);
// ccd_interface.h
	/* These constants should be the same as those in ccd_interface.h */
	/**
	 * Interface device number, showing that commands will currently be sent nowhere.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_NONE = 		0;
	/**
	 * Interface device number, showing that commands will currently be sent to the text interface 
	 * to be printed out.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_TEXT =		1;
	/**
	 * Interface device number, showing that commands will currently be sent to the PCI interface.
	 * @see #interfaceOpen
	 */
	public final static int INTERFACE_DEVICE_PCI = 		2;
// ccd_setup.h 
	/* These constants should be the same as those in ccd_setup.h */
	/**
	 * The number of windows the controller can put on the CCD.
	 */
	public final static int SETUP_WINDOW_COUNT = 		4;
	/**
	 * Window flag used as part of the window_flags bit-field parameter of setupDimensions to specify the
	 * first window position is to be used.
	 * @see #setupDimensions
	 */
	public final static int SETUP_WINDOW_ONE =	       	(1<<0);
	/**
	 * Window flag used as part of the window_flags bit-field parameter of setupDimensions to specify the
	 * second window position is to be used.
	 * @see #setupDimensions
	 */
	public final static int SETUP_WINDOW_TWO =		 (1<<1);
	/**
	 * Window flag used as part of the window_flags bit-field parameter of setupDimensions to specify the
	 * third window position is to be used.
	 * @see #setupDimensions
	 */
	public final static int SETUP_WINDOW_THREE =		  (1<<2);
	/**
	 * Window flag used as part of the window_flags bit-field parameter of setupDimensions to specify the
	 * fourth window position is to be used.
	 * @see #setupDimensions
	 */
	public final static int SETUP_WINDOW_FOUR =		   (1<<3);
	/**
	 * Window flag used as part of the window_flags bit-field parameter of setupDimensions to specify all the
	 * window positions are to be used.
	 * @see #setupDimensions
	 */
	public final static int SETUP_WINDOW_ALL =		    (SETUP_WINDOW_ONE|SETUP_WINDOW_TWO|
								     SETUP_WINDOW_THREE|SETUP_WINDOW_FOUR);

	/* These constants should be the same as those in ccd_setup.h */
	/**
	 * Setup Load Type passed to setupStartup as a loadType parameter. This makes setupStartup do
	 * nothing for the DSP code for the relevant board, as it assumes the DSP code was loaded from ROM
	 * at bootup.
	 * @see #setup
	 */
	public final static int SETUP_LOAD_ROM = 			0;
	/**
	 * Setup Load Type passed to setupStartup as a loadType parameter, to load DSP application code from 
	 * EEPROM.
	 * @see #setup
	 */
	public final static int SETUP_LOAD_APPLICATION = 		1;
	/**
	 * Setup flag passed to setupStartup as a loadType parameter, to load DSP application code from a file.
	 * @see #setup
	 */
	public final static int SETUP_LOAD_FILENAME = 		2;

// ccd_text.h
	/* These constants should be the same as those in ccd_text.h */
	/**
	 * Level number passed to setTextPrintLevel, to print out commands only.
	 * @see #setTextPrintLevel
	 */
	public final static int TEXT_PRINT_LEVEL_COMMANDS =   	0;
	/**
	 * Level number passed to setTextPrintLevel, to print out commands replies as well.
	 * @see #setTextPrintLevel
	 */
	public final static int TEXT_PRINT_LEVEL_REPLIES = 	  	1;
	/**
	 * Level number passed to setTextPrintLevel, to print out parameter value information as well.
	 * @see #setTextPrintLevel
	 */
	public final static int TEXT_PRINT_LEVEL_VALUES = 	     	2;
	/**
	 * Level number passed to setTextPrintLevel, to print out everything.
	 * @see #setTextPrintLevel
	 */
	public final static int TEXT_PRINT_LEVEL_ALL = 		3;

// ccd_dsp.h
	/**
	 * Native wrapper to libfrodospec_ccd routine "Read Elapsed Time" that reads the alapsed exposure length.
	 */
	private native int CCD_DSP_Command_RET();
// ccd_exposure.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that does an exposure.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Exposure_Expose(boolean openShutter,long startTime,int exposureTime,List filenameList) 
		throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that does a bias frame.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Exposure_Bias(String filename) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that aborts an exposure.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Exposure_Abort() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine thats returns whether an exposure is currently in progress.
	 */
	private native int CCD_Exposure_Get_Exposure_Status();
	/**
	 * Native wrapper to libfrodospec_ccd routine thats returns the length of the current/last exposure.
	 */
	private native int CCD_Exposure_Get_Exposure_Length();
	/**
	 * Native wrapper to libfrodospec_ccd routine thats returns the start time of the current/last exposure,
	 * in milliseconds since 1970.
	 */
	private native long CCD_Exposure_Get_Exposure_Start_Time();
// ccd_global.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that sets up the CCD library for use.
	 */
	private native void CCD_Global_Initialise();
	/**
	 * Native wrapper to libfrodospec_ccd routine that changes the log Filter Level.
	 */
	private native void CCD_Global_Set_Log_Filter_Level(int level);
// ccd_interface.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that opens the selected interface device.
	 * @param interfaceDevice The interface device to use to communicate with the SDSU CCD Controller.
	 * 	One of: INTERFACE_DEVICE_NONE, INTERFACE_DEVICE_TEXT, INTERFACE_DEVICE_PCI.
	 * @param devicePathname The pathname of the device. For devices of type PCI, this will be something like
	 *        "/dev/astropci0". For devices of type TEXT, this should be a valid pathname to a (possibly
	 *        existing text file (i.e. ~dev/tmp/output.txt).
	 * @see #INTERFACE_DEVICE_NONE
	 * @see #INTERFACE_DEVICE_TEXT
	 * @see #INTERFACE_DEVICE_PCI
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Interface_Open(int interfaceDevice,String devicePathname)
		throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that closes the selected interface device.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Interface_Close() throws CCDLibraryNativeException;
// ccd_setup.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that does the CCD setup.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Setup_Startup(int pci_load_type, String pci_filename,
		int timing_load_type,int timing_application_number,String timing_filename,
		int utility_load_type,int utility_application_number,String utility_filename,
		double target_temperature,int gain,boolean gain_speed,boolean idle) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that does the CCD shutdown.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Setup_Shutdown() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that does the CCD dimensions setup.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Setup_Dimensions(int ncols,int nrows,int nsbin,int npbin,
						 int amplifier,int deinterlace_setting,int window_flags,
						 CCDLibrarySetupWindow window_list[]) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that performs a hardware test data link.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Setup_Hardware_Test(int test_count) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that aborts the CCD setup.
	 */
	private native void CCD_Setup_Abort();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the number of columns.
	 */
	private native int CCD_Setup_Get_NCols();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the number of Rows.
	 */
	private native int CCD_Setup_Get_NRows();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the serial (column/X) binning.
	 */
	private native int CCD_Setup_Get_NSBin();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the parallel (row/Y) binning.
	 */
	private native int CCD_Setup_Get_NPBin();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the last configured readout amplifier.
	 */
	private native int CCD_Setup_Get_Amplifier();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the last configured deinterlace type.
	 */
	private native int CCD_Setup_Get_DeInterlace_Type();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets whether a setup operation has been completed 
	 * successfully.
	 */
	private native boolean CCD_Setup_Get_Setup_Complete();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets whether a setup operation is in progress.
	 */
	private native boolean CCD_Setup_Get_Setup_In_Progress();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the setup window flags.
	 */
	private native int CCD_Setup_Get_Window_Flags();
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets a setup CCD window.
	 */
	private native CCDLibrarySetupWindow CCD_Setup_Get_Window(int window_index) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the setup window width.
	 */
	private native int CCD_Setup_Get_Window_Width(int index);
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the setup window height.
	 */
	private native int CCD_Setup_Get_Window_Height(int index);
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets voltage ADUS.
	 */
	private native int CCD_Setup_Get_High_Voltage_Analogue_ADU() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets voltage ADUS.
	 */
	private native int CCD_Setup_Get_Low_Voltage_Analogue_ADU() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets voltage ADUS.
	 */
	private native int CCD_Setup_Get_Minus_Low_Voltage_Analogue_ADU() throws CCDLibraryNativeException;

// ccd_temperature.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the current temperature of the CCD.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native double CCD_Temperature_Get() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that sets the current temperature of the CCD.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native void CCD_Temperature_Set(double target_temperature) throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the ADU counts from the 
	 * utility board temperature sensor.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native int CCD_Temperature_Get_Utility_Board_ADU() throws CCDLibraryNativeException;
	/**
	 * Native wrapper to libfrodospec_ccd routine that gets the current heater ADU counts.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 */
	private native int CCD_Temperature_Get_Heater_ADU() throws CCDLibraryNativeException;
// ccd_text.h
	/**
	 * Native wrapper to libfrodospec_ccd routine that sets the amount of output from the text interface.
	 */
	private native void CCD_Text_Set_Print_Level(int level);

//internal C layer initialisation
	/**
	 * Native method that allows the JNI layer to store a reference to this Class's logger.
	 * @param logger The logger for this class.
	 */
	private native void initialiseLoggerReference(Logger logger);
	/**
	 * Native method that allows the JNI layer to release the global reference to this Class's logger.
	 */
	private native void finaliseLoggerReference();

// per instance variables
	/**
	 * The logger to log messages to.
	 */
	protected Logger logger = null;

// static code block
	/**
	 * Static code to load frodospec_libccd, the shared C library that implements an interface to the
	 * SDSU CCD Controller.
	 */
	static
	{
		System.loadLibrary("frodospec_ccd");
	}

// constructor
	/**
	 * Constructor. Constructs the logger, and sets the C layers reference to it.
	 * @see #logger
	 * @see #initialiseLoggerReference
	 */
	public CCDLibrary()
	{
		super();
		logger = LogManager.getLogger(this);
		initialiseLoggerReference(logger);
	}

	/**
	 * Finalize method for this class, delete JNI global references.
	 * @see #finaliseLoggerReference
	 */
	protected void finalize() throws Throwable
	{
		super.finalize();
		finaliseLoggerReference();
	}

// ccd_dsp.h
	/**
	 * Method to get the elapsed exposure length of the current exposure.
	 * @return The elapsed exposure length in milliseconds.
	 * @see #CCD_DSP_Command_RET
	 */
	public int getElapsedExposureTime()
	{
		return CCD_DSP_Command_RET();
	}

	/**
	 * Routine to parse a gain string and return a gain number suitable for input into
	 * <a href="#setupStartup">setupStartup</a>, or a DSP Set Gain (SGN) command. 
	 * @param s The string to parse.
	 * @return The gain number, one of:
	 * 	<ul>
	 * 	<li><a href="#DSP_GAIN_ONE">DSP_GAIN_ONE</a>
	 * 	<li><a href="#DSP_GAIN_TWO">DSP_GAIN_TWO</a>
	 * 	<li><a href="#DSP_GAIN_FOUR">DSP_GAIN_FOUR</a>
	 * 	<li><a href="#DSP_GAIN_NINE">DSP_GAIN_NINE</a>
	 * 	</ul>.
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 */
	public static int dspGainFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("DSP_GAIN_ONE"))
			return DSP_GAIN_ONE;
		if(s.equals("DSP_GAIN_TWO"))
			return DSP_GAIN_TWO;
		if(s.equals("DSP_GAIN_FOUR"))
			return DSP_GAIN_FOUR;
		if(s.equals("DSP_GAIN_NINE"))
			return DSP_GAIN_NINE;
		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","dspGainFromString",s);
	}

	/**
	 * Routine to parse an amplifier string and return a amplifier number suitable for input into
	 * <a href="#setupDimensions">setupDimensions</a>, or a DSP Set Output Source (SOS) command. 
	 * @param s The string to parse.
	 * @return The amplifier number, one of:
	 * 	<ul>
	 * 	<li><a href="#DSP_AMPLIFIER_LEFT">DSP_AMPLIFIER_LEFT</a>
	 * 	<li><a href="#DSP_AMPLIFIER_RIGHT">DSP_AMPLIFIER_RIGHT</a>
	 * 	<li><a href="#DSP_AMPLIFIER_BOTH">DSP_AMPLIFIER_BOTH</a>
	 * 	</ul>.
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 */
	public static int dspAmplifierFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("DSP_AMPLIFIER_LEFT"))
			return DSP_AMPLIFIER_LEFT;
		if(s.equals("DSP_AMPLIFIER_RIGHT"))
			return DSP_AMPLIFIER_RIGHT;
		if(s.equals("DSP_AMPLIFIER_BOTH"))
			return DSP_AMPLIFIER_BOTH;
		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","dspAmplifierFromString",s);
	}

	/**
	 * Routine to parse a string containing a representation of a valid deinterlace type and to return
	 * the numeric value of that type, suitable for passing into 
	 * <a href="#setupDimensions">setupDimensions</a> and other methods.
	 * @param s The string to parse.
	 * @return The deinterlace type number, one of:
	 * 	<ul>
	 * 	<li><a href="#DSP_DEINTERLACE_SINGLE">DSP_DEINTERLACE_SINGLE</a>
	 * 	<li><a href="#DSP_DEINTERLACE_FLIP">DSP_DEINTERLACE_FLIP</a>
	 * 	<li><a href="#DSP_DEINTERLACE_SPLIT_PARALLEL">DSP_DEINTERLACE_SPLIT_PARALLEL</a>
	 * 	<li><a href="#DSP_DEINTERLACE_SPLIT_SERIAL">DSP_DEINTERLACE_SPLIT_SERIAL</a>
	 * 	<li><a href="#DSP_DEINTERLACE_SPLIT_QUAD">DSP_DEINTERLACE_SPLIT_QUAD</a>
	 * 	</ul>.
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 */
	public static int dspDeinterlaceFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("DSP_DEINTERLACE_SINGLE"))
			return DSP_DEINTERLACE_SINGLE;
		if(s.equals("DSP_DEINTERLACE_FLIP"))
			return DSP_DEINTERLACE_FLIP;
		if(s.equals("DSP_DEINTERLACE_SPLIT_PARALLEL"))
			return DSP_DEINTERLACE_SPLIT_PARALLEL;
		if(s.equals("DSP_DEINTERLACE_SPLIT_SERIAL"))
			return DSP_DEINTERLACE_SPLIT_SERIAL;
		if(s.equals("DSP_DEINTERLACE_SPLIT_QUAD"))
			return DSP_DEINTERLACE_SPLIT_QUAD;
		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","dspDeinterlaceFromString",s);
	}

// ccd_exposure.h
	/**
	 * Routine to perform an exposure.
	 * @param openShutter Determines whether the shutter should be opened to do the exposure. The shutter might
	 * 	be left closed to perform calibration images etc.
	 * @param startTime The start time, in milliseconds since the epoch (1st January 1970) to start the exposure.
	 * 	Passing the value -1 will start the exposure as soon as possible.
	 * @param exposureTime The number of milliseconds to expose the CCD.
	 * @param filenameList A list of filename strings (one per window) to save the exposure into.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if 
	 *           CCD_Exposure_Expose failed.
	 * @see #CCD_Exposure_Expose
	 */
	public void expose(boolean openShutter,long startTime,int exposureTime,List filenameList) 
		throws CCDLibraryNativeException
	{
		CCD_Exposure_Expose(openShutter,startTime,exposureTime,filenameList);
	}

	/**
	 * Routine to perform an exposure.
	 * @param openShutter Determines whether the shutter should be opened to do the exposure. The shutter might
	 * 	be left closed to perform calibration images etc.
	 * @param startTime The start time, in milliseconds since the epoch (1st January 1970) to start the exposure.
	 * 	Passing the value -1 will start the exposure as soon as possible.
	 * @param exposureTime The number of milliseconds to expose the CCD.
	 * @param filename The filename to save the exposure into.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if 
	 *           CCD_Exposure_Expose failed.
	 * @see #CCD_Exposure_Expose
	 */
	public void expose(boolean openShutter,long startTime,int exposureTime,String filename) 
		throws CCDLibraryNativeException
	{
		List filenameList = null;

		filenameList = new Vector();
		filenameList.add(filename);
		CCD_Exposure_Expose(openShutter,startTime,exposureTime,filenameList);
	}

	/**
	 * Routine to perform a bias frame.
	 * @param filename The filename to save the bias frame into.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if 
	 *            CCD_Exposure_Bias failed.
	 * @see #CCD_Exposure_Bias
	 */
	public void bias(String filename) throws CCDLibraryNativeException
	{
		CCD_Exposure_Bias(filename);
	}

	/**
	 * Routine to abort an exposure/bias.
	 * @exception CCDLibraryNativeException This routine throws a CCDLibraryNativeException if 
	 *            the method failed.
	 * @see #CCD_Exposure_Abort
	 */
	public void abort() throws CCDLibraryNativeException
	{
		CCD_Exposure_Abort();
	}

	/**
	 * Returns whether an exposure is currently in progress. The library keeps track of whether a call to
	 * expose is in progress, and whether it is exposing or reading out.
	 * @return Returns the exposure status.
	 * @see #expose
	 * @see #CCD_Exposure_Get_Exposure_Status
	 * @see #EXPOSURE_STATUS_WAIT_START
	 * @see #EXPOSURE_STATUS_NONE
	 * @see #EXPOSURE_STATUS_PRE_EXPOSE_READOUT
	 * @see #EXPOSURE_STATUS_EXPOSE
	 * @see #EXPOSURE_STATUS_PRE_READOUT
	 * @see #EXPOSURE_STATUS_READOUT
	 * @see #EXPOSURE_STATUS_POST_READOUT
	 */
	public int getExposureStatus()
	{
		return CCD_Exposure_Get_Exposure_Status();
	}

	/**
	 * Method to get the exposure length the controller was last set to.
	 * @return The exposure length.
	 * @see #CCD_Exposure_Get_Exposure_Length
	 */
	public int getExposureLength()
	{
		return CCD_Exposure_Get_Exposure_Length();
	}

	/**
	 * Method to get number of milliseconds since 1st Jan 1970 to the exposure start time.
	 * @return A long, in milliseconds.
	 * @see #CCD_Exposure_Get_Exposure_Start_Time
	 */
	public long getExposureStartTime()
	{
		return CCD_Exposure_Get_Exposure_Start_Time();
	}

// ccd_global.h
	/**
	 * Routine that sets up all the parts of CCDLibrary at the start of it's use. This routine should be
	 * called once at the start of each program. 
	 * @see #CCD_Global_Initialise
	 */
	public void initialise()
	{
		CCD_Global_Initialise();
	}

	/**
	 * Routine that changes the libfrodospec_ccd logging filter level.
	 * @param level The logging filter level.
	 * @see #CCD_Global_Set_Log_Filter_Level
	 * @see #LOG_BIT_SETUP
	 * @see #LOG_BIT_EXPOSURE
	 * @see #LOG_BIT_DSP
	 * @see #LOG_BIT_INTERFACE
	 * @see #LOG_BIT_GLOBAL
	 */
	public void setLogFilterLevel(int level)
	{
		CCD_Global_Set_Log_Filter_Level(level);
	}

// ccd_interface.h
	/**
	 * Routine to open the interface. 
	 * @param interfaceDevice The interface device to use to communicate with the SDSU CCD Controller.
	 * 	One of: INTERFACE_DEVICE_NONE, INTERFACE_DEVICE_TEXT, INTERFACE_DEVICE_PCI.
	 * @param devicePathname The pathname of the device. For devices of type PCI, this will be something like
	 *        "/dev/astropci0". For devices of type TEXT, this should be a valid pathname to a (possibly
	 *        existing text file (i.e. ~dev/tmp/output.txt).
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the device could
	 * 	not be opened.
	 * @see #INTERFACE_DEVICE_NONE
	 * @see #INTERFACE_DEVICE_TEXT
	 * @see #INTERFACE_DEVICE_PCI
	 * @see #interfaceClose
	 * @see #CCD_Interface_Open
	 */
	public void interfaceOpen(int interfaceDevice,String devicePathname) throws CCDLibraryNativeException
	{
		CCD_Interface_Open(interfaceDevice,devicePathname);
	}

	/**
	 * Routine to close the interface opened with 
	 * <b>interfaceOpen</b>.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the device could
	 * 	not be closed.
	 * @see #interfaceOpen
	 * @see #CCD_Interface_Close
	 */
	public void interfaceClose() throws CCDLibraryNativeException
	{
		CCD_Interface_Close();
	}

	/**
	 * Routine to get an interface device number from a string representation of the device. Used for
	 * getting a device number from a string in a properties file.
	 * @param s A string representing a device number, one of:
	 * 	<ul>
	 * 	<li>INTERFACE_DEVICE_NONE,
	 * 	<li>INTERFACE_DEVICE_TEXT,
	 * 	<li>INTERFACE_DEVICE_PCI.
	 * 	</ul>.
	 * @return An interface device number, one of:
	 * 	<ul>
	 * 	<li>INTERFACE_DEVICE_NONE,
	 * 	<li>INTERFACE_DEVICE_TEXT,
	 * 	<li>INTERFACE_DEVICE_PCI.
	 * 	</ul>. 
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 */
	public static int interfaceDeviceFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("INTERFACE_DEVICE_NONE"))
			return INTERFACE_DEVICE_NONE;
		if(s.equals("INTERFACE_DEVICE_TEXT"))
			return INTERFACE_DEVICE_TEXT;
		if(s.equals("INTERFACE_DEVICE_PCI"))
			return INTERFACE_DEVICE_PCI;
		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","interfaceDeviceFromString",s);
	}

// ccd_setup.h
	/**
	 * This routine sets up the SDSU CCD Controller.
	 * This routine can be aborted with setupAbort.
	 * @param pciLoadType Where to load the PCI DSP program code from. Acceptable values are
	 * 	SETUP_LOAD_ROM, SETUP_LOAD_APPLICATION and SETUP_LOAD_FILENAME.
	 * @param pciFilename If pciLoadType is SETUP_LOAD_FILENAMEthis specifies which file to load from.
	 * @param timingLoadType Where to load the Timing application DSP code from. Acceptable values are
	 * 	SETUP_LOAD_ROM, SETUP_LOAD_APPLICATION and SETUP_LOAD_FILENAME.
	 * @param timingApplicationNumber If timingLoadType is SETUP_LOAD_APPLICATION this specifies which 
	 * 	application to load.
	 * @param timingFilename If timingLoadType is SETUP_LOAD_FILENAME this specifies which file to load from.
	 * @param utilityLoadType Where to load the Utility application DSP code from. Acceptable values are
	 * 	SETUP_LOAD_ROM, SETUP_LOAD_APPLICATION and SETUP_LOAD_FILENAME.
	 * @param utilityApplicationNumber If utilityLoadType is SETUP_LOAD_APPLICATION this specifies which 
	 * 	application to load.
	 * @param utilityFilename If utilityLoadType is SETUP_LOAD_FILENAME this specifies which file to load from.
	 * @param targetTemperature Specifies the target temperature the CCD is meant to run at, 
	 *        in degrees centigrade.
	 * @param gain The gain to use when reading out the CCD, one of : 
	 *        DSP_GAIN_ONE, DSP_GAIN_TWO, DSP_GAIN_FOUR, DSP_GAIN_NINE.
	 * @param gainSpeed A boolean, if true read out "fast", otherwise "slow".
	 * @param idle A boolean, if true puts CCD clocks in readout sequence, but not transferring any data, whenever 
	 *        an exposurecommand is not executing.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the setup failed.
	 * @see #setupAbort
	 * @see #CCD_Setup_Startup
	 * @see #SETUP_LOAD_ROM
	 * @see #SETUP_LOAD_APPLICATION
	 * @see #SETUP_LOAD_FILENAME
	 * @see #DSP_GAIN_ONE
	 * @see #DSP_GAIN_TWO
	 * @see #DSP_GAIN_FOUR
	 * @see #DSP_GAIN_NINE
	 */
	public void setup(int pciLoadType, String pciFilename,
		int timingLoadType,int timingApplicationNumber,String timingFilename,
		int utilityLoadType,int utilityApplicationNumber,String utilityFilename,
		double targetTemperature,int gain,boolean gainSpeed,boolean idle) throws CCDLibraryNativeException
	{
		CCD_Setup_Startup(pciLoadType,pciFilename,
				  timingLoadType,timingApplicationNumber,timingFilename,
				  utilityLoadType,utilityApplicationNumber,utilityFilename,
				  targetTemperature,gain,gainSpeed,idle);
	}

	/**
	 * Routine to shut down the SDSU CCD Controller board. This consists of:
	 * It then just remains to close the connection to the astro device driver.
	 * This routine can be aborted with setupAbort.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the shutdown failed.
	 * @see #setup
	 * @see #setupAbort
	 * @see #CCD_Setup_Shutdown
	 */
	public void setupShutdown() throws CCDLibraryNativeException
	{
		CCD_Setup_Shutdown();
	}

	/**
	 * Routine to setup dimension information in the controller. This needs to be setup before an exposure
	 * can take place. This routine must be called <b>after</b> the setup method.
	 * This routine can be aborted with setupAbort.
	 * @param ncols The number of columns in the image.
	 * @param nrows The number of rows in the image.
	 * @param nsbin The amount of binning applied to pixels in columns.This parameter will change internally
	 *	ncols.
	 * @param npbin The amount of binning applied to pixels in rows.This parameter will change internally
	 *	nrows.
	 * @param amplifier The amplifier to use when reading out CCD data. One of:
	 * 	<a href="#DSP_AMPLIFIER_LEFT">DSP_AMPLIFIER_LEFT</a>,
	 * 	<a href="#DSP_AMPLIFIER_RIGHT">DSP_AMPLIFIER_RIGHT</a> or
	 * 	<a href="#DSP_AMPLIFIER_BOTH">DSP_AMPLIFIER_BOTH</a>.
	 * @param deinterlaceSetting The algorithm to use for deinterlacing the resulting data. The data needs to be
	 * 	deinterlaced if the CCD is read out from multiple readouts.One of:
	 * 	<a href="#DSP_DEINTERLACE_SINGLE">DSP_DEINTERLACE_SINGLE</a>,
	 * 	<a href="#DSP_DEINTERLACE_FLIP">DSP_DEINTERLACE_FLIP</a>,
	 * 	<a href="#DSP_DEINTERLACE_SPLIT_PARALLEL">DSP_DEINTERLACE_SPLIT_PARALLEL</a>,
	 * 	<a href="#DSP_DEINTERLACE_SPLIT_SERIAL">DSP_DEINTERLACE_SPLIT_SERIAL</a>,
	 * 	<a href="#DSP_DEINTERLACE_SPLIT_QUAD">DSP_DEINTERLACE_SPLIT_QUAD</a>.
	 * @param windowFlags Flags describing which windows are in use. A bit-field combination of:
	 * 	<a href="#SETUP_WINDOW_ONE">SETUP_WINDOW_ONE</a>,
	 * 	<a href="#SETUP_WINDOW_TWO">SETUP_WINDOW_TWO</a>,
	 * 	<a href="#SETUP_WINDOW_THREE">SETUP_WINDOW_THREE</a> and
	 * 	<a href="#SETUP_WINDOW_FOUR">SETUP_WINDOW_FOUR</a>.
	 * @param windowList A list of CCDLibrarySetupWindow objects describing the window dimensions.
	 * 	This list should have <b>four</b> items in it.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the setup failed.
	 * @see #setup
	 * @see #setupAbort
	 * @see #CCD_Setup_Dimensions
	 */
	public void setupDimensions(int ncols,int nrows,int nsbin,int npbin,
		int amplifier,int deinterlaceSetting,
		int windowFlags,CCDLibrarySetupWindow windowList[]) throws CCDLibraryNativeException
	{
		CCD_Setup_Dimensions(ncols,nrows,nsbin,npbin,amplifier,deinterlaceSetting,windowFlags,windowList);
	}

	/**
	 * Method to perform a hardware data link test, to ensure we can communicate with each board in the
	 * controller. This routine can be aborted with setupAbort.
	 * @param testCount The number of times to test each board.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if the test failed.
	 * @see #setupAbort
	 * @see #CCD_Setup_Hardware_Test
	 */
	public void setupHardwareTest(int testCount) throws CCDLibraryNativeException
	{
		CCD_Setup_Hardware_Test(testCount);
	}

	/**
	 * Routine to abort a setup that is underway.
	 * @see #setup
	 * @see #CCD_Setup_Abort
	 */
	public void setupAbort()
	{
		CCD_Setup_Abort();
	}

	/**
	 * Routine to get the number of columns on the CCD chip last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the number of columns.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_NCols
	 */
	public int getNCols()
	{
		return CCD_Setup_Get_NCols();
	}

	/**
	 * Routine to get the number of rows on the CCD chip last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the number of rows.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_NRows
	 */
	public int getNRows()
	{
		return CCD_Setup_Get_NRows();
	}

	/**
	 * Routine to get the serial/column/X binning on the CCD chip last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the binning.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_NSBin
	 */
	public int getXBin()
	{
		return CCD_Setup_Get_NSBin();
	}

	/**
	 * Routine to get the parallel/row/Y binning on the CCD chip last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the binning.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_NPBin
	 */
	public int getYBin()
	{
		return CCD_Setup_Get_NPBin();
	}

	/**
	 * Routine to get the readout amplifier last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the amplifier.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_Amplifier
	 * @see #DSP_AMPLIFIER_LEFT
	 * @see #DSP_AMPLIFIER_RIGHT
	 * @see #DSP_AMPLIFIER_BOTH
	 */
	public int getAmplifier()
	{
		return CCD_Setup_Get_Amplifier();
	}

	/**
	 * Routine to get the deinterlace type last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns an integer representing the deinterlace type.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_DeInterlace_Type
	 * @see #DSP_DEINTERLACE_SINGLE
	 * @see #DSP_DEINTERLACE_FLIP
	 * @see #DSP_DEINTERLACE_SPLIT_PARALLEL
	 * @see #DSP_DEINTERLACE_SPLIT_SERIAL
	 * @see #DSP_DEINTERLACE_SPLIT_QUAD 
	 */
	public int getDeInterlaceType()
	{
		return CCD_Setup_Get_DeInterlace_Type();
	}

	/**
	 * Routine to return whether a setup operation has been sucessfully completed since the last controller
	 * reset.
	 * @return Returns true if a setup has been completed otherwise false.
	 * @see #setup
	 * @see #CCD_Setup_Get_Setup_Complete
	 */
	public boolean getSetupComplete()
	{
		return (boolean)CCD_Setup_Get_Setup_Complete();
	}

	/**
	 * Routine to detect whether a setup operation is underway.
	 * @return Returns true is a setup is in progress otherwise false.
	 * @see #setup
	 * @see #CCD_Setup_Get_Setup_In_Progress
	 */
	public boolean getSetupInProgress()
	{
		return (boolean)CCD_Setup_Get_Setup_In_Progress();
	}

	/**
	 * Returns the window flags passed into the last setup. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @return Returns the window flags, a bit-field containing which window data is active. This consists of:
	 * 	<a href="#SETUP_WINDOW_ONE">SETUP_WINDOW_ONE</a>,
	 * 	<a href="#SETUP_WINDOW_TWO">SETUP_WINDOW_TWO</a>,
	 * 	<a href="#SETUP_WINDOW_THREE">SETUP_WINDOW_THREE</a> and
	 * 	<a href="#SETUP_WINDOW_FOUR">SETUP_WINDOW_FOUR</a>.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_Window_Flags
	 */
	public int setupGetWindowFlags()
	{
		return CCD_Setup_Get_Window_Flags();
	}

	/**
	 * Returns a new instance of CCDLibrarySetupWindow describing the CCD camera window at index windowIndex.
	 * Note this window is only used if the corresponding bit in setupGetWindowFlags() is set.
	 * @param windowIndex Which window to get information for. This should be from zero to the 
	 * 	number of windows minus one. Four windows are supported by the hardware, hence the maximum value
	 * 	the index can take is three.
	 * @return A new instance of CCDLibrarySetupWindow with the window paramaters.
	 * @exception CCDLibraryNativeException Thrown if the windowIndex is out of range.
	 * @see #setupGetWindowFlags
	 * @see #CCD_Setup_Get_Window
	 */
	public CCDLibrarySetupWindow setupGetWindow(int windowIndex) throws CCDLibraryNativeException
	{
		return CCD_Setup_Get_Window(windowIndex);
	}

	/**
	 * Routine to get window width of the window last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @param index Which window to return.
	 * @return Returns an integer, the window height.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_Window_Width
	 */
	public int getWindowWidth(int index)
	{
		return CCD_Setup_Get_Window_Width(index);
	}

	/**
	 * Routine to get window height of the window last passed into setupDimensions. This value
	 * is got from the stored setup data, rather than querying the camera directly.
	 * @param index Which window to return.
	 * @return Returns an integer, the window height.
	 * @see #setupDimensions
	 * @see #CCD_Setup_Get_Window_Height
	 */
	public int getWindowHeight(int index)
	{
		return CCD_Setup_Get_Window_Height(index);
	}

	/**
	 * Routine to get the measured high voltage analogue voltage, in ADUs.
	 * @return Returns the number of ADU counts representing this voltage.
	 * @exception CCDLibraryFormatException Thrown if the adu value could not be retrieved.
	 * @see #CCD_Setup_Get_High_Voltage_Analogue_ADU
	 */
	public int getHighVoltageAnalogueADU() throws CCDLibraryNativeException
	{
		return CCD_Setup_Get_High_Voltage_Analogue_ADU();
	}

	/**
	 * Routine to get the measured low voltage analogue voltage, in ADUs.
	 * @return Returns the number of ADU counts representing this voltage.
	 * @exception CCDLibraryFormatException Thrown if the adu value could not be retrieved.
	 * @see #CCD_Setup_Get_Low_Voltage_Analogue_ADU
	 */
	public int getLowVoltageAnalogueADU() throws CCDLibraryNativeException
	{
		return CCD_Setup_Get_Low_Voltage_Analogue_ADU();
	}

	/**
	 * Routine to get the measured high voltage analogue voltage, in ADUs.
	 * @return Returns the number of ADU counts representing this voltage.
	 * @exception CCDLibraryFormatException Thrown if the adu value could not be retrieved.
	 * @see #CCD_Setup_Get_Minus_Low_Voltage_Analogue_ADU
	 */
	public int getMinusLowVoltageAnalogueADU() throws CCDLibraryNativeException
	{
		return CCD_Setup_Get_Minus_Low_Voltage_Analogue_ADU();
	}

	/**
	 * Routine to parse a setup load type string and return a setup load type to pass into <b>setup</b>. 
	 * @param s The string to parse.
	 * @return The load type, either SETUP_LOAD_ROM, SETUP_LOAD_APPLICATION or SETUP_LOAD_FILENAME.
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 * @see #SETUP_LOAD_ROM
	 * @see #SETUP_LOAD_APPLICATION
	 * @see #SETUP_LOAD_FILENAME
	 */
	public static int loadTypeFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("SETUP_LOAD_ROM"))
			return SETUP_LOAD_ROM;
		if(s.equals("SETUP_LOAD_APPLICATION"))
			return SETUP_LOAD_APPLICATION;
		if(s.equals("SETUP_LOAD_FILENAME"))
			return SETUP_LOAD_FILENAME;
		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","loadTypeFromString",s);
	}

// ccd_temperature.h
	/**
	 * Routine to get the current CCD temperature.
	 * @return The current temperature is returned, in degrees centigrade.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 * @see #CCD_Temperature_Get
	 */
	public double temperatureGet() throws CCDLibraryNativeException
	{
		return CCD_Temperature_Get();
	}

	/**
	 * Routine to set the temperature of the CCD.
	 * @param targetTemperature The temperature in degrees centigrade required for the CCD.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 * @see #CCD_Temperature_Set
	 */
	public void temperatureSet(double targetTemperature) throws CCDLibraryNativeException
	{
		CCD_Temperature_Set(targetTemperature);
	}

	/**
	 * Routine to get the Analogue to Digital count from the utility board temperature sensor, 
	 * a measure of the temperature of the utility board electronics.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 * @see #CCD_Temperature_Get_Utility_Board_ADU
	 */
	public int temperatureGetUtilityBoardADU() throws CCDLibraryNativeException
	{
		return CCD_Temperature_Get_Utility_Board_ADU();
	}

	/**
	 * Routine to get the current CCD heater Analogue to Digital count, a measure of how much
	 * heat is being put into the dewar to control the temperature.
	 * @exception CCDLibraryNativeException This method throws a CCDLibraryNativeException if it failed.
	 * @see #CCD_Temperature_Get_Heater_ADU
	 */
	public int temperatureGetHeaterADU() throws CCDLibraryNativeException
	{
		return CCD_Temperature_Get_Heater_ADU();
	}

// ccd_text.h
	/**
	 * Routine thats set the amount of information displayed when the text interface device
	 * is enabled.
	 * @param level An integer describing how much information is displayed. Can be one of:
	 *       TEXT_PRINT_LEVEL_COMMANDS, TEXT_PRINT_LEVEL_REPLIES, TEXT_PRINT_LEVEL_VALUES and TEXT_PRINT_LEVEL_ALL.
	 * @see #TEXT_PRINT_LEVEL_COMMANDS
	 * @see #TEXT_PRINT_LEVEL_REPLIES
	 * @see #TEXT_PRINT_LEVEL_VALUES
	 * @see #TEXT_PRINT_LEVEL_ALL
	 * @see #CCD_Text_Set_Print_Level
	 */
	public void setTextPrintLevel(int level)
	{
		CCD_Text_Set_Print_Level(level);
	}

	/**
	 * Routine to parse a string version of a text print level and to return
	 * the numeric value of that level, suitable for passing into setTextPrintLevel.
	 * @param s The string to parse.
	 * @return The printlevel number, one of:
	 * 	<ul>
	 * 	<li>TEXT_PRINT_LEVEL_COMMANDS,
	 * 	<li>TEXT_PRINT_LEVEL_REPLIES,
	 * 	<li>TEXT_PRINT_LEVEL_VALUES and
	 * 	<li>TEXT_PRINT_LEVEL_ALL.
	 * 	</ul>.
	 * @exception CCDLibraryFormatException If the string was not an accepted value an exception is thrown.
	 * @see #TEXT_PRINT_LEVEL_COMMANDS
	 * @see #TEXT_PRINT_LEVEL_REPLIES
	 * @see #TEXT_PRINT_LEVEL_VALUES
	 * @see #TEXT_PRINT_LEVEL_ALL
	 */
	public static int textPrintLevelFromString(String s) throws CCDLibraryFormatException
	{
		if(s.equals("TEXT_PRINT_LEVEL_COMMANDS"))
			return TEXT_PRINT_LEVEL_COMMANDS;
		if(s.equals("TEXT_PRINT_LEVEL_REPLIES"))
			return TEXT_PRINT_LEVEL_REPLIES;
		if(s.equals("TEXT_PRINT_LEVEL_VALUES"))
			return TEXT_PRINT_LEVEL_VALUES;
		if(s.equals("TEXT_PRINT_LEVEL_ALL"))
			return TEXT_PRINT_LEVEL_ALL;

		throw new CCDLibraryFormatException("ngat.frodospec.ccd.CCDLibrary","textPrintLevelFromString",s);
	}
}
 
//
// $Log: not supported by cvs2svn $
//
