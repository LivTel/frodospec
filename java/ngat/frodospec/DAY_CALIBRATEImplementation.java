// DAY_CALIBRATEImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/DAY_CALIBRATEImplementation.java,v 1.6 2010-04-07 15:09:52 cjm Exp $
package ngat.frodospec;

import java.io.*;
import java.lang.*;
import java.util.*;

import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation of a FRODOSPEC_DAY_CALIBRATE command sent to a server using the
 * Java Message System. It performs a series of BIAS and DARK frames from a configurable list,
 * taking into account frames done in previous invocations of this command (it saves it's state).
 * @author Chris Mottram
 * @version $Revision: 1.6 $
 */
public class DAY_CALIBRATEImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: DAY_CALIBRATEImplementation.java,v 1.6 2010-04-07 15:09:52 cjm Exp $");
	/**
	 * Initial part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_STRING = "frodospec.day_calibrate.";
	/**
	 * Final part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_TYPE_STRING = ".type";
	/**
	 * Final part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_BIN_STRING = ".config.bin";
	/**
	 * Final part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_FREQUENCY_STRING = ".frequency";
	/**
	 * Final part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_COUNT_STRING = ".count";
	/**
	 * Final part of a key string, used to create a list of potential day calibrations to
	 * perform from a Java property file.
	 */
	protected final static String LIST_KEY_EXPOSURE_TIME_STRING = ".exposure_time";
	/**
	 * Middle part of a key string, used for saving and restoring the stored calibration state.
	 */
	protected final static String LIST_KEY_LAST_TIME_STRING = "last_time.";
	/**
	 * The time, in milliseconds since the epoch, that the implementation of this command was started.
	 */
	private long implementationStartTime = 0L;
	/**
	 * The saved state of calibrations done over time by invocations of this command.
	 * @see DAY_CALIBRATESavedState
	 */
	private DAY_CALIBRATESavedState dayCalibrateState = null;
	/**
	 * The list of calibrations to select from.
	 * Each item in the list is an instance of DAY_CALIBRATECalibration.
	 * @see DAY_CALIBRATECalibration
	 */
	protected List calibrationList = null;
	/**
	 * The readout overhead for a full frame, in milliseconds.
	 */
	private int readoutOverhead = 0;

	/**
	 * Constructor.
	 */
	public DAY_CALIBRATEImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.DAY_CALIBRATE&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_DAY_CALIBRATE";
	}

	/**
	 * This method is the first to be called in this class. 
	 * <ul>
	 * <li>It calls the superclass's init method.
	 * </ul>
	 * @param command The command to be implemented.
	 */
	public void init(COMMAND command)
	{
		super.init(command);
	}

	/**
	 * This method gets the unknown command's acknowledge time. This returns the server connection threads 
	 * default acknowledge time.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getMinAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the DAY_CALIBRATE command.
	 * <ul>
	 * <li>The implementation start time is saved.
	 * <li>loadCalibrationList is called to load a calibration list from the property file.
	 * <li>initialiseState is called to load the saved calibration database.
	 * <li>The readoutOverhead is retrieved from the configuration.
	 * <li>addSavedStateToCalibration is called, which finds the correct last time for each
	 * 	calibration in the list and sets the relevant field.
	 * <li>The FITS headers are cleared, and a the MULTRUN number is incremented.
	 * <li>For each calibration, we do the following:
	 *      <ul>
	 *      <li>testCalibration is called, to see whether the calibration should be done.
	 * 	<li>If it should, doCalibration is called to get the relevant frames.
	 *      </ul>
	 * <li>sendBasicAck is called, to stop the client timing out whilst creating the master bias.
	 * <li>The makeMasterBias method is called, to create master bias fields from the data just taken.
	 * </ul>
	 * Note this method assumes the loading and initialisation before the main loop takes less than the
	 * default acknowledge time, as no ACK's are sent to the client until we are ready to do the first
	 * sequence of calibration frames.
	 * @param command The command to be implemented.
	 * @return An instance of DAY_CALIBRATE_DONE is returned, with it's fields indicating
	 * 	the result of the command implementation.
	 * @see #implementationStartTime
	 * @see #loadCalibrationList
	 * @see #initialiseState
	 * @see #addSavedStateToCalibration
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#frodospecFilenameList
	 * @see #testCalibration
	 * @see #doCalibration
	 * @see #readoutOverhead
	 * @see #frodospecFilenameList
	 * @see ngat.fits.FitsFilename#nextMultRunNumber
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand = (FRODOSPEC_DAY_CALIBRATE)command;
		FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone = new FRODOSPEC_DAY_CALIBRATE_DONE(command.getId());
		DAY_CALIBRATECalibration calibration = null;
		String directoryString = null;
		int makeBiasAckTime,arm;

		dayCalibrateDone.setMeanCounts(0.0f);
		dayCalibrateDone.setPeakCounts(0.0f);
		// get the arm
		arm = dayCalibrateCommand.getArm();
	// initialise
		implementationStartTime = System.currentTimeMillis();
		if(loadCalibrationList(dayCalibrateCommand,dayCalibrateDone) == false)
			return dayCalibrateDone;
		if(initialiseState(dayCalibrateCommand,dayCalibrateDone,arm) == false)
			return dayCalibrateDone;
	// Get the amount of time to readout and save a full frame
		try
		{
			readoutOverhead = status.getPropertyInteger("frodospec.day_calibrate.readout_overhead");
		}
		catch (Exception e)
		{
			String errorString = new String(command.getId()+
				":processCommand:Failed to get readout overhead.");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2200);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return dayCalibrateDone;
		}
	// match saved state to calibration list (put last time into calibration list)
		if(addSavedStateToCalibration(dayCalibrateCommand,dayCalibrateDone) == false)
			return dayCalibrateDone;
	// initialise status/fits header info, in case any frames are produced.
	// get fits headers
		clearFitsHeaders(arm);
	// get a filename to store frame in
		frodospecFilenameList[arm].nextMultRunNumber();
	// main loop, do calibrations until we run out of time.
		for(int i = 0; i < calibrationList.size(); i++)
		{
			calibration = (DAY_CALIBRATECalibration)(calibrationList.get(i));
		// see if we are going to do this calibration.
		// Note if we have run out of time (timeToComplete) then this method
		// should always return false.
			if(testCalibration(dayCalibrateCommand,dayCalibrateDone,calibration))
			{
				if(doCalibration(dayCalibrateCommand,dayCalibrateDone,calibration) == false)
					return dayCalibrateDone;
			}
		}// end for on calibration list
	// send an ack before make master processing, so the client doesn't time out.
		makeBiasAckTime = status.getPropertyInteger("frodospec.day_calibrate.acknowledge_time.make_bias");
		if(sendBasicAck(dayCalibrateCommand,dayCalibrateDone,makeBiasAckTime) == false)
			return dayCalibrateDone;
	// get directory FITS files are in.
		directoryString = status.getProperty("frodospec.file.fits.path");
		if(directoryString.endsWith(System.getProperty("file.separator")) == false)
			directoryString = directoryString.concat(System.getProperty("file.separator"));
	// Call pipeline to create master bias.
		if(makeMasterBias(dayCalibrateCommand,dayCalibrateDone,directoryString) == false)
			return dayCalibrateDone;
	// return done
		dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		dayCalibrateDone.setErrorString("");
		dayCalibrateDone.setSuccessful(true);
		return dayCalibrateDone;
	}

	/**
	 * Method to load a list of calibrations to do.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @return The method returns true if it succeeds, false if it fails. If false is returned the error
	 * 	data in dayCalibrateDone is filled in.
	 * @see #calibrationList
	 * @see #LIST_KEY_STRING
	 * @see #LIST_KEY_TYPE_STRING
	 * @see #LIST_KEY_BIN_STRING
	 * @see #LIST_KEY_FREQUENCY_STRING
	 * @see #LIST_KEY_COUNT_STRING
	 * @see #LIST_KEY_EXPOSURE_TIME_STRING
	 */
	protected boolean loadCalibrationList(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
					      FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone)
	{
		DAY_CALIBRATECalibration calibration = null;
		String typeString = null;
		int index,bin,count,exposureTime;
		long frequency;
		boolean done;

		index = 0;
		done = false;
		calibrationList = new Vector();
		while(done == false)
		{
			// diddly per arm or same day calibrations for each arm?
			typeString = status.getProperty(LIST_KEY_STRING+index+LIST_KEY_TYPE_STRING);
			if(typeString != null)
			{
			// create calibration instance, and set it's type
				calibration = new DAY_CALIBRATECalibration();
				try
				{
					calibration.setType(typeString);
				}
				catch(Exception e)
				{
					String errorString = new String(dayCalibrateCommand.getId()+
						":loadCalibrationList:Failed to set type "+typeString+
						" at index "+index+".");
					frodospec.error(this.getClass().getName()+":"+errorString,e);
					dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+
								     2203);
					dayCalibrateDone.setErrorString(errorString);
					dayCalibrateDone.setSuccessful(false);
					return false;
				}
			// get common parameters
				try
				{
					bin = status.getPropertyInteger(LIST_KEY_STRING+index+LIST_KEY_BIN_STRING);
					frequency = status.getPropertyLong(LIST_KEY_STRING+index+
										LIST_KEY_FREQUENCY_STRING);
					count = status.getPropertyInteger(LIST_KEY_STRING+index+LIST_KEY_COUNT_STRING);
					if(calibration.isBias())
					{
						exposureTime = 0;
					}
					else if(calibration.isDark())
					{
						exposureTime = status.getPropertyInteger(LIST_KEY_STRING+index+
							LIST_KEY_EXPOSURE_TIME_STRING);
					}
					else // we should never get here
						exposureTime = 0;
				}
				catch(Exception e)
				{
					String errorString = new String(dayCalibrateCommand.getId()+
						":loadCalibrationList:Failed at index "+index+".");
					frodospec.error(this.getClass().getName()+":"+errorString,e);
					dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+
								     2204);
					dayCalibrateDone.setErrorString(errorString);
					dayCalibrateDone.setSuccessful(false);
					return false;
				}
			// set calibration data
				try
				{
					calibration.setBin(bin);
					calibration.setFrequency(frequency);
					calibration.setCount(count);
					calibration.setExposureTime(exposureTime);
				}
				catch(Exception e)
				{
					String errorString = new String(dayCalibrateCommand.getId()+
						":loadCalibrationList:Failed to set calibration data at index "+index+
						":bin:"+bin+
						":frequency:"+frequency+":count:"+count+
						":exposure time:"+exposureTime+".");
					frodospec.error(this.getClass().getName()+":"+errorString,e);
					dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+
								     2205);
					dayCalibrateDone.setErrorString(errorString);
					dayCalibrateDone.setSuccessful(false);
					return false;
				}
			// add calibration instance to list
				calibrationList.add(calibration);
			// log
				frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
					"Command:"+dayCalibrateCommand.getClass().getName()+
					":Loaded calibration "+index+
					"\n\ttype:"+calibration.getType()+
					":bin:"+calibration.getBin()+
					":count:"+calibration.getCount()+
					":texposure time:"+calibration.getExposureTime()+
					":frequency:"+calibration.getFrequency()+".");
			}
			else
				done = true;
			index++;
		}
		return true;
	}

	/**
	 * Method to initialse dayCalibrateState.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param arm Which arm to retrieve the day calibration state for, either RED_ARM or BLUE_ARM.
	 * @return The method returns true if it succeeds, false if it fails. If false is returned the error
	 * 	data in dayCalibrateDone is filled in.
	 * @see #dayCalibrateState
	 */
	protected boolean initialiseState(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
					  FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,int arm)
	{
	// initialise and load dayCalibrateState instance
		dayCalibrateState = new DAY_CALIBRATESavedState();
		try
		{
			dayCalibrateState.load(arm);
		}
		catch (Exception e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":initialiseState:Failed to load state filename for arm "+
							FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2202);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * This method matches the saved state to the calibration list to set the last time
	 * each calibration was completed.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @return The method returns true if it succeeds, false if it fails. It currently always returns true.
	 * @see #calibrationList
	 * @see #dayCalibrateState
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 */
	protected boolean addSavedStateToCalibration(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
		FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone)
	{

		DAY_CALIBRATECalibration calibration = null;
		int type,count,bin,exposureTime,arm;
		long lastTime;

		arm = dayCalibrateCommand.getArm();
		for(int i = 0; i< calibrationList.size(); i++)
		{
			calibration = (DAY_CALIBRATECalibration)(calibrationList.get(i));
			type = calibration.getType();
			bin = calibration.getBin();
			exposureTime = calibration.getExposureTime();
			count = calibration.getCount();
			lastTime = dayCalibrateState.getLastTime(arm,type,bin,exposureTime,count);
			calibration.setLastTime(lastTime);
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
				"Command:"+dayCalibrateCommand.getClass().getName()+":Calibration:"+
				"\n\tarm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				":type:"+calibration.getType()+
				":bin:"+calibration.getBin()+
				":count:"+calibration.getCount()+
				":exposure time:"+calibration.getExposureTime()+
				":frequency:"+calibration.getFrequency()+
				"\n\t\tnow has last time set to:"+lastTime+".");
		}
		return true;
	}

	/**
	 * This method try's to determine whether we should perform the passed in calibration.
	 * The following  cases are tested:
	 * <ul>
	 * <li>If the current time is after the implementationStartTime plus the dayCalibrateCommand's timeToComplete
	 * 	return false, because the FRODOSPEC_DAY_CALIBRATE command should be stopping (it's run out of time).
	 * <li>If the difference between the current time and the last time the calibration was done is
	 * 	less than the frequency return false, it's too soon to do this calibration again.
	 * <li>We work out how long it will take us to do the calibration, using the <b>count</b>, 
	 * 	<b>exposureTime</b>, and the <b>readoutOverhead</b> property.
	 * <li>If it's going to take us longer to do the calibration than the remaining time available, return
	 * 	false.
	 * <li>Otherwise, return true.
	 * </ul>
	 * @param dayCalibrateCommand The instance of DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param calibration The calibration we wish to determine whether to do or not.
	 * @return The method returns true if we should do the calibration, false if we should not.
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 */
	protected boolean testCalibration(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
					  FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,
					  DAY_CALIBRATECalibration calibration)
	{
		long now;
		long calibrationCompletionTime;
		int arm;

		arm = dayCalibrateCommand.getArm();
	// get current time
		now = System.currentTimeMillis();
	// if current time is after the implementation start time plus time to complete, it's time to finish
	// We don't want to do any more calibrations, return false.
		if(now > (implementationStartTime+dayCalibrateCommand.getTimeToComplete()))
		{
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
				"Command:"+dayCalibrateCommand.getClass().getName()+":Testing Calibration:"+
				"\n\tarm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				":type:"+calibration.getType()+
				":bin:"+calibration.getBin()+
				":count:"+calibration.getCount()+
				":exposure time:"+calibration.getExposureTime()+
				":frequency:"+calibration.getFrequency()+
				"\n\tlast time:"+calibration.getLastTime()+
				"\n\t\twill not be done,time to complete exceeded ("+now+" > ("+
				implementationStartTime+" + "+dayCalibrateCommand.getTimeToComplete()+")).");
			return false;
		}
	// If the last time we did this calibration was less than frequency milliseconds ago, then it's
	// too soon to do the calibration again.
		if((now-calibration.getLastTime()) < calibration.getFrequency())
		{
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
				"Command:"+dayCalibrateCommand.getClass().getName()+":Testing Calibration:"+
				"\n\tarm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				":type:"+calibration.getType()+
				":bin:"+calibration.getBin()+
				":count:"+calibration.getCount()+
				":exposure time:"+calibration.getExposureTime()+
				":frequency:"+calibration.getFrequency()+
				"\n\tlast time:"+calibration.getLastTime()+
				"\n\t\twill not be done,too soon after last calibration.");
			return false;
		}
	// How long will it take us to do this calibration?
		if(calibration.isBias())
		{
			calibrationCompletionTime = calibration.getCount()*readoutOverhead;
		}
		else if(calibration.isDark())
		{
			calibrationCompletionTime = calibration.getCount()*
					(calibration.getExposureTime()+readoutOverhead);
		}
		else // we should never get here, but if we do make method return false.
			calibrationCompletionTime = Long.MAX_VALUE;
	// if it's going to take us longer than the remaining time to do this, return false
		if((now+calibrationCompletionTime) > (implementationStartTime+dayCalibrateCommand.getTimeToComplete()))
		{
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
				"Command:"+dayCalibrateCommand.getClass().getName()+":Testing Calibration:"+
				"\n\tarm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+
				":type:"+calibration.getType()+
				":bin:"+calibration.getBin()+
				":count:"+calibration.getCount()+
				":exposure time:"+calibration.getExposureTime()+
				":frequency:"+calibration.getFrequency()+
				"\n\tlast time:"+calibration.getLastTime()+
				"\n\t\twill not be done,will take too long to complete:"+
				calibrationCompletionTime+".");
			return false;
		}
		return true;
	}

	/**
	 * This method does the specified calibration.
	 * <ul>
	 * <li>The relevant data is retrieved from the calibration parameter.
	 * <li><b>doConfig</b> is called for the relevant binning factor to be setup.
	 * <li><b>sendBasicAck</b> is called to stop the client timing out before the first frame is completed.
	 * <li><b>doFrames</b> is called to exposure count frames with the correct exposure length (DARKs only).
	 * <li>If the calibration suceeded, the saved state's last time is updated to now, and the state saved.
	 * </ul>
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param calibration The calibration to do.
	 * @return The method returns true if the calibration was done successfully, false if an error occured.
	 * @see #doConfig
	 * @see #doFrames
	 * @see #sendBasicAck
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 */
	protected boolean doCalibration(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
					FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,
					DAY_CALIBRATECalibration calibration)
	{
		int type,count,bin,exposureTime;
		long lastTime;
		int arm;

		arm = dayCalibrateCommand.getArm();
		frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
			      "Command:"+dayCalibrateCommand.getClass().getName()+
			      ":doCalibrate:arm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+
			      ":type:"+calibration.getType()+":bin:"+calibration.getBin()+
			      ":count:"+calibration.getCount()+":exposure time:"+calibration.getExposureTime()+
			      ":frequency:"+calibration.getFrequency()+".");
	// get copy of calibration data
		type = calibration.getType();
		bin = calibration.getBin();
		count = calibration.getCount();
		exposureTime = calibration.getExposureTime();
	// configure CCD camera
	// don't send a basic ack, as setting the binning takes less than 1 second
		if(doConfig(dayCalibrateCommand,dayCalibrateDone,bin) == false)
			return false;
	// send an ack before the frame, so the client doesn't time out during the first exposure
		if(sendBasicAck(dayCalibrateCommand,dayCalibrateDone,exposureTime+readoutOverhead) == false)
			return false;
	// do the frames with this configuration
		if(doFrames(dayCalibrateCommand,dayCalibrateDone,type,exposureTime,count) == false)
			return false;
	// update state
		dayCalibrateState.setLastTime(arm,type,bin,exposureTime,count);
		try
		{
			dayCalibrateState.save();
		}
		catch(IOException e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doCalibration:Failed to save state filename for arm "+
							FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2206);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
		lastTime = dayCalibrateState.getLastTime(arm,type,bin,exposureTime,count);
		calibration.setLastTime(lastTime);
		return true;
	}

	/**
	 * Method to setup the CCD configuration with the specified binning factor.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param bin The binning factor to use.
	 * @return The method returns true if the calibration was done successfully, false if an error occured.
	 * @see #redCCD
	 * @see #blueCCD
	 * @see FrodoSpecStatus#getNumberColumns
	 * @see FrodoSpecStatus#getNumberRows
	 * @see FrodoSpecStatus#getPropertyBoolean
	 * @see FITSImplementation#getAmplifier
	 * @see FITSImplementation#getDeInterlaceSetting
	 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.frodospec.ccd.CCDLibrary#setupDimensions
	 */
	protected boolean doConfig(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
				   FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,int bin)
	{
		CCDLibrarySetupWindow windowList[] = new CCDLibrarySetupWindow[CCDLibrary.SETUP_WINDOW_COUNT];
		CCDLibrary ccd = null;
		int arm,numberColumns,numberRows,amplifier,deInterlaceSetting;
		boolean ccdEnable;

		arm = dayCalibrateCommand.getArm();
	// Which instance of the CCD library.
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doConfig:Failed to get sensible arm:"+arm);
			frodospec.error(this.getClass().getName()+":"+errorString);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2203);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
	// load other required config for dimension configuration from properties file.
		try
		{
		// numbers returned from these are affected by previous binning configurations.
			numberColumns = status.getNumberColumns(bin);
			numberRows = status.getNumberRows(bin);
	                amplifier = getAmplifier(arm);
			deInterlaceSetting = getDeInterlaceSetting(arm);
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
							      ".enable");
		}
	// CCDLibraryFormatException,IllegalArgumentException,NumberFormatException.
		catch(Exception e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doConfig:Failed to get config:");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2207);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
	// test abort
		if(testAbort(dayCalibrateCommand,dayCalibrateDone) == true)
			return false;
		// setup windows
		for(int i = 0; i < CCDLibrary.SETUP_WINDOW_COUNT; i++)
		{
			windowList[i] = new CCDLibrarySetupWindow(-1,-1,-1,-1);
		}
	// send dimension configuration to the SDSU controller
		try
		{
			if(ccdEnable)
			{
				ccd.setupDimensions(numberColumns,numberRows,bin,bin,
						    amplifier,deInterlaceSetting,0,windowList);
			}
			else
			{
				frodospec.log(Logger.VERBOSITY_INTERMEDIATE,
					      this.getClass().getName()+
					      ":doConfig:CCD not enabled:CCD library NOT configured.");
			}
		}
		catch(CCDLibraryNativeException e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doConfig:Failed to setup dimensions:");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2208);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
	// test abort
		if(testAbort(dayCalibrateCommand,dayCalibrateDone) == true)
			return false;
	// Increment unique config ID.
	// This is queried when saving FITS headers to get the CONFIGID value.
		try
		{
			status.incConfigId(arm);
		}
		catch(Exception e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doConfig:Incrementing configuration ID failed:");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2209);
			dayCalibrateDone.setErrorString(errorString+e);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
	// Store name of configuration used in status object.
	// This is queried when saving FITS headers to get the CONFNAME value.
		status.setConfigName(arm,"DAY_CALIBRATION:"+dayCalibrateCommand.getId()+":"+bin);
		return true;
	}

	/**
	 * The method that does a series of calibration frames with the current configuration, based
	 * on the passed in parameter set. The following occurs:
	 * <ul>
	 * <li>A loop is entered, from zero to <b>count</b>.
	 * <li>The pause and resume times are cleared, and the FITS headers setup from the current configuration.
	 * <li>Some FITS headers are got from the ISS.
	 * <li>testAbort is called to see if this command implementation has been aborted.
	 * <li>The correct filename code is set in frodospecFilenameList. The run number is incremented and
	 * 	a new unique filename generated.
	 * <li>The FITS headers are saved using saveFitsHeaders.
	 * <li>The frame is taken, using libccd. If the <b>type</b> is BIAS, CCDLibrary's bias
	 * 	method is called, otherwise expose is used.
	 * <li>The FITS file lock craeted by saveFitsHeaders is removed using unLockFile.
	 * <li>testAbort is called to see if this command implementation has been aborted.
	 * <li>reduceCalibrate is called to pass the frame to the Real Time Data Pipeline for processing.
	 * </ul>
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param type The type of calibration, one of DAY_CALIBRATECalibration.TYPE_BIAS
	 * 	or DAY_CALIBRATECalibration.TYPE_DARK.
	 * @param exposureTime The length of exposure for a DARK, always zero for a BIAS.
	 * @param count The number of frames to do of this type.
	 * @return The method returns true if the calibration was done successfully, false if an error occured.
	 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_BIAS
	 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_DARK
	 * @see FITSImplementation#testAbort
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FITSImplementation#frodospecFilenameList
	 * @see ngat.frodospec.ccd.CCDLibrary#bias
	 * @see ngat.frodospec.ccd.CCDLibrary#expose
	 * @see CALIBRATEImplementation#reduceCalibrate
	 * @see #readoutOverhead
	 */
	protected boolean doFrames(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
				   FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,
				   int type, int exposureTime, int count)
	{
		CCDLibrary ccd = null;
		String filename = null;
		int arm;

		arm = dayCalibrateCommand.getArm();
	// Which instance of the CCD library.
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":doFrames:Failed to get sensible arm:"+arm);
			frodospec.error(this.getClass().getName()+":"+errorString);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2204);
			dayCalibrateDone.setErrorString(errorString);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
		for(int i = 0;i < count; i++)
		{
		// Clear the pause and resume times.
			status.clearPauseResumeTimes();
			if(type == DAY_CALIBRATECalibration.TYPE_BIAS)
			{
				if(setFitsHeaders(dayCalibrateCommand,dayCalibrateDone,arm,
					FitsHeaderDefaults.OBSTYPE_VALUE_BIAS,0) == false)
					return false;
			}
			else if (type == DAY_CALIBRATECalibration.TYPE_DARK)
			{
				if(setFitsHeaders(dayCalibrateCommand,dayCalibrateDone,arm,
					FitsHeaderDefaults.OBSTYPE_VALUE_DARK,exposureTime) == false)
					return false;
			}
			if(getFitsHeadersFromISS(dayCalibrateCommand,dayCalibrateDone,arm) == false)
				return false;
			if(testAbort(dayCalibrateCommand,dayCalibrateDone) == true)
				return false;
		// get a filename to store frame in
			try
			{
				if(type == DAY_CALIBRATECalibration.TYPE_BIAS)
				{
					frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_BIAS);
				}
				else if (type == DAY_CALIBRATECalibration.TYPE_DARK)
				{
					frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_DARK);
				}
			}
			catch(Exception e)
			{
				String errorString = new String(dayCalibrateCommand.getId()+
					":doFrames:Doing frame "+i+" failed:");
				frodospec.error(this.getClass().getName()+":"+errorString,e);
				dayCalibrateDone.setFilename(filename);
				dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2214);
				dayCalibrateDone.setErrorString(errorString+e);
				dayCalibrateDone.setSuccessful(false);
				return false;
			}
			frodospecFilenameList[arm].nextRunNumber();
			filename = frodospecFilenameList[arm].getFilename();
			if(saveFitsHeaders(dayCalibrateCommand,dayCalibrateDone,arm,filename) == false)
			{
				unLockFile(dayCalibrateCommand,dayCalibrateDone,filename);
				return false;
			}
			status.setExposureFilename(arm,filename);
		// do exposure
			try
			{
				if(type == DAY_CALIBRATECalibration.TYPE_BIAS)
				{
					ccd.bias(filename);
				}
				else if (type == DAY_CALIBRATECalibration.TYPE_DARK)
				{
					ccd.expose(false,-1,exposureTime,filename);
				}
			}
			catch(CCDLibraryNativeException e)
			{
				String errorString = new String(dayCalibrateCommand.getId()+
					":doFrames:Doing frame "+i+" failed:");
				frodospec.error(this.getClass().getName()+":"+errorString,e);
				dayCalibrateDone.setFilename(filename);
				dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2210);
				dayCalibrateDone.setErrorString(errorString+e);
				dayCalibrateDone.setSuccessful(false);
				unLockFile(dayCalibrateCommand,dayCalibrateDone,filename);
				return false;
			}
			// unlock FITS file lock created by saveFitsHeaders
			if(unLockFile(dayCalibrateCommand,dayCalibrateDone,filename) == false)
				return false;
		// send with filename back to client
		// time to complete is reduction time, we will send another ACK after reduceCalibrate
			if(sendDayCalibrateAck(dayCalibrateCommand,dayCalibrateDone,readoutOverhead,filename) == false)
				return false; 
		// Test abort status.
			if(testAbort(dayCalibrateCommand,dayCalibrateDone) == true)
				return false;
		// Call pipeline to reduce data.
			if(reduceCalibrate(dayCalibrateCommand,dayCalibrateDone,filename) == false)
				return false; 
		// send dp_ack, filename/mean counts/peak counts are all retrieved from dayCalibrateDone,
		// which had these parameters filled in by reduceCalibrate
		// time to complete is readout overhead + exposure Time for next frame
			if(sendDayCalibrateDpAck(dayCalibrateCommand,dayCalibrateDone,
				exposureTime+readoutOverhead) == false)
				return false;
		}// end for on count
		return true;
	}

	/**
	 * Method to send an instance of DAY_CALIBRATE_ACK back to the client. This tells the client about
	 * a FITS frame that has been produced, and also stops the client timing out.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * @param timeToComplete The time it will take to complete the next set of operations
	 *	before the next ACK or DONE is sent to the client. The time is in milliseconds. 
	 * 	The server connection thread's default acknowledge time is added to the value before it
	 * 	is sent to the client, to allow for network delay etc.
	 * @param filename The FITS filename to be sent back to the client, that has just completed
	 * 	processing.
	 * @return The method returns true if the ACK was sent successfully, false if an error occured.
	 */
	protected boolean sendDayCalibrateAck(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
					      FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,
					      int timeToComplete,String filename)
	{
		DAY_CALIBRATE_ACK dayCalibrateAck = null;

	// send acknowledge to say frame is completed.
		dayCalibrateAck = new DAY_CALIBRATE_ACK(dayCalibrateCommand.getId());
		dayCalibrateAck.setTimeToComplete(timeToComplete+
			serverConnectionThread.getDefaultAcknowledgeTime());
		dayCalibrateAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(dayCalibrateAck,true);
		}
		catch(IOException e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":sendDayCalibrateAck:Sending DAY_CALIBRATE_ACK failed:");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2211);
			dayCalibrateDone.setErrorString(errorString+e);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Method to send an instance of DAY_CALIBRATE_DP_ACK back to the client. This tells the client about
	 * a FITS frame that has been produced, and the mean and peak counts in the frame.
	 * The time to complete parameter stops the client timing out.
	 * @param dayCalibrateCommand The instance of FRODOSPEC_DAY_CALIBRATE we are currently running.
	 * @param dayCalibrateDone The instance of FRODOSPEC_DAY_CALIBRATE_DONE to fill in with errors we receive.
	 * 	It also contains the filename and mean and peak counts returned from the last reduction calibration.
	 * @param timeToComplete The time it will take to complete the next set of operations
	 *	before the next ACK or DONE is sent to the client. The time is in milliseconds. 
	 * 	The server connection thread's default acknowledge time is added to the value before it
	 * 	is sent to the client, to allow for network delay etc.
	 * @return The method returns true if the ACK was sent successfully, false if an error occured.
	 */
	protected boolean sendDayCalibrateDpAck(FRODOSPEC_DAY_CALIBRATE dayCalibrateCommand,
						FRODOSPEC_DAY_CALIBRATE_DONE dayCalibrateDone,
						int timeToComplete)
	{
		DAY_CALIBRATE_DP_ACK dayCalibrateDpAck = null;

	// send acknowledge to say frame is completed.
		dayCalibrateDpAck = new DAY_CALIBRATE_DP_ACK(dayCalibrateCommand.getId());
		dayCalibrateDpAck.setTimeToComplete(timeToComplete+
			serverConnectionThread.getDefaultAcknowledgeTime());
		dayCalibrateDpAck.setFilename(dayCalibrateDone.getFilename());
		dayCalibrateDpAck.setMeanCounts(dayCalibrateDone.getMeanCounts());
		dayCalibrateDpAck.setPeakCounts(dayCalibrateDone.getPeakCounts());
		try
		{
			serverConnectionThread.sendAcknowledge(dayCalibrateDpAck,true);
		}
		catch(IOException e)
		{
			String errorString = new String(dayCalibrateCommand.getId()+
				":sendDayCalibrateDpAck:Sending DAY_CALIBRATE_DP_ACK failed:");
			frodospec.error(this.getClass().getName()+":"+errorString,e);
			dayCalibrateDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2212);
			dayCalibrateDone.setErrorString(errorString+e);
			dayCalibrateDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Private inner class that deals with loading and interpreting the saved state of calibrations
	 * (the DAY_CALIBRATE calibration database).
	 */
	private class DAY_CALIBRATESavedState
	{
		/**
		 * The properties filename.
		 */
		private String filename = null;
		/**
		 * The properties containing the saved state.
		 */
		private NGATProperties properties = null;

		/**
		 * Constructor.
		 */
		public DAY_CALIBRATESavedState()
		{
			super();
			properties = new NGATProperties();
		}

		/**
	 	 * Load method, that retrieves the saved state from file.
		 * Calls the <b>properties</b> load method.
		 * The filename is retrieved from "frodospec.day_calibrate.state_filename."+arm.
		 * There is separate saved state filenames for each arm, so two instances of DAY_CALIBRATE can run 
		 * on each arm at the same time without state writes overwriting each other.
		 * @param arm The arm to load the saved state for.
		 * @exception FileNotFoundException Thrown if the file described by filename does not exist.
		 * @exception IOException Thrown if an IO error occurs whilst reading the file.
		 * @exception Exception Thrown if getProperty fails.
		 * @see #properties
		 * @see #filename
		 * @see #status
		 * @see FrodoSpecConstants#ARM_STRING_LIST
	 	 */
		public void load(int arm) throws FileNotFoundException, IOException, Exception
		{
			filename = status.getProperty("frodospec.day_calibrate.state_filename."+
						      FrodoSpecConstants.ARM_STRING_LIST[arm]);
			properties.load(filename);
		}

		/**
	 	 * Save method, that stores the saved state into a file.
		 * Calls the <b>properties</b> save method.
		 * @exception IOException Thrown if an IO error occurs whilst writing the file.
		 * @see #properties
	 	 */
		public void save() throws IOException
		{
			Date now = null;

			now = new Date();
			properties.save(filename,"DAY_CALIBRATE saved state saved on:"+now);
		}

		/**
		 * Method to get the last time a calibration with these attributes was done.
		 * @param arm Which arm the calibration was done on, either RED_ARM or BLUE_ARM.
		 * @param type The type of calibration, one of DAY_CALIBRATECalibration.TYPE_BIAS
		 * 	or DAY_CALIBRATECalibration.TYPE_DARK.
		 * @param bin The binning factor used for this calibration.
		 * @param exposureTime The length of exposure for a DARK, always zero for a BIAS.
		 * @param count The number of frames.
		 * @return The number of milliseconds since the EPOCH, the last time a calibration with these
		 * 	parameters was completed. If this calibraion has not been performed before, zero
		 * 	is returned.
		 * @see #typeToString
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_BIAS
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_DARK
		 * @see #LIST_KEY_STRING
		 * @see #LIST_KEY_LAST_TIME_STRING
		 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
		 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
		 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
		 */
		public long getLastTime(int arm,int type,int bin,int exposureTime,int count)
		{
			long time;

			try
			{
				time = properties.getLong(LIST_KEY_STRING+LIST_KEY_LAST_TIME_STRING+
							  FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
							  typeToString(type)+"."+bin+"."+exposureTime+"."+count);
			}
			catch(NGATPropertyException e)/* assume failure due to key not existing */
			{
				time = 0;
			}
			return time;
		}

		/**
		 * Method to set the last time a calibration with these attributes was done.
		 * The time is set to now. The property file should be saved after a call to this method is made.
		 * @param arm Which arm the calibration was done on, either RED_ARM or BLUE_ARM.
		 * @param type The type of calibration, one of DAY_CALIBRATECalibration.TYPE_BIAS
		 * 	or DAY_CALIBRATECalibration.TYPE_DARK.
		 * @param bin The binning factor used for this calibration.
		 * @param exposureTime The length of exposure for a DARK, always zero for a BIAS.
		 * @param count The number of frames.
		 * @see #typeToString
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_BIAS
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_DARK
		 * @see #LIST_KEY_STRING
		 * @see #LIST_KEY_LAST_TIME_STRING
		 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
		 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
		 * @see ngat.frodospec.FrodoSpecConstants#ARM_STRING_LIST
		 */
		public void setLastTime(int arm,int type,int bin,int exposureTime,int count)
		{
			long now;

			now = System.currentTimeMillis();
			properties.setProperty(LIST_KEY_STRING+LIST_KEY_LAST_TIME_STRING+
					       FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+typeToString(type)
					       +"."+bin+"."+exposureTime+"."+count,new String(""+now));
		}

		/**
		 * Method to convert a type number to a string.
		 * @param type The type number, either TYPE_DARK or TYPE_BIAS.
		 * @return A String, either "dark" or "bias".
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_BIAS
		 * @see DAY_CALIBRATEImplementation.DAY_CALIBRATECalibration#TYPE_DARK
		 */
		protected String typeToString(int type)
		{
			if(type == DAY_CALIBRATECalibration.TYPE_DARK)
				return "dark";
			else if(type == DAY_CALIBRATECalibration.TYPE_BIAS)
				return "bias";
			return null;
		}
	}

	/**
	 * Private inner class that stores data pertaining to one possible calibration run that can take place during
	 * a DAY_CALIBRATE command invocation.
	 */
	private class DAY_CALIBRATECalibration
	{
		/**
		 * Constant used in the type field to specify this calibration is a BIAS.
		 * @see #type
		 */
		public final static int TYPE_BIAS = 1;
		/**
		 * Constant used in the type field to specify this calibration is a DARK.
		 * @see #type
		 */
		public final static int TYPE_DARK = 2;
		/**
		 * What type of calibration is it? A DARK or a BIAS?
		 * @see #TYPE_BIAS
		 * @see #TYPE_DARK
		 */
		protected int type = 0;
		/**
		 * What binning to configure the ccd to for this calibration.
		 */
		protected int bin;
		/**
		 * How many times to perform this calibration.
		 */
		protected int count;
		/**
		 * How often we should perform the calibration in milliseconds.
		 */
		protected long frequency;
		/**
		 * How long an exposure time this calibration has. This is zero for BIAS frames.
		 */
		protected int exposureTime;
		/**
		 * The last time this calibration was performed. This is retrieved from the saved state,
		 * not from the calibration list.
		 */
		protected long lastTime;
		
		/**
		 * Constructor.
		 */
		public DAY_CALIBRATECalibration()
		{
			super();
		}

		/**
		 * Method to set the type of the calibration i.e. is it a DARK or BIAS.
		 * @param typeString A string describing the type. This should be &quot;dark&quot; or
		 * 	&quot;bias&quot;.
		 * @exception IllegalArgumentException Thrown if typeString is an illegal type.
		 * @see #type
		 */
		public void setType(String typeString) throws IllegalArgumentException
		{
			if(typeString.equals("dark"))
				type = TYPE_DARK;
			else if(typeString.equals("bias"))
				type = TYPE_BIAS;
			else
				throw new IllegalArgumentException(this.getClass().getName()+":setType failed:"+
					typeString+" not a legal type of calibration.");
		}

		/**
		 * Method to get the calibration type of this calibration.
		 * @return The type of calibration.
		 * @see #type
		 * @see #TYPE_DARK
		 * @see #TYPE_BIAS
		 */
		public int getType()
		{
			return type;
		}

		/**
		 * Method to return whether this calibration is a DARK.
		 * @return This method returns true if <b>type</b> is <b>TYPE_DARK</b>, otherwqise it returns false.
		 * @see #type
		 * @see #TYPE_DARK
		 */
		public boolean isDark()
		{
			return (type == TYPE_DARK);
		}

		/**
		 * Method to return whether this calibration is a BIAS.
		 * @return This method returns true if <b>type</b> is <b>TYPE_BIAS</b>, otherwise it returns false.
		 * @see #type
		 * @see #TYPE_BIAS
		 */
		public boolean isBias()
		{
			return (type == TYPE_BIAS);
		}

		/**
		 * Method to set the binning configuration for this calibration.
		 * @param b The binning to use. This should be greater than 0 and less than 5.
		 * @exception IllegalArgumentException Thrown if parameter b is out of range.
		 * @see #bin
		 */
		public void setBin(int b) throws IllegalArgumentException
		{
			if((b < 1)||(b > 4))
			{
				throw new IllegalArgumentException(this.getClass().getName()+":setBin failed:"+
					b+" not a legal binning value.");
			}
			bin = b;
		}

		/**
		 * Method to get the binning configuration for this calibration.
		 * @return The binning.
		 * @see #bin
		 */
		public int getBin()
		{
			return bin;
		}

		/**
		 * Method to set the frequency this calibration should be performed.
		 * @param f The frequency in milliseconds. This should be greater than zero.
		 * @exception IllegalArgumentException Thrown if parameter f is out of range.
		 * @see #frequency
		 */
		public void setFrequency(long f) throws IllegalArgumentException
		{
			if(f <= 0)
			{
				throw new IllegalArgumentException(this.getClass().getName()+":setFrequency failed:"+
					f+" not a legal frequency.");
			}
			frequency = f;
		}

		/**
		 * Method to get the frequency configuration for this calibration.
		 * @return The frequency this calibration should be performed, in milliseconds.
		 * @see #frequency
		 */
		public long getFrequency()
		{
			return frequency;
		}

		/**
		 * Method to set the number of times this calibration should be performed in one invocation
		 * of DAY_CALIBRATE.
		 * @param c The number of times this calibration should be performed in one invocation
		 * of DAY_CALIBRATE This should be greater than zero.
		 * @exception IllegalArgumentException Thrown if parameter c is out of range.
		 * @see #count
		 */
		public void setCount(int c) throws IllegalArgumentException
		{
			if(c <= 0)
			{
				throw new IllegalArgumentException(this.getClass().getName()+":setCount failed:"+
					c+" not a legal count.");
			}
			count = c;
		}

		/**
		 * Method to get the number of times this calibration is performed per invocation of DAY_CALIBRATE.
		 * @return The number of times this calibration is performed per invocation of DAY_CALIBRATE.
		 * @see #count
		 */
		public int getCount()
		{
			return count;
		}

		/**
		 * Method to set the exposure length of this calibration.
		 * @param t The exposure length in milliseconds. This should be greater than or equal to zero.
		 * @exception IllegalArgumentException Thrown if parameter t is out of range.
		 * @see #exposureTime
		 */
		public void setExposureTime(int t) throws IllegalArgumentException
		{
			if(t < 0)
			{
				throw new IllegalArgumentException(this.getClass().getName()+
					":setExposureTime failed:"+t+" not a legal exposure length.");
			}
			exposureTime = t;
		}

		/**
		 * Method to get the exposure length of this calibration.
		 * @return The exposure length of this calibration.
		 * @see #exposureTime
		 */
		public int getExposureTime()
		{
			return exposureTime;
		}

		/**
		 * Method to set the last time this calibration was performed.
		 * @param t A long representing the last time the calibration was done, as a 
		 * 	number of milliseconds since the EPOCH.
		 * @exception IllegalArgumentException Thrown if parameter f is out of range.
		 * @see #frequency
		 */
		public void setLastTime(long t) throws IllegalArgumentException
		{
			if(t < 0)
			{
				throw new IllegalArgumentException(this.getClass().getName()+":setLastTime failed:"+
					t+" not a legal last time.");
			}
			lastTime = t;
		}

		/**
		 * Method to get the last time this calibration was performed.
		 * @return The number of milliseconds since the epoch that this calibration was last performed.
		 * @see #frequency
		 */
		public long getLastTime()
		{
			return lastTime;
		}
	}
}
 
//
// $Log: not supported by cvs2svn $
// Revision 1.5  2010/02/08 11:09:43  cjm
// Added unLockFile calls as saveFitsHeaders now creates FITS file locks.
//
// Revision 1.4  2009/09/18 15:20:46  cjm
// Day calibration state now held in per-arm filenames, so saving state does not overwrite
// state changes being done in a potenitally concurrent run on the other arm.
// typeToString added so saved state BIAS/DARK type is more readable.
//
// Revision 1.3  2009/08/14 14:12:30  cjm
// Amplifier setting now per-arm.
//
// Revision 1.2  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
