// TestCCDRedatum.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ccd/test/TestCCDRedatum.java,v 1.1 2009-03-05 12:15:48 cjm Exp $
package ngat.frodospec.ccd.test;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.text.*;
import java.util.*;

import ngat.message.ISS_INST.*;
import ngat.phase2.*;
import ngat.util.*;
import ngat.util.logging.*;

import ngat.frodospec.ccd.*;

/**
 * This software tests opening and closing connections to the FrodoSpec CCDs - this is currently failing with
 * missing interface pointers from FrodoSpec itself.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class TestCCDRedatum
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: TestCCDRedatum.java,v 1.1 2009-03-05 12:15:48 cjm Exp $");
	/**
	 * Default config filename.
	 */
	public final static String DEFAULT_CONFIG_FILENAME = new String("./test_ccd_redatum.properties");
	/**
	 * The number of arms on FrodoSpec.
	 */
	public final static int FRODOSPEC_ARM_COUNT = 2;
	/**
	 * The default number of test open and closes to do.
	 */
	public final static int DEFAULT_TEST_COUNT = 2;
	/**
	 * Mapping from arm numbers to strings. The first index is "none", so there are three elements in the list,
	 * as RED_ARM is configured as 1 and BLUE_ARM is configured as 2.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public final static String ARM_STRING_LIST[] = {"none","red","blue"};
	/**
	 * The CCDLibrary class - used to interface with the SDSU CCD Controller.
	 */
	private CCDLibrary redCCD = null;
	/**
	 * The CCDLibrary class - used to interface with the SDSU CCD Controller.
	 */
	private CCDLibrary blueCCD = null;
	/**
	 * The logger.
	 */
	protected Logger logLogger = null;
	/**
	 * Filename of the config properties.
	 * @see #DEFAULT_CONFIG_FILENAME
	 */
	protected String configFilename = DEFAULT_CONFIG_FILENAME;
	/**
	 * Properties file containing config values.
	 */
	protected NGATProperties properties = null;
	/**
	 * The number of test open/closes of the interface to the CCD library to attempt.
	 */
	protected int testCount = DEFAULT_TEST_COUNT;

	/**
	 * This is the initialisation routine. 
	 * @see #redCCD
	 * @see #blueCCD
	 * @see #initLoggers
	 * @exception FileNotFoundException Thrown if the properties file does not exist.
	 * @exception IOException Thrown if an error occurs whilst reading the property file.
	 */
	protected void init() throws CCDLibraryFormatException,NumberFormatException,CCDLibraryNativeException,
				     FileNotFoundException,IOException
	{
		FileInputStream fileInputStream = null;

		// properties
		properties = new NGATProperties();
		fileInputStream = new FileInputStream(configFilename);
		properties.load(fileInputStream);
		fileInputStream.close();
	// Logging
		initLoggers();
	// create hardware control objects
		redCCD = new CCDLibrary();
		blueCCD = new CCDLibrary();
	}

	/**
	 * Initialise log handlers. 
	 * @see #logLogger
	 */
	protected void initLoggers()
	{
		Logger logger = null;
		LogHandler handler = null;
		SimpleLogFormatter formatter = null;

		// setup logLogger
		logLogger = LogManager.getLogger("log");
		logLogger.setLogLevel(Logging.VERBOSITY_VERY_VERBOSE);
		// setup console/simple handler/formatter
		formatter = new SimpleLogFormatter();
		formatter.setDateFormat(new SimpleDateFormat("yyyy/MM/dd HH:mm:ss.SSS z"));
		handler = new ConsoleLogHandler(formatter);
		handler.setLogLevel(Logging.ALL);
		logLogger.addHandler(handler);
		// setup ccd library logger
		logger = LogManager.getLogger("ngat.frodospec.ccd.CCDLibrary");
		logger.setLogLevel(Logging.VERBOSITY_VERY_VERBOSE);
		logger.addHandler(handler);
	}

	/**
	 * Method to open a connection to the FrodoSpec control objects and send initialisation control sequences
	 * to them. The SDSU CCD Controller(s) are configured. 
	 * This method assumes the <a href="#init">init</a> method has already been run to
	 * construct the redCCD, blueCCD,and <a href="#status">status</a> objects.
	 * It gets it's configuration from the FrodoSpec config file.
	 * <ul>
	 * <li>It gets it's configuration from the FrodoSpec config file.
	 * <li>The CCD librarys are initialised, the interfaces opened, and the controllers setup.
	 * </ul>
	 * @exception CCDLibraryFormatException Thrown if the configuration properties cannot be determined.
	 * @exception CCDLibraryNativeException Thrown if the call to open or setup the CCD controllers fails.
	 * @exception Exception Thrown if an error occurs.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see #status
	 * @see NGATProperties#getProperty
	 * @see NGATProperties#getPropertyInteger
	 * @see NGATProperties#getBoolean
	 * @see NGATProperties#getPropertyDouble
	 * @see ngat.frodospec.ccd.CCDLibrary#loadTypeFromString
	 * @see ngat.frodospec.ccd.CCDLibrary#initialise
	 * @see ngat.frodospec.ccd.CCDLibrary#setTextPrintLevel
	 * @see ngat.frodospec.ccd.CCDLibrary#interfaceOpen
	 * @see ngat.frodospec.ccd.CCDLibrary#setup
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #ARM_STRING_LIST
	 */
	public void startupCCDController() throws CCDLibraryFormatException, CCDLibraryNativeException, 
					       Exception
	{
		CCDLibrary ccd = null;
		int deviceNumber,textPrintLevel;
		int pciLoadType,timingLoadType,timingApplicationNumber,utilityLoadType,utilityApplicationNumber,gain;
		int startExposureClearTime,startExposureOffsetTime,readoutRemainingTime;
		boolean gainSpeed,idle,enable;
		double targetTemperature;
		String deviceString,pciFilename,timingFilename,utilityFilename,devicePathname;

		for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
		{
			// get the relevant configuration information from the FrodoSpec configuration file.
			// CCDLibraryFormatException is caught and re-thrown by this method.
			// Other exceptions (NumberFormatException) are not caught here, 
			// but by the calling method catch(Exception e)
			try
			{
				log(Logger.VERBOSITY_VERY_TERSE,":startupCCDController:Getting frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".enable");
				// ccd controller
				enable = properties.getBoolean("frodospec.ccd."+ARM_STRING_LIST[arm]+
								       ".enable");
				log(Logger.VERBOSITY_VERY_TERSE,":startupCCDController:Getting frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".device");
				deviceString = properties.getProperty("frodospec.ccd."+
								      ARM_STRING_LIST[arm]+".device");
				deviceNumber = CCDLibrary.interfaceDeviceFromString(deviceString);
				devicePathname = properties.getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
									".device.pathname");
				textPrintLevel = CCDLibrary.textPrintLevelFromString(
						 properties.getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
							   ".device.text.print_level"));
				pciLoadType = CCDLibrary.loadTypeFromString(properties.
				    getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
						".config.pci_load_type"));
				pciFilename = properties.getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
								 ".config.pci_filename");
				timingLoadType = CCDLibrary.loadTypeFromString(properties.
				    getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
						".config.timing_load_type"));
				timingApplicationNumber = properties.getInt("frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".config.timing_application_number");
				timingFilename = properties.getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
								    ".config.timing_filename");
				utilityLoadType = CCDLibrary.loadTypeFromString(properties.
				    getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
						".config.utility_load_type"));
				utilityApplicationNumber = properties.getInt("frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".config.utility_application_number");
				utilityFilename = properties.getProperty("frodospec.ccd."+ARM_STRING_LIST[arm]+
								     ".config.utility_filename");
				targetTemperature = properties.getDouble("frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".config.temperature.target");
				gain = CCDLibrary.dspGainFromString(properties.getProperty("frodospec.ccd."+
				    ARM_STRING_LIST[arm]+".config.gain"));
				gainSpeed = properties.getBoolean("frodospec.ccd."+ARM_STRING_LIST[arm]+
								      ".config.gain_speed");
				idle = properties.getBoolean("frodospec.ccd."+ARM_STRING_LIST[arm]+
								 ".config.idle");
				//startExposureClearTime = properties.getInt("frodospec.ccd."+
				//   ARM_STRING_LIST[arm]+".config.start_exposure_clear_time");
				//startExposureOffsetTime = properties.getInt("frodospec.ccd."+
				//    ARM_STRING_LIST[arm]+".config.start_exposure_offset_time");
				//readoutRemainingTime = properties.getInt("frodospec.ccd."+
				//    ARM_STRING_LIST[arm]+".config.readout_remaining_time");
			}
			catch(CCDLibraryFormatException e)
			{
				error(this.getClass().getName()+":startupCCDController:",e);
				throw e;
			}
			// configure ccd controller
			try
			{
				if(enable)
				{
					if(arm == FrodoSpecConfig.RED_ARM)
						ccd = redCCD;
					else if(arm == FrodoSpecConfig.BLUE_ARM)
						ccd = blueCCD;
					ccd.initialise();
					ccd.setTextPrintLevel(textPrintLevel);
					ccd.interfaceOpen(deviceNumber,devicePathname);
					ccd.setup(pciLoadType,pciFilename,
						  timingLoadType,timingApplicationNumber,timingFilename,
						  utilityLoadType,utilityApplicationNumber,utilityFilename,
						  targetTemperature,gain,gainSpeed,idle);
					// diddly not supported yet
					//libccd.CCDExposureSetStartExposureClearTime(startExposureClearTime);
					//libccd.CCDExposureSetStartExposureOffsetTime(startExposureOffsetTime);
					//libccd.CCDExposureSetReadoutRemainingTime(readoutRemainingTime);
				}// end if enable
			}
			catch (CCDLibraryNativeException e)
			{
				error(this.getClass().getName()+":startupCCDController:CCD:",e);
				throw e;
			}
		}// end for
	}

	/**
	 * Method to shut down the connection to the hardware controllers.
	 * <ul>
	 * <li>The CCD setup is shutdown (memory map), and the interface closed.
	 * </ul>
	 * @exception CCDLibraryNativeException Thrown if the device failed to shut down.
	 * @see #status
	 * @see #redCCD
	 * @see #blueCCD
	 * @see ngat.util.NGATProperties#getBoolean
	 * @see ngat.frodospec.ccd.CCDLibrary#setupShutdown
	 * @see ngat.frodospec.ccd.CCDLibrary#interfaceClose
	 */
	public void shutdownCCDController() throws CCDLibraryNativeException
	{
		boolean enable;

		enable = properties.getBoolean("frodospec.ccd.red.enable");
		if(enable)
		{
			redCCD.setupShutdown();
			redCCD.interfaceClose();
		}
		enable = properties.getBoolean("frodospec.ccd.blue.enable");
		if(enable)
		{
			blueCCD.setupShutdown();
			blueCCD.interfaceClose();
		}
	}
	/**
	 * Get ccd instance.
	 * @param arm Which arm. Either FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @return The ccd instance.
	 * @exception IllegalArgumentException Thrown if the arm is out of range.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see ngat.frodospec.ccd.CCDLibrary
	 */
	public CCDLibrary getCCD(int arm) throws IllegalArgumentException
	{
		if(arm == FrodoSpecConfig.RED_ARM)
			return redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			return blueCCD;
		else
			throw new IllegalArgumentException(this.getClass().getName()+":Illegal arm number "+arm+".");
	}

	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.out.
	 * @param level The level of logging this message belongs to.
	 * @param s The string to write.
	 * @see #logLogger
	 */
	public void log(int level,String s)
	{
		if(logLogger != null)
			logLogger.log(level,s);
		else
		{
			System.out.println(s);
		}
	}

	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.err.
	 * @param s The string to write.
	 * @see #logLogger
	 */
	public void error(String s)
	{
		if(logLogger != null)
			logLogger.log(Logger.VERBOSITY_VERY_TERSE,s);
		else
			System.err.println(s);
	}


	/**
	 * Routine to write the string to the relevant logger. If the relevant logger has not been
	 * created yet the error gets written to System.err.
	 * @param s The string to write.
	 * @param e An exception that caused the error to occur.
	 * @see #logLogger
	 */
	public void error(String s,Exception e)
	{
		if(logLogger != null)
		{
			logLogger.log(Logger.VERBOSITY_VERY_TERSE,s,e);
			logLogger.dumpStack(Logger.VERBOSITY_VERY_TERSE,e);
		}
		else
		{
			System.err.println(s);
			e.printStackTrace(System.err);
		}
	}

	/**
	 * This routine parses arguments.
	 * @see #help
	 * @see #configFilename
	 * @see #testCount
	 */
	private void parseArgs(String[] args)
	{
		for(int i = 0; i < args.length;i++)
		{
			if(args[i].equals("-c")||args[i].equals("-config_filename"))
			{
				if((i+1)< args.length)
				{
					configFilename = args[i+1];
					i++;
				}
				else
					System.err.println("-config_filename requires a valid filename.");
			}
			else if(args[i].equals("-h")||args[i].equals("-help"))
			{
				help();
				System.exit(0);
			}
			else if(args[i].equals("-tc")||args[i].equals("-test_count"))
			{
				if((i+1)< args.length)
				{
					testCount = Integer.parseInt(args[i+1]);
					i++;
				}
				else
					System.err.println("-test_count requires a number");
			}
			else
				System.out.println(this.getClass().getName()+":Option not supported:"+args[i]);
		}
	}

	/**
	 * Help message routine.
	 */
	private void help()
	{
		System.out.println(this.getClass().getName()+" Help:");
		System.out.println("Options are:");
		System.out.println("\t-c[onfig_filename] <filename> - Which config filename to use.");
		System.out.println("\t-tc|-test_count <number> - The number of open/close loops to try.");
	}

	/**
	 * The main routine. 
	 * @see #parseArgs
	 * @see #init
	 * @see #run
	 */
	public static void main(String[] args)
	{
		TestCCDRedatum o = null;
		int retval;

		retval = 0;
		o = new TestCCDRedatum();
		try
		{
			o.init();
		}
		catch(Exception e)
		{
			o.error("TestCCDRedatum:main",e);
			System.exit(1);
		}
		for(int i = 0; i < o.testCount; i++)
		{
			try
			{
				o.startupCCDController();
				Thread.sleep(1000);
				o.shutdownCCDController();
			}
			catch(Exception e)
			{
				o.error("TestCCDRedatum:main",e);
				retval = 1;
			}
		}
		System.exit(retval);
	}
}
//
// $Log: not supported by cvs2svn $
//
