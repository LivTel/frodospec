// FrodoSpecStatus.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecStatus.java,v 1.9 2013-08-01 11:24:29 eng Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;
import java.util.*;

import ngat.message.ISS_INST.*;
import ngat.phase2.*;
import ngat.util.PersistentUniqueInteger;
import ngat.util.FileUtilitiesNativeException;
import ngat.util.logging.FileLogHandler;

/**
 * This class holds status information for the FrodoSpec program.
 * @author Chris Mottram
 * @version $Revision: 1.9 $
 */
public class FrodoSpecStatus
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecStatus.java,v 1.9 2013-08-01 11:24:29 eng Exp $");
	/**
	 * Default filename containing network properties for frodospec.
	 */
	private final static String DEFAULT_NET_PROPERTY_FILE_NAME = "./frodospec.net.properties";
	/**
	 * Default filename containing properties for frodospec.
	 */
	private final static String DEFAULT_PROPERTY_FILE_NAME = "./frodospec.properties";

	/**
	 * The logging level. An absolute filter is used by the loggers:
	 * <ul>
	 * <li>0 is very terse.
	 * <li>5 is very verbose.
	 * </ul>
	 */
	private int logLevel = 0;
	/**
	 * The current thread that the FrodoSpec Control System is using to process the
	 * <a href="#currentCommand">currentCommand</a>. This does not get set for
	 * commands that can be sent while others are in operation, such as Abort and get status comamnds.
	 * This can be null when no command is currently being processed.
	 */
	private Thread currentThread[] = new Thread[3];
	/**
	 * The current command that the FrodoSpec Control System is working on. This does not get set for
	 * commands that can be sent while others are in operation, such as Abort and get status comamnds.
	 * This can be null when no command is currently being processed.
	 */
	private ISS_TO_INST currentCommand[] = new ISS_TO_INST[3];
	/**
	 * A list of properties held in the properties file. This contains configuration information in frodospec
	 * that needs to be changed irregularily.
	 */
	private Properties properties = null;
	/**
	 * The count of the number of exposures needed for the current command to be implemented.
	 */
	private int exposureCount[] = {0,0,0};
	/**
	 * The number of the current exposure being taken.
	 */
	private int exposureNumber[] = {0,0,0};
	/**
	 * The filename of the current exposure being taken (if any).
	 */
	private String exposureFilename[] = {null,null,null};
	/**
	 * The current unique config ID, held on disc over reboots.
	 * Incremented each time a new configuration is attained,
	 * and stored in the FITS header.
	 */
	private PersistentUniqueInteger configId[] = {null,null,null};
	/**
	 * The name of the ngat.phase2.FrodoSpecConfig object instance that was last used	
	 * to configure the FrodoSpec Camera (via an ngat.message.ISS_INST.CONFIG message).
	 * Used for the CONFNAME FITS keyword value. Now per-arm.
	 * Initialised to 'UNKNOWN', so that if we try to take a frame before configuring the FrodoSpec
	 * we get an error about setup not being complete, rather than an error about NULL FITS values.
	 */
	private String configName[] = {"UNKNOWN","UNKNOWN","UNKNOWN"};
	/**
	 * Boolean used to determine whether to do a calibration before taking an exposure.
	 * This value is set during the implementation of a ngat.message.ISS_INST.CONFIG message.
	 * The value is read during an exposure sequence (i.e. during an ngat.message.ISS_INST.MULTRUN command).
	 * The value is per-arm.
	 */
	private boolean configCalibrateBefore[] = {false,false,false};
	/**
	 * Boolean used to determine whether to do a calibration after taking an exposure.
	 * This value is set during the implementation of a ngat.message.ISS_INST.CONFIG message.
	 * The value is read during an exposure sequence (i.e. during an ngat.message.ISS_INST.MULTRUN command).
	 * The value is per-arm.
	 */
	private boolean configCalibrateAfter[] = {false,false,false};
	/**
	 * A list of Java longs holding the current time in milliseconds each time a 
	 * pause exposure command was initiated during an exposure.
	 */
	private Vector pauseTimeList = null;
	/**
	 * A list of Java longs holding the current time in milliseconds each time a 
	 * resume exposure command was initiated during an exposure.
	 */
	private Vector resumeTimeList = null;
	/**
	 * An object to be used as a synchronisation lock around ISS MOVE_FOLD calls.
	 * This stops two (arm) threads calling the ISS MOVE_FOLD command at the same time,
	 * resulting in the TCS throwing a "Command overriden by more recent request" TCS error <<<90041>>>.
	 */
	private Object foldLock = null;
	/**
	 * An object to be used as a synchronisation lock around ISS FOCUS_OFFSET calls.
	 * This stops two (arm) threads calling the ISS FOCUS_OFFSET command at the same time,
	 * resulting in the TCS throwing a "Command overriden by more recent request" TCS error <<<90041>>>.
	 */
	private Object focusOffsetLock = null;

	/**
	 * Default constructor. Initialises the pause and resume time lists, and the properties.
	 * Creates the foldLock and focusOffsetLock objects.
	 * @see #pauseTimeList
	 * @see #resumeTimeList
	 * @see #properties
	 * @see #foldLock
	 * @see #focusOffsetLock
	 */
	public FrodoSpecStatus()
	{
		pauseTimeList = new Vector();
		resumeTimeList = new Vector();
		properties = new Properties();
		foldLock = new Object();
		focusOffsetLock = new Object();
	}

	/**
	 * The load method for the class. This loads the property file from disc, using the specified
	 * filename. Any old properties are first cleared.
	 * The configId unique persistent integer is then initialised, using a filename stored in the properties.
	 * @param netFilename The filename of a Java property file containing network configuration for FrodoSpec.
	 * 	If netFilename is null, DEFAULT_NET_PROPERTY_FILE_NAME is used.
	 * @param filename The filename of a Java property file containing general configuration for FrodoSpec.
	 * 	If filename is null, DEFAULT_PROPERTY_FILE_NAME is used.
	 * @see #properties
	 * @see #initialiseConfigId
	 * @see #DEFAULT_NET_PROPERTY_FILE_NAME
	 * @see #DEFAULT_PROPERTY_FILE_NAME
	 * @exception FileNotFoundException Thrown if a configuration file is not found.
	 * @exception IOException Thrown if an IO error occurs whilst loading a configuration file.
	 */
	public void load(String netFilename,String filename) throws FileNotFoundException, IOException
	{
		FileInputStream fileInputStream = null;

	// clear old properties
		properties.clear();
	// network properties load
		if(netFilename == null)
			netFilename = DEFAULT_NET_PROPERTY_FILE_NAME;
		fileInputStream = new FileInputStream(netFilename);
		properties.load(fileInputStream);
		fileInputStream.close();
	// normal properties load
		if(filename == null)
			filename = DEFAULT_PROPERTY_FILE_NAME;
		fileInputStream = new FileInputStream(filename);
		properties.load(fileInputStream);
		fileInputStream.close();
	// initialise configId
		initialiseConfigId();
	}

	/**
	 * The reload method for the class. This reloads the specified property files from disc.
	 * The current properties are not cleared, as network properties are not re-loaded, as this would
	 * involve resetting up the server connection thread which may be in use. If properties have been
	 * deleted from the loaded files, reload does not clear these properties. Any new properties or
	 * ones where the values have changed will change.
	 * The configId unique persistent integer is then initialised, using a filename stored in the properties.
	 * @param filename The filename of a Java property file containing general configuration for FrodoSpec.
	 * 	If filename is null, DEFAULT_PROPERTY_FILE_NAME is used.
	 * @see #properties
	 * @see #initialiseConfigId
	 * @see #DEFAULT_NET_PROPERTY_FILE_NAME
	 * @see #DEFAULT_PROPERTY_FILE_NAME
	 * @exception FileNotFoundException Thrown if a configuration file is not found.
	 * @exception IOException Thrown if an IO error occurs whilst loading a configuration file.
	 */
	public void reload(String filename) throws FileNotFoundException,IOException
	{
		FileInputStream fileInputStream = null;

	// don't clear old properties, the network properties are not re-loaded
	// normal properties load
		if(filename == null)
			filename = DEFAULT_PROPERTY_FILE_NAME;
		fileInputStream = new FileInputStream(filename);
		properties.load(fileInputStream);
		fileInputStream.close();
	// initialise configId
		initialiseConfigId();
	}

	/**
	 * Set the logging level for FrodoSpec.
	 * @param level The level of logging.
	 */
	public synchronized void setLogLevel(int level)
	{
		logLevel = level;
	}

	/**
	 * Get the logging level for FrodoSpec.
	 * @return The current log level.
	 */	
	public synchronized int getLogLevel()
	{
		return logLevel;
	}

	/**
	 * Set the command that is currently executing.
	 * @param command The command that is currently executing.
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #getArmFromCommand
	 */
	public synchronized void setCurrentCommand(ISS_TO_INST command)
	{
		int arm;

		// derive arm from command
		arm = getArmFromCommand(command);
		currentCommand[arm] = command;
	}

	/**
	 * Clear the command that is currently executing.
	 * @param command The command that has finished executing.
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #getArmFromCommand
	 */
	public synchronized void clearCurrentCommand(ISS_TO_INST command)
	{
		int arm;

		// derive arm from command
		arm = getArmFromCommand(command);
		currentCommand[arm] = null;
	}

	/**
	 * Get the command that is currently executing on the arm specified by command.
	 * @param command A command specifying an arm (or NO_ARM).
	 * @return The command currently being processed on the specified arm.
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #getArmFromCommand
	 */
	public synchronized ISS_TO_INST getCurrentCommand(ISS_TO_INST command)
	{
		int arm;

		// derive arm from command
		arm = getArmFromCommand(command);
		return currentCommand[arm];
	}

	/**
	 * Get the the command FrodoSpec is currently processing.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @return The command currently being processed on the specified arm.
	 */
	public synchronized ISS_TO_INST getCurrentCommand(int arm)
	{
		return currentCommand[arm];
	}

	/**
	 * This routine determines whether a command can be run given the current status of FrodoSpec.
	 * If the FrodoSpec is currently not running a command on the specified arm the command can be run.
	 * If the command is getting status we can generally run this whilst other things are going on.
	 * If the command is a reboot or abort command that tells FrodoSpec to stop what it is going this is
	 * generally allowed to be run, otherwise we couldn't stop execution of exposures mid-exposure.
	 * @param command The command we want to run, used to determine the arm.
	 * @return Whether the command can be run given the current status of the system.
	 */
	// This is no longer true due to LampController: lamp locks should take care of this internally:
	// * ARCS are more complicated, we can do an ARC if nothing is exposing on the other arm, or the ARC
	// * on the other arm has the same lamp string as this command.
	public synchronized boolean commandCanBeRun(ISS_TO_INST command)
	{
		int arm;
		String lampString = null;

		// get arm from command
		// This code is similar to that in getArmFromCommand
		//if(command instanceof FRODOSPEC_CALIBRATE)
		//{
		//	FRODOSPEC_CALIBRATE calibrateCommand = null;
		//	calibrateCommand = (FRODOSPEC_CALIBRATE)command;
		//	arm = calibrateCommand.getArm();
		//	if(command instanceof FRODOSPEC_ARC)
		//	{
		//		FRODOSPEC_ARC arcCommand = null;
		//		arcCommand = (FRODOSPEC_ARC)command;
		//		lampString = arcCommand.getLamp();
		//	}
		//}
		//else if (command instanceof FRODOSPEC_EXPOSE)
		//{
		//	FRODOSPEC_EXPOSE exposeCommand = null;
		//	exposeCommand = (FRODOSPEC_EXPOSE)command;
		//	arm = exposeCommand.getArm();
		//}
		//else if (command instanceof CONFIG)
		//{
		//	CONFIG configCommand = null;
		//	InstrumentConfig instrumentConfig = null;

		//	configCommand = (CONFIG)command;
		//	instrumentConfig = configCommand.getConfig();
		//	if(instrumentConfig instanceof FrodoSpecConfig)
		//	{
		//		FrodoSpecConfig frodospecConfig = null;

		//              frodospecConfig = (FrodoSpecConfig)instrumentConfig;
		//		arm = frodospecConfig.getArm();
		//	}
		//	else // a little bit dodgy, but the CONFIG should just return an error when run anyway.
		//		arm = FrodoSpecConfig.NO_ARM;
		//}
		//else
		//	arm = FrodoSpecConfig.NO_ARM;
		// This is no longer true due to LampController: lamp locks should take care of this internally:
		// arcs are more complicated
		//if(lampString != null)
		//{
			// for each arm
		//	for(int cci = FrodoSpecConfig.RED_ARM; cci <= FrodoSpecConfig.BLUE_ARM; cci++)
		//	{
				// we are doing something on this arm
		//		if(currentCommand[cci] != null)
		//		{
					// we can't do an arc if an expose is underway on either arm
		//			if(currentCommand[cci] instanceof FRODOSPEC_EXPOSE)
		//				return false;
					// we are doing an arc on this [cci] arm
		//			if(currentCommand[cci] instanceof FRODOSPEC_ARC)
		//			{
		//				FRODOSPEC_ARC arcCommand = null;
		//				arcCommand = (FRODOSPEC_ARC)command;
						// if we are already doing an ARC command the lamps must match
		//				if(lampString.equals(arcCommand.getLamp()) == false)
		//					return false;
		//			}
					// we don't care if a CONFIG is underway on the other arm
		//		}
		//	}
		//}
		arm = getArmFromCommand(command);
		if(currentCommand[arm] == null)
			return true;
		if(command instanceof INTERRUPT)
			return true;
		return false;
	}

	/**
	 * Set the thread that is currently executing the <a href="#currentCommand">currentCommand</a>.
	 * @param command The command to be executed in the thread. This is used to extract the arm to be used.
	 * @param thread The thread that is currently executing (per-arm).
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #currentThread
	 * @see #getArmFromCommand
	 */
	public synchronized void setCurrentThread(ISS_TO_INST command,Thread thread)
	{
		int arm;

		// derive arm from command
		arm = getArmFromCommand(command);
		currentThread[arm] = thread;
	}

	/**
	 * Get the the thread FrodoSpec is currently executing to process the 
	 * <a href="#currentCommand">currentCommand</a>.
	 * @return The thread currently being executed on the specified arm.
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see #currentThread
	 */
	public synchronized Thread getCurrentThread(int arm)
	{
		return currentThread[arm];
	}

	/**
	 * Try to extract arm information from the specified command.
	 * If the command is a subclass of FRODOSPEC_CALIBRATE or FRODOSPEC_EXPOSE the arm can be extracted.
	 * If it is a CONFIG the arm can be extracted from from enclosed instrument config.
	 * @param command A command specifying an arm (or NO_ARM).
	 * @return The extracted arm number.
	 * @see ngat.phase2.FrodoSpecConfig#NO_ARM
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public int getArmFromCommand(ISS_TO_INST command)
	{
		int arm;

		if(command instanceof FRODOSPEC_CALIBRATE)
		{
			FRODOSPEC_CALIBRATE calibrateCommand = null;
			calibrateCommand = (FRODOSPEC_CALIBRATE)command;
			arm = calibrateCommand.getArm();
		}
		else if (command instanceof FRODOSPEC_EXPOSE)
		{
			FRODOSPEC_EXPOSE exposeCommand = null;
			exposeCommand = (FRODOSPEC_EXPOSE)command;
			arm = exposeCommand.getArm();
		}
		else if (command instanceof FRODOSPEC_SETUP)
		{
			FRODOSPEC_SETUP setupCommand = null;
			setupCommand = (FRODOSPEC_SETUP)command;
			arm = setupCommand.getArm();
		}
		else if (command instanceof CONFIG)
		{
			CONFIG configCommand = null;
			InstrumentConfig instrumentConfig = null;

			configCommand = (CONFIG)command;
			instrumentConfig = configCommand.getConfig();
			if(instrumentConfig instanceof FrodoSpecConfig)
			{
				FrodoSpecConfig frodospecConfig = null;

				frodospecConfig = (FrodoSpecConfig)instrumentConfig;
				arm = frodospecConfig.getArm();
			}
			else // a little bit dodgy, but the CONFIG should just return an error when run anyway.
				arm = FrodoSpecConfig.NO_ARM;
		}
		else
			arm = FrodoSpecConfig.NO_ARM;
		return arm;
	}

	/**
	 * Set the number of exposures needed to complete the current command implementation.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @param c The total number of exposures needed.
	 * @see #exposureCount
	 */
	public synchronized void setExposureCount(int arm,int c)
	{
		exposureCount[arm] = c;
	}

	/**
	 * Get the number of exposures needed to complete the current command implementation.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @return Returns the number of exposures needed.
	 * @see #exposureCount
	 */
	public synchronized int getExposureCount(int arm)
	{
		return exposureCount[arm];
	}

	/**
	 * Set the current exposure number the current command implementation is on.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @param n The current exposure number.
	 * @see #exposureNumber
	 */
	public synchronized void setExposureNumber(int arm,int n)
	{
		exposureNumber[arm] = n;
	}

	/**
	 * Get the current exposure number the current command implementation is on.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @return Returns the current exposure number.
	 * @see #exposureNumber
	 */
	public synchronized int getExposureNumber(int arm)
	{
		return exposureNumber[arm];
	}

	/**
	 * Set the current exposure filename being taken.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @param f The current filename.
	 * @see #exposureFilename
	 */
	public synchronized void setExposureFilename(int arm,String f)
	{
		exposureFilename[arm] = f;
	}

	/**
	 * Get the current exposure filename.
	 * @param arm Which arm to get the current command for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @return Returns the current exposure filename.
	 * @see #exposureFilename
	 */
	public synchronized String getExposureFilename(int arm)
	{
		return exposureFilename[arm];
	}

	/**
	 * Method to change (increment) the unique ID number of the last
	 * ngat.phase2.FrodoSpecConfig instance to successfully configure the FrodoSpec camera.
	 * This is done by calling <i>configId[arm].increment()</i>.
	 * @param arm Which arm to increment the config Id for, one of RED_ARM or BLUE_ARM.
	 * @see #configId
	 * @see ngat.util.PersistentUniqueInteger#increment
	 * @exception FileUtilitiesNativeException Thrown if <i>PersistentUniqueInteger.increment()</i> fails.
	 * @exception NumberFormatException Thrown if <i>PersistentUniqueInteger.increment()</i> fails.
	 * @exception Exception Thrown if <i>PersistentUniqueInteger.increment()</i> fails.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public synchronized void incConfigId(int arm) throws FileUtilitiesNativeException,
		NumberFormatException, Exception
	{
		configId[arm].increment();
	}

	/**
	 * Method to get the unique config ID number of the last
	 * ngat.phase2.FrodoSpecConfig instance to successfully configure the FrodoSpec camera.
	 * @param arm Which arm to get the config Id for, one of NO_ARM,RED_ARM, BLUE_ARM.
	 * @return The unique config ID number.
	 * This is done by calling <i>configId[arm].get()</i>.
	 * @see #configId
	 * @see ngat.util.PersistentUniqueInteger#get
	 * @exception FileUtilitiesNativeException Thrown if <i>PersistentUniqueInteger.get()</i> fails.
	 * @exception NumberFormatException Thrown if <i>PersistentUniqueInteger.get()</i> fails.
	 * @exception Exception Thrown if <i>PersistentUniqueInteger.get()</i> fails.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	public synchronized int getConfigId(int arm) throws FileUtilitiesNativeException,
		NumberFormatException, Exception
	{
		return configId[arm].get();
	}

	/**
	 * Method to set our reference to the string identifier of the last
	 * ngat.phase2.FrodoSpecConfig instance to successfully configure the FrodoSpec camera.
	 * @param arm Which arm has this name.
	 * @param s The string from the configuration object instance.
	 * @see #configName
	 */
	public synchronized void setConfigName(int arm,String s)
	{
		configName[arm] = s;
	}

	/**
	 * Mehtod to get the string identifier of the last
	 * ngat.phase2.FrodoSpecConfig instance to successfully configure the FrodoSpec camera.
	 * @param arm Which arm to get the last config name for.
	 * @return The string identifier, or null if the FrodoSpec camera has not been configured
	 * 	since the FrodoSpec started.
	 * @see #configName
	 */
	public synchronized String getConfigName(int arm)
	{
		return configName[arm];
	}

	/**
	 * Method to set whether to take a calibration frame before an exposure is taken.
	 * @param arm Which arm.
	 * @param b The boolean, if true a calibration is done before an exposure, if false it is not.
	 * @see #configCalibrateBefore
	 */
	public synchronized void setConfigCalibrateBefore(int arm,boolean b)
	{
		configCalibrateBefore[arm] = b;
	}

	/**
	 * Method to get whether to take a calibration frame before an exposure is taken.
	 * @param arm Which arm.
	 * @return A boolean, true if a calibration frame is to be done before an exposure.
	 * @see #configCalibrateBefore
	 */
	public synchronized boolean getConfigCalibrateBefore(int arm)
	{
		return configCalibrateBefore[arm];
	}

	/**
	 * Method to set whether to take a calibration frame after an exposure is taken.
	 * @param arm Which arm.
	 * @param b The boolean, if true a calibration is done after an exposure, if false it is not.
	 * @see #configCalibrateAfter
	 */
	public synchronized void setConfigCalibrateAfter(int arm,boolean b)
	{
		configCalibrateAfter[arm] = b;
	}

	/**
	 * Method to get whether to take a calibration frame after an exposure is taken.
	 * @param arm Which arm.
	 * @return A boolean, true if a calibration frame is to be done after an exposure.
	 * @see #configCalibrateAfter
	 */
	public synchronized boolean getConfigCalibrateAfter(int arm)
	{
		return configCalibrateAfter[arm];
	}

	/**
	 * Method to add a time a pause was initiated to the list. The time is put into a Long object.
	 * @param time A time, in milliseconds since the 1st January 1970, that a pause was initiated.
	 * @see #pauseTimeList
	 */
	public void addPauseTime(long time)
	{
		pauseTimeList.add(new Long(time));
	}

	/**
	 * Method to add a time a resume was initiated to the list. The time is put into a Long object.
	 * @param time A time, in milliseconds since the 1st January 1970, that a resume was initiated.
	 * @see #resumeTimeList
	 */
	public void addResumeTime(long time)
	{
		resumeTimeList.add(new Long(time));
	}

	/**
	 * Method to clear the pause and resume times held in the status. The <b>clear</b> methods of the
	 * two lists are called.
	 * @see #pauseTimeList
	 * @see #resumeTimeList
	 */
	public void clearPauseResumeTimes()
	{
		pauseTimeList.clear();
		resumeTimeList.clear();
	}

	/**
	 * Method to get the number of times in the Pause time list.
	 * @return The number of times in the Pause time list.
	 * @see #pauseTimeList
	 */
	public int getPauseTimeListSize()
	{
		return pauseTimeList.size();
	}

	/**
	 * Method to get one of the pause times held in the pause time list. The time is returned 
	 * as a Long object, the value of which is the number of milliseconds after 1st January 1970 the
	 * pause was initiated.
	 * @param index The index in the pause time list to get the time for.
	 * @return A Long object, the contents of which are the milliseconds since 1st January 1970 the
	 * 	(index +1)th pause occured during the last exposure.
	 * @exception ArrayIndexOutOfBoundsException Thrown if the index is not in the list.
	 */
	public Long getPauseTime(int index) throws ArrayIndexOutOfBoundsException
	{
		return (Long)(pauseTimeList.get(index));
	}

	/**
	 * Method to get the number of times in the resume time list.
	 * @return The number of times in the resume time list.
	 * @see #resumeTimeList
	 */
	public int getResumeTimeListSize()
	{
		return resumeTimeList.size();
	}

	/**
	 * Method to get one of the resume times held in the resume time list. The time is returned 
	 * as a Long object, the value of which is the number of milliseconds after 1st January 1970 the
	 * resume was initiated.
	 * @param index The index in the resume time list to get the time for.
	 * @return A Long object, the contents of which are the milliseconds since 1st January 1970 the
	 * 	(index +1)th resume occured during the last exposure.
	 * @exception ArrayIndexOutOfBoundsException Thrown if the index is not in the list.
	 */
	public Long getResumeTime(int index) throws ArrayIndexOutOfBoundsException
	{
		return (Long)(resumeTimeList.get(index));
	}

	/**
	 * Method to return whether the loaded properties contain the specified keyword.
	 * Calls the proprties object containsKey method. Note assumes the properties object has been initialised.
	 * @param p The property key we wish to test exists.
	 * @return The method returnd true if the specified key is a key in out list of properties,
	 *         otherwise it returns false.
	 * @see #properties
	 */
	public boolean propertyContainsKey(String p)
	{
		return properties.containsKey(p);
	}

	/**
	 * Routine to get a properties value, given a key. Just calls the properties object getProperty routine.
	 * @param p The property key we want the value for.
	 * @return The properties value, as a string object.
	 * @see #properties
	 */
	public String getProperty(String p)
	{
		return properties.getProperty(p);
	}

	/**
	 * Routine to get a properties value, given a key. The value must be a valid integer, else a 
	 * NumberFormatException is thrown.
	 * @param p The property key we want the value for.
	 * @return The properties value, as an integer.
	 * @exception NumberFormatException If the properties value string is not a valid integer, this
	 * 	exception will be thrown when the Integer.parseInt routine is called.
	 * @see #properties
	 */
	public int getPropertyInteger(String p) throws NumberFormatException
	{
		String valueString = null;
		int returnValue = 0;

		valueString = properties.getProperty(p);
		try
		{
			returnValue = Integer.parseInt(valueString);
		}
		catch(NumberFormatException e)
		{
			// re-throw exception with more information e.g. keyword
			throw new NumberFormatException(this.getClass().getName()+":getPropertyInteger:keyword:"+
				p+":valueString:"+valueString);
		}
		return returnValue;
	}

	/**
	 * Routine to get a properties value, given a key. The value must be a valid long, else a 
	 * NumberFormatException is thrown.
	 * @param p The property key we want the value for.
	 * @return The properties value, as a long.
	 * @exception NumberFormatException If the properties value string is not a valid long, this
	 * 	exception will be thrown when the Long.parseLong routine is called.
	 * @see #properties
	 */
	public long getPropertyLong(String p) throws NumberFormatException
	{
		String valueString = null;
		long returnValue = 0;

		valueString = properties.getProperty(p);
		try
		{
			returnValue = Long.parseLong(valueString);
		}
		catch(NumberFormatException e)
		{
			// re-throw exception with more information e.g. keyword
			throw new NumberFormatException(this.getClass().getName()+":getPropertyLong:keyword:"+
				p+":valueString:"+valueString);
		}
		return returnValue;
	}

	/**
	 * Routine to get a properties value, given a key. The value must be a valid double, else a 
	 * NumberFormatException is thrown.
	 * @param p The property key we want the value for.
	 * @return The properties value, as an double.
	 * @exception NumberFormatException If the properties value string is not a valid double, this
	 * 	exception will be thrown when the Double.valueOf routine is called.
	 * @see #properties
	 */
	public double getPropertyDouble(String p) throws NumberFormatException
	{
		String valueString = null;
		Double returnValue = null;

		valueString = properties.getProperty(p);
		try
		{
			returnValue = Double.valueOf(valueString);
		}
		catch(NumberFormatException e)
		{
			// re-throw exception with more information e.g. keyword
			throw new NumberFormatException(this.getClass().getName()+":getPropertyDouble:keyword:"+
				p+":valueString:"+valueString);
		}
		return returnValue.doubleValue();
	}

    /** Set a property.
     * @param key The keyword to set.
     * @param value The value to set.
     */
    public void setProperty(String key, String value) {
	
	properties.put(key, value);

    }


	/**
	 * Routine to get a properties value, given a key. The value must be a valid float, else a 
	 * NumberFormatException is thrown.
	 * @param p The property key we want the value for.
	 * @return The properties value, as a float.
	 * @exception NumberFormatException If the properties value string is not a valid float, this
	 * 	exception will be thrown.
	 * @see #properties
	 */
	public float getPropertyFloat(String p) throws NumberFormatException
	{
		String valueString = null;
		Float returnValue = null;

		valueString = properties.getProperty(p);
		try
		{
			returnValue = Float.valueOf(valueString);
		}
		catch(NumberFormatException e)
		{
			// re-throw exception with more information e.g. keyword
			throw new NumberFormatException(this.getClass().getName()+":getPropertyFloat:keyword:"+
				p+":valueString:"+valueString);
		}
		return returnValue.floatValue();
	}

	/**
	 * Routine to get a properties boolean value, given a key. The properties value should be either 
	 * "true" or "false".
	 * Boolean.valueOf is used to convert the string to a boolean value.
	 * @param p The property key we want the boolean value for.
	 * @return The properties value, as an boolean.
	 * @exception NullPointerException If the properties value string is null, this
	 * 	exception will be thrown.
	 * @see #properties
	 */
	public boolean getPropertyBoolean(String p) throws NullPointerException
	{
		String valueString = null;
		Boolean b = null;

		valueString = properties.getProperty(p);
		if(valueString == null)
		{
			throw new NullPointerException(this.getClass().getName()+":getPropertyBoolean:keyword:"+
				p+":Value was null.");
		}
		b = Boolean.valueOf(valueString);
		return b.booleanValue();
	}

	/**
	 * Routine to get a properties character value, given a key. The properties value should be a 1 letter string.
	 * @param p The property key we want the character value for.
	 * @return The properties value, as a character.
	 * @exception NullPointerException If the properties value string is null, this
	 * 	exception will be thrown.
	 * @exception Exception Thrown if the properties value string is not of length 1.
	 * @see #properties
	 */
	public char getPropertyChar(String p) throws NullPointerException, Exception
	{
		String valueString = null;
		char ch;

		valueString = properties.getProperty(p);
		if(valueString == null)
		{
			throw new NullPointerException(this.getClass().getName()+":getPropertyChar:keyword:"+
				p+":Value was null.");
		}
		if(valueString.length() != 1)
		{
			throw new Exception(this.getClass().getName()+":getPropertyChar:keyword:"+
					    p+":Value not of length 1, had length "+valueString.length());
		}
		ch = valueString.charAt(0);
		return ch;
	}

	/**
	 * Routine to get an integer representing a ngat.util.logging.FileLogHandler time period.
	 * The value of the specified property should contain either:'HOURLY_ROTATION', 'DAILY_ROTATION' or
	 * 'WEEKLY_ROTATION'.
	 * @param p The property key we want the time period value for.
	 * @return The properties value, as an FileLogHandler time period (actually an integer).
	 * @exception NullPointerException If the properties value string is null an exception is thrown.
	 * @exception IllegalArgumentException If the properties value string is not a valid time period,
	 *            an exception is thrown.
	 * @see #properties
	 */
	public int getPropertyLogHandlerTimePeriod(String p) throws NullPointerException, IllegalArgumentException
	{
		String valueString = null;
		int timePeriod = 0;
 
		valueString = properties.getProperty(p);
		if(valueString == null)
		{
			throw new NullPointerException(this.getClass().getName()+
						       ":getPropertyLogHandlerTimePeriod:keyword:"+
						       p+":Value was null.");
		}
		if(valueString.equals("HOURLY_ROTATION"))
			timePeriod = FileLogHandler.HOURLY_ROTATION;
		else if(valueString.equals("DAILY_ROTATION"))
			timePeriod = FileLogHandler.DAILY_ROTATION;
		else if(valueString.equals("WEEKLY_ROTATION"))
			timePeriod = FileLogHandler.WEEKLY_ROTATION;
		else
		{
			throw new IllegalArgumentException(this.getClass().getName()+
							   ":getPropertyLogHandlerTimePeriod:keyword:"+
							   p+":Illegal value:"+valueString+".");
		}
		return timePeriod;
	}

	/**
	 * Method to get the number of columns to tell the SDSU controller to read out, for a specified
	 * binning factor. The binning factor is needed for DUAL readout amplifiers, which need different
	 * dimensions to be sent to the controller depending on the binning setting, to stop bias strips appearing
	 * in the centre of the image, or missing central columns.
	 * This information is stored in the FrodoSpec property file, 
	 * under the 'frodospec.config.ncols.<binning factor>' property.
	 * @param xbin The X binning factor to get the number of columns for.
	 * @return An integer, the number of columns to configure the controller with.
	 * @exception NumberFormatException Thrown if the property cannot be found, or parsed into a valid int.
	 * @see #getPropertyInteger
	 */
	public int getNumberColumns(int xbin) throws NumberFormatException
	{
		return getPropertyInteger("frodospec.config.ncols."+xbin);
	}

	/**
	 * Method to get the number of rows to tell the SDSU controller to read out, for a specified
	 * binning factor. The binning factor is not really needed when computing nrows, but is retained for 
	 * completness against ncols configuration.
	 * This information is stored in the FrodoSpec property file, 
	 * under the 'frodospec.config.nrows.<binning factor>' property.
	 * @param ybin The Y binning factor to get the number of rows for.
	 * @return An integer, the number of rows to configure the controller with.
	 * @exception NumberFormatException Thrown if the property cannot be found, or parsed into a valid int.
	 * @see #getPropertyInteger
	 */
	public int getNumberRows(int ybin) throws NumberFormatException
	{
		return getPropertyInteger("frodospec.config.nrows."+ybin);
	}

	/**
	 * Method to get the thread priority to run the server thread at.
	 * The value is retrieved from the <b>frodospec.thread.priority.server</b> property.
	 * If this fails the default FRODOSPEC_DEFAULT_THREAD_PRIORITY_SERVER is returned.
	 * @return A valid thread priority between threads MIN_PRIORITY and MAX_PRIORITY.
	 * @see FrodoSpecConstants#FRODOSPEC_DEFAULT_THREAD_PRIORITY_SERVER
	 */
	public int getThreadPriorityServer()
	{
		int retval;

		try
		{
			retval = getPropertyInteger("frodospec.thread.priority.server");
			if(retval < Thread.MIN_PRIORITY)
				retval = Thread.MIN_PRIORITY;
			if(retval > Thread.MAX_PRIORITY)
				retval = Thread.MAX_PRIORITY;
		}
		catch(NumberFormatException e)
		{
			retval = FrodoSpecConstants.FRODOSPEC_DEFAULT_THREAD_PRIORITY_SERVER;
		}
		return retval;
	}

	/**
	 * Method to get the thread priority to run interrupt threads at.
	 * The value is retrieved from the <b>frodospec.thread.priority.interrupt</b> property.
	 * If this fails the default FRODOSPEC_DEFAULT_THREAD_PRIORITY_INTERRUPT is returned.
	 * @return A valid thread priority between threads MIN_PRIORITY and MAX_PRIORITY.
	 * @see FrodoSpecConstants#FRODOSPEC_DEFAULT_THREAD_PRIORITY_INTERRUPT
	 */
	public int getThreadPriorityInterrupt()
	{
		int retval;

		try
		{
			retval = getPropertyInteger("frodospec.thread.priority.interrupt");
			if(retval < Thread.MIN_PRIORITY)
				retval = Thread.MIN_PRIORITY;
			if(retval > Thread.MAX_PRIORITY)
				retval = Thread.MAX_PRIORITY;
		}
		catch(NumberFormatException e)
		{
			retval = FrodoSpecConstants.FRODOSPEC_DEFAULT_THREAD_PRIORITY_INTERRUPT;
		}
		return retval;
	}

	/**
	 * Method to get the thread priority to run normal threads at.
	 * The value is retrieved from the <b>frodospec.thread.priority.normal</b> property.
	 * If this fails the default FRODOSPEC_DEFAULT_THREAD_PRIORITY_NORMAL is returned.
	 * @return A valid thread priority between threads MIN_PRIORITY and MAX_PRIORITY.
	 * @see FrodoSpecConstants#FRODOSPEC_DEFAULT_THREAD_PRIORITY_NORMAL
	 */
	public int getThreadPriorityNormal()
	{
		int retval;

		try
		{
			retval = getPropertyInteger("frodospec.thread.priority.normal");
			if(retval < Thread.MIN_PRIORITY)
				retval = Thread.MIN_PRIORITY;
			if(retval > Thread.MAX_PRIORITY)
				retval = Thread.MAX_PRIORITY;
		}
		catch(NumberFormatException e)
		{
			retval = FrodoSpecConstants.FRODOSPEC_DEFAULT_THREAD_PRIORITY_NORMAL;
		}
		return retval;
	}

	/**
	 * Method to get the thread priority to run the Telescope Image Transfer server and client 
	 * connection threads at.
	 * The value is retrieved from the <b>frodospec.thread.priority.tit</b> property.
	 * If this fails the default FRODOSPEC_DEFAULT_THREAD_PRIORITY_TIT is returned.
	 * @return A valid thread priority between threads MIN_PRIORITY and MAX_PRIORITY.
	 * @see FrodoSpecConstants#FRODOSPEC_DEFAULT_THREAD_PRIORITY_TIT
	 */
	public int getThreadPriorityTIT()
	{
		int retval;

		try
		{
			retval = getPropertyInteger("frodospec.thread.priority.tit");
			if(retval < Thread.MIN_PRIORITY)
				retval = Thread.MIN_PRIORITY;
			if(retval > Thread.MAX_PRIORITY)
				retval = Thread.MAX_PRIORITY;
		}
		catch(NumberFormatException e)
		{
			retval = FrodoSpecConstants.FRODOSPEC_DEFAULT_THREAD_PRIORITY_TIT;
		}
		return retval;
	}

	/**
	 * Method to return the object to synchronise on, whilst moving the fold mirror.
	 * This stops two MOVE_FOLD ISS commands being issued simultaneously by FrodoSpec,
	 * causing the TCS error "Command overriden by more recent request" <<<90041>>>.
	 * @see #foldLock
	 */
	public Object getFoldLock()
	{
		return foldLock;
	}

	/**
	 * Method to return the object to synchronise on, whilst offseting the focus.
	 * This stops two FOCUS_OFFSET ISS commands being issued simultaneously by FrodoSpec,
	 * causing the TCS error "Command overriden by more recent request" <<<90041>>>.
	 * @see #focusOffsetLock
	 */
	public Object getFocusOffsetLock()
	{
		return focusOffsetLock;
	}

	/**
	 * Internal method to initialise the configId array field. This is not done during construction
	 * as the property files need to be loaded to determine the filename to use.
	 * This is got from the <i>frodospec.config.unique_id_filename</i> property.
	 * The configId field is then constructed.
	 * @see #configId
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 */
	private void initialiseConfigId()
	{
		String fileName = null;

		for(int arm=FrodoSpecConfig.RED_ARM; arm<=FrodoSpecConfig.BLUE_ARM; arm++)
		{
			fileName = getProperty("frodospec.config.unique_id_filename."+
					       FrodoSpecConstants.ARM_STRING_LIST[arm]);
			configId[arm] = new PersistentUniqueInteger(fileName);
		}
	}

}
//
// $Log: not supported by cvs2svn $
// Revision 1.8  2013/08/01 11:22:40  eng
// *** empty log message ***
//
// Revision 1.7  2011/06/22 13:32:35  cjm
// Added FRODOSPEC_SETUP processing to getArmFromCommand, so LAMPFOCUS commands
// end up setting currentCommand correctly.
//
// Revision 1.6  2009/08/19 13:55:34  cjm
// Simplified commandCanBeRun. This now allows an ARC on one arm and a MULTRUN on the other,
// assuming the LampController calls internally to these implementations synchronise ARC light/
// lack of arc light to the fibre front end.
//
// Revision 1.5  2009/08/06 13:38:47  cjm
// Added foldLock and focusOffsetLock, used for synchronisation to stop the TCS
// receiving two MOVE_FOLD or FOCUS_OFFSET commands at the same time, which causes a TCS error.
//
// Revision 1.4  2009/04/30 09:57:46  cjm
// Added getArmFromCommand method to collect together common code.
// Used in setCurrentCommand, clearCurrentCommand, getCurrentCommand.
// getArmFromCommand also retrieves arm from CONFIG command so currentCommand setting have changed
// for CONFIG commands.
// Added setCurrentThread, getCurrentThread for ABORT implementation.
//
// Revision 1.3  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.2  2008/11/28 11:16:43  cjm
// configId now per-arm.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
