// CONFIGImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/CONFIGImplementation.java,v 1.12 2011-01-17 10:48:10 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import ngat.frodospec.ccd.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.CONFIG;
import ngat.message.ISS_INST.CONFIG_DONE;
import ngat.message.ISS_INST.OFFSET_FOCUS;
import ngat.message.ISS_INST.OFFSET_FOCUS_DONE;
import ngat.message.ISS_INST.INST_TO_ISS_DONE;
import ngat.phase2.*;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the CONFIG command sent to a server using the
 * Java Message System. It extends SETUPImplementation.
 * @see SETUPImplementation
 * @author Chris Mottram
 * @version $Revision: 1.12 $
 */
public class CONFIGImplementation extends SETUPImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: CONFIGImplementation.java,v 1.12 2011-01-17 10:48:10 cjm Exp $");
	/**
	 * Constructor. 
	 */
	public CONFIGImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.CONFIG&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.CONFIG";
	}

	/**
	 * This method gets the CONFIG command's acknowledge time.
	 * This can take a long time to move the filter wheels to the required position.
	 * This method returns an ACK with timeToComplete set to the &quot; frodospec.config.acknowledge_time &quot;
	 * held in the configuration file. If this cannot be found/is not a valid number the default acknowledge
	 * time is used instead.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set to a time (in milliseconds).
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;
		int timeToComplete = 0;

		acknowledge = new ACK(command.getId());
		try
		{
			timeToComplete += frodospec.getStatus().getPropertyInteger("frodospec.config.acknowledge_time");
		}
		catch(NumberFormatException e)
		{
			frodospec.error(this.getClass().getName()+":calculateAcknowledgeTime:"+e);
			timeToComplete += serverConnectionThread.getDefaultAcknowledgeTime();
		}
		acknowledge.setTimeToComplete(timeToComplete);
		return acknowledge;
	}

	/**
	 * This method implements the CONFIG command. 
	 * <ul>
	 * <li>It checks the message contains a suitable FrodoSpecConfig object to configure the controller.
	 * <li>It gets the number of rows and columns from the loaded properties file.
	 * <li>It gets binning information from the FrodoSpecConfig object passed with the command.
	 * <li>It gets windowing information from the FrodoSpecConfig object passed with the command.
	 * <li>It sends the information to the SDSU CCD Controller to configure it.
	 * <li>If configured, it configures the grating using the PLC.
	 * <li>It issues an OFFSET_FOCUS commmand to the ISS based on the optical thickness of the filter(s).
	 * <li>It increments the unique configuration ID.
	 * </ul>
	 * An object of class CONFIG_DONE is returned. If an error occurs a suitable error message is returned.
	 * @see #setFocusOffset
	 * @see ngat.phase2.CCDConfig
	 * @see FrodoSpecStatus#getNumberColumns
	 * @see FrodoSpecStatus#getNumberRows
	 * @see FrodoSpecStatus#getPropertyInteger
	 * @see FrodoSpecStatus#incConfigId
	 * @see FrodoSpecStatus#setConfigName
	 * @see FrodoSpecStatus#setConfigCalibrateBefore
	 * @see FrodoSpecStatus#setConfigCalibrateAfter
	 * @see FrodoSpec#getPLC
	 * @see Plc#setGrating
	 * @see ngat.frodospec.ccd.CCDLibrary#dspDeinterlaceFromString
	 * @see ngat.frodospec.ccd.CCDLibrary#setupDimensions
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		CONFIG configCommand = null;
		FrodoSpecConfig frodospecConfig = null;
		Detector detector = null;
		CONFIG_DONE configDone = null;
		CCDLibrary ccd = null;
		CCDLibrarySetupWindow windowList[] = new CCDLibrarySetupWindow[CCDLibrary.SETUP_WINDOW_COUNT];
		Plc plc = null;
		FocusStage focusStage = null;
		int numberColumns,numberRows,amplifier,deInterlaceSetting,arm;
		boolean ccdEnable,calibrateBefore,calibrateAfter;

		frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",null,this.getClass().getName()+":Command:"+
			     command.getClass().getName()+":processCommand:started.");
	// test contents of command.
		configCommand = (CONFIG)command;
		configDone = new CONFIG_DONE(command.getId());
		if(testAbort(configCommand,configDone) == true)
			return configDone;
		if(configCommand.getConfig() == null)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":Config was null.");
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+800);
			configDone.setErrorString(":Config was null.");
			configDone.setSuccessful(false);
			return configDone;
		}
		if((configCommand.getConfig() instanceof FrodoSpecConfig) == false)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
				command+":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+801);
			configDone.setErrorString(":Config has wrong class:"+
				configCommand.getConfig().getClass().getName());
			configDone.setSuccessful(false);
			return configDone;
		}
	// get frodospecConfig from configCommand.
		frodospecConfig = (FrodoSpecConfig)configCommand.getConfig();
	// which arm?
		arm = frodospecConfig.getArm();
	// Which instance of the CCD library.
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
				command+":Arm has illegal value:"+arm);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+809);
			configDone.setErrorString(":Arm has illegal value:"+arm);
			configDone.setSuccessful(false);
			return configDone;
		}
	// get local detector copy
		detector = frodospecConfig.getDetector(0);
	// set get calibrateBefore and calibrateAfter
		calibrateBefore = frodospecConfig.getCalibrateBefore();
		calibrateAfter = frodospecConfig.getCalibrateAfter();
	// load other required config for dimension configuration from FrodoSpec properties file.
		try
		{
			numberColumns = status.getNumberColumns(detector.getXBin());
			numberRows = status.getNumberRows(detector.getYBin());
	                amplifier = getAmplifier(arm);
			deInterlaceSetting = getDeInterlaceSetting(arm);
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+frodospecConfig.armToString()+
							      ".enable");
		}
	// CCDLibraryFormatException is caught and re-thrown by this method.
	// Other exceptions (IllegalArgumentException,NumberFormatException) are not caught here, 
	// but by the calling method catch(Exception e)
		catch(CCDLibraryFormatException e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
					command+":",e);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+802);
			configDone.setErrorString("processCommand:"+command+":"+e);
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// check xbin and ybin: greater than zero (hardware restriction), 
	// xbin == ybin (pipeline restriction) - commented out for testing for now
		if((detector.getXBin()<1)||(detector.getYBin()<1))
			// ||(detector.getXBin()!=detector.getYBin())
		{
			String errorString = null;

			errorString = new String("Illegal xBin and yBin:xBin="+detector.getXBin()+",yBin="+
						detector.getYBin());
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":"+errorString);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+807);
			configDone.setErrorString(errorString);
			configDone.setSuccessful(false);
			return configDone;
		}
	// We can either bin, or window, but not both at once. 
		if((detector.getWindowFlags() > 0) && ((detector.getXBin() > 1)||(detector.getYBin() > 1)))
		{
			String errorString = null;

			errorString = new String("Illegal binning and windowing:xBin="+detector.getXBin()+",window="+
						detector.getWindowFlags());
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":"+errorString);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+808);
			configDone.setErrorString(errorString);
			configDone.setSuccessful(false);
			return configDone;
		}
	// setup window list from ccdConfig.
		for(int i = 0; i < detector.getMaxWindowCount(); i++)
		{
			Window w = null;

			if(detector.isActiveWindow(i))
			{
				w = detector.getWindow(i);
				if(w == null)
				{
					String errorString = new String("Window "+i+" is null.");

					frodospec.error(this.getClass().getName()+":processCommand:"+
							command+":"+errorString);
					configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+803);
					configDone.setErrorString(errorString);
					configDone.setSuccessful(false);
					return configDone;
				}
				windowList[i] = new CCDLibrarySetupWindow(w.getXs(),w.getYs(),
									  w.getXe(),w.getYe());
			}
			else
			{
				windowList[i] = new CCDLibrarySetupWindow(-1,-1,-1,-1);
			}
		}
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// send dimension/configuration to the SDSU controller
		try
		{
			if(ccdEnable)
			{
				frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",
					      FrodoSpecConstants.ARM_STRING_LIST[arm],
					      this.getClass().getName()+":processCommand:Calling setupDimensions.");
				ccd.setupDimensions("CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
						    numberColumns,numberRows,detector.getXBin(),detector.getYBin(),
						    amplifier,deInterlaceSetting,detector.getWindowFlags(),windowList);
				frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",
					      FrodoSpecConstants.ARM_STRING_LIST[arm],this.getClass().getName()+
					      ":processCommand:Finished setupDimensions.");
			}
			else
			{
				frodospec.log(Logger.VERBOSITY_VERY_TERSE,"CONFIG",
					      FrodoSpecConstants.ARM_STRING_LIST[arm],
					      this.getClass().getName()+
					      ":processCommand:CCD not enabled:CCD library NOT configured.");
			}
			if(testAbort(configCommand,configDone) == true)
				return configDone;
		}
		catch(CCDLibraryNativeException e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":",e);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+804);
			configDone.setErrorString(":processCommand:"+command+":"+e);
			configDone.setSuccessful(false);
			return configDone;
		}
       // send grating configuration to the PLC
		try
		{
			frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Finding PLC.");
			plc = frodospec.getPLC();
			frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Setting arm "+arm+" to resolution "+
				      frodospecConfig.getResolution()+".");
			plc.setGrating("CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
				       arm,frodospecConfig.getResolution());
			frodospec.log(Logger.VERBOSITY_VERBOSE,"CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Grating set.");
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":",e);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+810);
			configDone.setErrorString(":processCommand:Set grating position:"+command+":"+e);
			configDone.setSuccessful(false);
			return configDone;
		}
		// set focus stage dependant on resolution
		try
		{
			frodospec.log(Logger.VERBOSITY_VERBOSE,
				      "CONFIG",FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Getting focus stage for arm "+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+".");
			focusStage = frodospec.getFocusStage(arm);
			frodospec.log(Logger.VERBOSITY_VERBOSE,
				      command.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Moving focus stage for arm "+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+" and resolution "+
				      FrodoSpecConstants.RESOLUTION_STRING_LIST[frodospecConfig.getResolution()]+".");
			focusStage.moveToSetPoint(configCommand.getClass().getName(),frodospecConfig.getResolution());
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":",e);
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+811);
			configDone.setErrorString(":processCommand:Set focus stage:"+command+":"+e);
			configDone.setSuccessful(false);
			return configDone;
		}
	// test abort
		if(testAbort(configCommand,configDone) == true)
			return configDone;
	// Issue ISS OFFSET_FOCUS commmand
		frodospec.log(Logger.VERBOSITY_VERBOSE,
			      command.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
			      this.getClass().getName()+":processCommand:Setting focus Offset.");
		if(setFocusOffset(configCommand.getId(),frodospecConfig,status,configDone) == false)
			return configDone;
	// Increment unique config ID.
	// This is queried when saving FITS headers to get the CONFIGID value.
		try
		{
			frodospec.log(Logger.VERBOSITY_VERBOSE,
				      command.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				      this.getClass().getName()+":processCommand:Incrementing Config Id.");
			status.incConfigId(arm);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
					command+":Incrementing configuration ID:"+e.toString());
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+806);
			configDone.setErrorString("Incrementing configuration ID:"+e.toString());
			configDone.setSuccessful(false);
			return configDone;
		}
		frodospec.log(Logger.VERBOSITY_VERBOSE,
			      command.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
			      this.getClass().getName()+":processCommand:Setting cached status details.");
	// Store name of configuration used in status object.
	// This is queried when saving FITS headers to get the CONFNAME value.
		status.setConfigName(arm,frodospecConfig.getId());
	// store cached calibrateBefore/After
		status.setConfigCalibrateBefore(arm,calibrateBefore);
		status.setConfigCalibrateAfter(arm,calibrateAfter);
	// setup return object.
		configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		configDone.setErrorString("");
		configDone.setSuccessful(true);
		frodospec.log(Logger.VERBOSITY_VERBOSE,this.getClass().getName()+
			      ":processCommand:Finished.");
	// return done object.
		return configDone;
	}

	/**
	 * Routine to set the telescope focus offset. Sends a OFFSET_FOCUS command to
	 * the ISS. The OFFSET_FOCUS sent is the total of the selected filter's optical thickness.
	 * The sendISSCommand command is synchronised by the getFocusOffsetLock lock object,
	 * to stop both arms sending OFFSET_FOCUS at the same time, which causes the TCS to issue an error message.
	 * @param id The Id is used as the OFFSET_FOCUS command's id.
	 * @param frodospecConfig The configuration to attain.
	 * @param status A reference to the FrodoSpec's status object, which contains the filter database.
	 * @param configDone The instance of CONFIG_DONE. This is filled in with an error message if the
	 * 	OFFSET_FOCUS fails.
	 * @return The method returns true if the telescope attained the focus offset, otherwise false is
	 * 	returned an telFocusDone is filled in with an error message.
	 * @see FrodoSpecStatus#getFocusOffsetLock
	 * @see FrodoSpec#sendISSCommand
	 * @see #status
	 * @see #frodospec
	 */
	private boolean setFocusOffset(String id,FrodoSpecConfig frodospecConfig,FrodoSpecStatus status,
				       CONFIG_DONE configDone)
	{
		OFFSET_FOCUS focusOffsetCommand = null;
		INST_TO_ISS_DONE instToISSDone = null;
		String filterIdName = null;
		String filterTypeString = null;
		float focusOffset = 0.0f;
		Object focusOffsetLock = null;

		focusOffsetCommand = new OFFSET_FOCUS(id);
		focusOffset = 0.0f;
	// get default focus offset
		focusOffset += status.getPropertyDouble("frodospec.focus.offset");
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,frodospecConfig.getClass().getName(),null,
			      this.getClass().getName()+":setFocusOffset:Master offset is "+focusOffset+".");
	// set the commands focus offset
		focusOffsetCommand.setFocusOffset(focusOffset);
		frodospec.log(Logger.VERBOSITY_VERY_TERSE,frodospecConfig.getClass().getName(),null,
			      this.getClass().getName()+":setFocusOffset:Total offset is "+focusOffset+".");
       // Get a synchronisation lock
		focusOffsetLock = status.getFocusOffsetLock();
       // do the focus offset
		synchronized(focusOffsetLock)
		{
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,frodospecConfig.getClass().getName(),null,
				      this.getClass().getName()+":setFocusOffset:Acquired lock.");
			instToISSDone = frodospec.sendISSCommand(focusOffsetCommand,serverConnectionThread);
			frodospec.log(Logger.VERBOSITY_INTERMEDIATE,frodospecConfig.getClass().getName(),null,
				      this.getClass().getName()+":setFocusOffset:Command returned:"+
				      instToISSDone.getSuccessful());
		}
		if(instToISSDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":focusOffset failed:"+focusOffset+":"+
				instToISSDone.getErrorString());
			configDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+805);
			configDone.setErrorString(instToISSDone.getErrorString());
			configDone.setSuccessful(false);
			return false;
		}
		return true;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.11  2011/01/12 11:50:03  cjm
// Adding clazz and source logging to PLC/Lamp API.
//
// Revision 1.10  2011/01/05 14:07:42  cjm
// Added class argument to focusStage.getPosition to improve logging.
//
// Revision 1.9  2010/11/25 14:08:35  cjm
// Removed restrictions on square binning. This so we can test binning in the spectral direction.
//
// Revision 1.8  2010/08/03 09:24:20  cjm
// Added calls to move focus stage to per-resolution based setpoint.
//
// Revision 1.7  2009/10/20 18:15:16  cjm
// Added logging.
//
// Revision 1.6  2009/08/14 14:12:41  cjm
// Amplifier setting now per-arm.
//
// Revision 1.5  2009/08/06 13:41:45  cjm
// Added focousOffsetLock to setFocusOffset, to stop both arms calling setFocusOffset at the same time,
// which can cause the TCS to return an error to the first one.
//
// Revision 1.4  2009/05/07 15:36:26  cjm
// Fixed comment.
//
// Revision 1.3  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.2  2008/11/28 11:16:14  cjm
// CONFIGID / incConfigId now per-arm.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
