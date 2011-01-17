// LAMPFOCUSImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/LAMPFOCUSImplementation.java,v 1.7 2011-01-17 10:48:10 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;
import java.util.*;
import ngat.eip.*;
import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.LAMPFOCUS;
import ngat.message.ISS_INST.LAMPFOCUS_DONE;
import ngat.message.ISS_INST.FRODOSPEC_LAMPFOCUS;
import ngat.message.ISS_INST.FRODOSPEC_LAMPFOCUS_DONE;
import ngat.message.ISS_INST.FILENAME_ACK;
import ngat.message.INST_DP.*;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the LAMPFOCUS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.7 $
 */
public class LAMPFOCUSImplementation extends SETUPImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: LAMPFOCUSImplementation.java,v 1.7 2011-01-17 10:48:10 cjm Exp $");
	/**
	 * A small number. Used to prevent a division by zero.
	 */	 
	protected final static double NEARLY_ZERO = 0.00001;
	/**
	 * The focus stage we are moving.
	 * @see FocusStage
	 */
	protected FocusStage focusStage = null;
	/**
	 * Which arm we are focusing.
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected int arm;
	/**
	 * Which CCD to get exposures from.
	 * @see ngat.frodospec.ccd.CCDLibrary
	 */
	protected CCDLibrary ccd = null;
	/**
	 * Whether the CCD (SDSU) controller is enabled for comms.
	 */
	protected boolean ccdEnable = false;
	/**
	 * The exposure length to use, in milliseconds.
	 */
	protected int exposureLength = 0;
	/**
	 * Which lamp(s) we turn on for the focus run.
	 */
	protected String lampsString = null;
	/**
	 * This overhead is loaded from the <b>&quot;frodospec.lampfocus.ack_time.per_exposure_overhead&quot;</b>
	 * and represents the number of milliseconds overhead for every exposure (readout time, saving to
	 * disc etc). It is setup in the <i>init</i> method and used for sending
	 * acknowledgements with the correct time to complete.
	 */
	private int perExposureOverhead = 0;
	/**
	 * This overhead is loaded from the <b>&quot;frodospec.lampfocus.ack_time.reduce_overhead&quot;</b>
	 * and represents the number of milliseconds overhead for the reduction of each frame (de-biasing, saving to
	 * disc etc). It is setup in the <i>init</i> method and used for sending data pipeline
	 * acknowledgements with the correct time to complete.
	 */
	private int reduceOverhead = 0;

	/**
	 * Constructor.
	 */
	public LAMPFOCUSImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_LAMPFOCUS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_LAMPFOCUS";
	}

	/**
	 * This method is the first to be called in this class, to initialise variables needed
	 * when implementing LAMPFOCUS. 
	 * <ul>
	 * <li>It calls the superclass's init method.
	 * <li>It initialises perExposureOverhead and reduceOverhead by querying the status for them.
	 * </ul>
	 * @param command The command to be implemented.
	 * @see #perExposureOverhead
	 * @see #reduceOverhead
	 */
	public void init(COMMAND command)
	{
		super.init(command);
	// Get the amount of time to add to the exposure time for readout/setup
		try
		{
			perExposureOverhead = status.getPropertyInteger("frodospec.lampfocus.ack_time."+
									"per_exposure_overhead");
		}
		catch (Exception e)
		{
			perExposureOverhead = 20000;
			frodospec.error(this.getClass().getName()+":init:getting per exposure overhead failed:"+
					"using default value:"+perExposureOverhead,e);
		}
	// Get the amount of time to reduce each frame
		try
		{
			reduceOverhead = status.getPropertyInteger("frodospec.lampfocus.ack_time.reduce_overhead");
		}
		catch (Exception e)
		{
			reduceOverhead = 5000;
			frodospec.error(this.getClass().getName()+":init:getting reduce overhead failed:"+
					"using default value:"+reduceOverhead,e);
		}
	}

	/**
	 * This method returns the FRODOSPEC_LAMPFOCUS command's acknowledge time. 
	 * This currently returns the per exposure overhead plus default acknowledge time.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see #perExposureOverhead
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(perExposureOverhead+serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the FRODOSPEC_LAMPFOCUS command. 
	 * <ul>
	 * <li>The arm is retrieved from the command.
	 * <li>The start/end and step in focus, and lamps, are retrieved from config.
	 * <li>The PLC is queried to get the current resolution for the arm.
	 * <li>getArcExposureLength is called to get the exposure length from lamp/arm/resolution.
	 * <li>We initialise  status's exposure count/exposure number etc.
	 * <li>We call setLampLock to turn on the specified lamps and get hold of the lock.
	 * <li>We enter a loop through the focus settings to use:
	 *     <ul>
	 *     <li>We call setFocus to set the focus stage position.
	 *     <li>We call exposeFrame to do the exposure.
	 *     <li>We call sendFrameAcknowledge to keep the server connection alive.
	 *     </ul>
	 * <li>We call turnLampsOff, which turns the lamps off (by using the lamp controllers
	 *     clearLampLock method).
	 * <li>We enter a loop through the list of exposures just taken:
	 *     <ul>
	 *     <li>We call reduceFrame to pass the exposure to the data pipeline.
	 *     <li>We call sendDpAcknowledge to send the reduced filename to the client, and keep the connection alive.
	 *     <li>
	 *     </ul>
	 * <li>We call resetFocus to set the focus back to the set-point. When the data-pipeline
	 *     produces some measure of focus, we will replace this with a call to set the computed best focus.
	 * <li>We setup the done message object to be successful.
	 * </ul>
	 * @see #frodospec
	 * @see #focusStage
	 * @see #setFocus
	 * @see #exposeFrame
	 * @see #sendFrameAcknowledge
	 * @see #reduceFrame
	 * @see #sendDpAcknowledge
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#objectName
	 * @see FITSImplementation#getArcExposureLength
	 * @see FITSImplementation#turnLampsOff
	 * @see FrodoSpec#getLampUnit
	 * @see FrodoSpec#getLampController
	 * @see FrodoSpec#getPLC
	 * @see Plc#getGratingResolution
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 * @see ngat.frodospec.FrodoSpec#getLampController
	 * @see ngat.frodospec.LampController#setLampLock
	 * @see ngat.frodospec.LampController#clearLampLock
	 * @see ngat.fits.FitsHeaderDefaults#OBSTYPE_VALUE_LAMP_FLAT
	 * @see ngat.fits.FitsFilename#EXPOSURE_CODE_LAMP_FLAT
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_LAMPFOCUS lampFocusCommand = null;
		FRODOSPEC_LAMPFOCUS_DONE lampFocusDone = new FRODOSPEC_LAMPFOCUS_DONE(command.getId());
		FILENAME_ACK lampFocusAck = null;
		LAMPFOCUSFrameParameters frameParameters = null;
		FitsHeaderCardImage objectCardImage = null;
		Plc plc = null;
		String filename = null;
		int resolution,numberOfExposures;
		float startFocus, endFocus, focusStep;
		Vector list = null;
		int exposureNumber;

		if(testAbort(command,lampFocusDone) == true)
			return lampFocusDone;
		if((command instanceof FRODOSPEC_LAMPFOCUS) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Lamp Focus command has wrong class:"+
					command.getClass().getName());
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2800);
			lampFocusDone.setErrorString("Lamp Focus command has wrong class:"+
							    command.getClass().getName());
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		lampFocusCommand = (FRODOSPEC_LAMPFOCUS)command;
		arm = lampFocusCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Illegal arm:"+arm+".");
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2801);
			lampFocusDone.setErrorString("Illegal arm:"+arm+".");
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		// get per-arm config for focus ranges/steps and lamps
		startFocus = status.getPropertyFloat("frodospec.lampfocus.focus.start."+
						     FrodoSpecConstants.ARM_STRING_LIST[arm]);
		endFocus = status.getPropertyFloat("frodospec.lampfocus.focus.end."+
						   FrodoSpecConstants.ARM_STRING_LIST[arm]);
		focusStep = status.getPropertyFloat("frodospec.lampfocus.focus.step."+
						    FrodoSpecConstants.ARM_STRING_LIST[arm]);
		lampsString = status.getProperty("frodospec.lampfocus.lamp."+
						 FrodoSpecConstants.ARM_STRING_LIST[arm]);
		// get arm resolution 
		plc = frodospec.getPLC();
		try
		{
			resolution = plc.getGratingResolution("FRODOSPEC_LAMPFOCUS",
							      FrodoSpecConstants.ARM_STRING_LIST[arm],arm);
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to get Grating resolution.",e);
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2802);
			lampFocusDone.setErrorString("Failed to get Grating resolution:"+e);
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		// get exposure length from lamp/arm/resolution
		try
		{
			exposureLength = getArcExposureLength(lampsString,arm,resolution);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to get LAMPFLAT exposure length for lamps:"
					+lampsString+" arm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+" resolution:"+
					FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution],e);
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2803);
			lampFocusDone.setErrorString("Failed to get LAMPFOCUS exposure length for lamps:"
					+lampsString+" arm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+" resolution:"+
					FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution]+":"+e);
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		// get the focus stage to move
		focusStage = frodospec.getFocusStage(arm);
		// are we actually talking to the CCD
		ccdEnable = status.getPropertyBoolean("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".enable");
	// setup exposure status.
		if(focusStep < NEARLY_ZERO)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+command+":focus step too small:"+
					focusStep+".");
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2804);
			lampFocusDone.setErrorString("Focus step too small:"+focusStep+".");
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		numberOfExposures = (int)((endFocus-startFocus)/focusStep);
		status.setExposureCount(arm,numberOfExposures);
		status.setExposureNumber(arm,0);
		status.clearPauseResumeTimes();
		exposureNumber = 0;
		list = new Vector();
		if(testAbort(lampFocusCommand,lampFocusDone) == true)
			return lampFocusDone;
	// switch lamp on
	// We must do this before saving the FITS headers, if we want the right LAMPFLUX and LAMP<n>SET values.
		try
		{
			frodospec.getLampController().setLampLock("FRODOSPEC_LAMPFOCUS",
								  FrodoSpecConstants.ARM_STRING_LIST[arm],
								  arm,lampsString,serverConnectionThread);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to turn on lamps:"+lampsString,e);
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2805);
			lampFocusDone.setErrorString("Failed to turn on lamps:"+lampsString+":"+e);
			lampFocusDone.setSuccessful(false);
			return lampFocusDone;
		}
		// start exposure loop for each focus
		for(float focus=startFocus; focus <= endFocus; focus += focusStep)
		{
			status.setExposureNumber(arm,exposureNumber);
			// Clear pause and resume times.
			status.clearPauseResumeTimes();
			frameParameters = new LAMPFOCUSFrameParameters();
			list.add(frameParameters);
			// set focus stage focus
			if(setFocus(lampFocusCommand,lampFocusDone,focus) == false)
			{
				turnLampsOff("FRODOSPEC_LAMPFOCUS",FrodoSpecConstants.ARM_STRING_LIST[arm],
					     arm,lampFocusCommand,lampFocusDone);
				return lampFocusDone;
			}
			frameParameters.setFocus(focus);
			// do exposure
			if(exposeFrame(lampFocusCommand,lampFocusDone,exposureNumber,frameParameters) == false)
			{
				turnLampsOff("FRODOSPEC_LAMPFOCUS",FrodoSpecConstants.ARM_STRING_LIST[arm],
					     arm,lampFocusCommand,lampFocusDone);
				return lampFocusDone;
			}
			// send acknowledge
			if(sendFrameAcknowledge(lampFocusCommand,lampFocusDone,
						frameParameters.getFilename()) == false)
			{
				turnLampsOff("FRODOSPEC_LAMPFOCUS",FrodoSpecConstants.ARM_STRING_LIST[arm],
					     arm,lampFocusCommand,lampFocusDone);
				return lampFocusDone;
			}
			// increment frame number.
			exposureNumber++;
		}// end for on focus
		// switch lamp off
		if(turnLampsOff("FRODOSPEC_LAMPFOCUS",FrodoSpecConstants.ARM_STRING_LIST[arm],
				arm,lampFocusCommand,lampFocusDone) == false)
			return lampFocusDone;
	// reduce data
		for(int i = 0; i < list.size(); i++)
		{
			frameParameters = (LAMPFOCUSFrameParameters)list.get(i);
			if(testAbort(lampFocusCommand,lampFocusDone) == true)
				return lampFocusDone;
		// call pipeline
			if(reduceFrame(lampFocusCommand,lampFocusDone,frameParameters) == false)
				return lampFocusDone;
			if(testAbort(lampFocusCommand,lampFocusDone) == true)
				return lampFocusDone;
		// send data pipeline acknowledge
			if(sendDpAcknowledge(lampFocusCommand,lampFocusDone,frameParameters) == false)
				return lampFocusDone;
		}// end for on exposures
		// reset focus to set-point
		// eventually - reduction will calculate best position to use here.
		if(resetFocus(lampFocusCommand,lampFocusDone,resolution) == false)
			return lampFocusDone;
	// setup return values.
	// meanCounts and peakCounts set by pipelineProcess for last image reduced.
		lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		lampFocusDone.setErrorString("");
		lampFocusDone.setSuccessful(true);
		return lampFocusDone;
	}

	/**
	 * Move the focus stage to the specified position.
	 * @param lampFocusCommand The command being implemented.
	 * @param lampFocusDone The done command to be returned. If an error occurs, the error string and error number
	 *        are filled in.
	 * @param focus The focus position in mm.
	 * @return A boolean, true if the operation succeeds, and false if an error occurs (returned in lampFocusDone).
	 * @see #focusStage
	 * @see FocusStage#moveToPosition
	 */
	protected boolean setFocus(FRODOSPEC_LAMPFOCUS lampFocusCommand,FRODOSPEC_LAMPFOCUS_DONE lampFocusDone,
				   float focus)
	{
		try
		{
			focusStage.moveToPosition(lampFocusCommand.getClass().getName(),focus);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":setFocus:"+lampFocusCommand+":Failed to move focus to position:"+focus,e);
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2806);
			lampFocusDone.setErrorString("Failed to move focus to position:"+focus+":"+e);
			lampFocusDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Move the focus stage to the configured initial set-point.
	 * @param lampFocusCommand The command being implemented.
	 * @param lampFocusDone The done command to be returned. If an error occurs, the error string and error number
	 *        are filled in.
	 * @param resolution Whether to reset the focus to the low or high resolution focus position.
	 * @return A boolean, true if the operation succeeds, and false if an error occurs (returned in lampFocusDone).
	 * @see #focusStage
	 * @see FocusStage#moveToSetPoint
	 */
	protected boolean resetFocus(FRODOSPEC_LAMPFOCUS lampFocusCommand,FRODOSPEC_LAMPFOCUS_DONE lampFocusDone,
				     int resolution)
	{
		try
		{
			focusStage.moveToSetPoint(lampFocusCommand.getClass().getName(),resolution);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":resetFocus:"+lampFocusCommand+":Failed to move focus to set-point.",e);
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2806);
			lampFocusDone.setErrorString("Failed to move focus to set-point:"+e);
			lampFocusDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Method to take one frame needed for a LAMPFOCUS.
	 * <ul>
	 * <li>The filename is generated from the exposureNumber and the "frodospec.file.fits.path" property.
	 * <li>Any old files of this name are deleted.
	 * <li>The FITS headers are generated from FrodoSpec status data and ISS GET_FITS command 
	 * 	(clearFitsHeaders, setFitsHeaders, getFitsHeadersFromISS).
	 * <li>The FITS headers for this frame are saved using the saveFitsHeaders method.
	 * <li>sendBasicAck is called to ensure the client connection does not time out.
	 * <li>The exposure is performed and saved in the filename, using CCDExposureExpose.
	 * <li>The FITS lock file created by saveFitsHeaders is deleted using unLockFile.
	 * <li>The frameParameters filename field is set to the saved filename.
	 * </ul>
	 * testAbort is called during this method to see if the command has been aborted.
	 * @param lampFocusCommand The LAMPFOCUS command that is causing this exposure. It is passed
	 * 	to saveFitsHeaders and testAbort.
	 * @param lampFocusDone The instance of LAMPFOCUS_DONE. This is filled in with an error message if the
	 * 	exposure fails. It is passed to saveFitsHeaders and testAbort.
	 * @param exposureNumber The frame number. Used to generate the filename.
	 * @param frameParameters The frame parameters for this exposure. The filename is set in this,
	 * 	if the exposure is completed successfully.
	 * @return The method returns true if the exposure was completed successfully. Otherwise false is returned,
	 * 	and the error fields in telFocusDone are filled in.
	 * @see #arm
	 * @see #exposureLength
	 * @see #ccd
	 * @see #perExposureOverhead
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FITSImplementation#sendBasicAck
	 * @see #testAbort
	 * @see CCDLibrary#expose
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 */
	private boolean exposeFrame(COMMAND lampFocusCommand,COMMAND_DONE lampFocusDone,int exposureNumber,
				    LAMPFOCUSFrameParameters frameParameters)
	{
		FitsHeaderCardImage objectCardImage = null;
		File file = null;
		String directoryString = null;
		String filename = null;

	// get directory/filename
		directoryString = status.getProperty("frodospec.file.fits.path");
		if(directoryString.endsWith(System.getProperty("file.separator")) == false)
			directoryString = directoryString.concat(System.getProperty("file.separator"));
		filename = new String(directoryString+status.getProperty("frodospec.lampfocus.file")+
				      FrodoSpecConstants.ARM_STRING_LIST[arm]+exposureNumber+".fits");
	// delete old file if it exists
		file = new File(filename);
		if(file.exists())
			file.delete();
	// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(lampFocusCommand,lampFocusDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_EXPOSURE,
				  exposureLength) == false)
			return false;
		if(getFitsHeadersFromISS(lampFocusCommand,lampFocusDone,arm) == false)
			return false;
		if(testAbort(lampFocusCommand,lampFocusDone) == true)
			return false;
	// Modify "OBJECT" FITS header value to distinguish between spectra of the OBJECT
        // and calibration LAMPFLATs taken for that observation.
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectCardImage.setValue(new String("LAMPFOCUS: "+lampsString));
	// save FITS headers
		if(saveFitsHeaders(lampFocusCommand,lampFocusDone,arm,filename) == false)
		{
			unLockFile(lampFocusCommand,lampFocusDone,filename);
			return false;
		}
		if(testAbort(lampFocusCommand,lampFocusDone) == true)
		{
			unLockFile(lampFocusCommand,lampFocusDone,filename);
			return false;
		}
		// send ACK so connection does not time out
		if(sendBasicAck(lampFocusCommand,lampFocusDone,exposureLength+perExposureOverhead) == false)
		{
			unLockFile(lampFocusCommand,lampFocusDone,filename);
			return false;
		}
	// do exposure
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				ccd.expose("LAMPFOCUS",FrodoSpecConstants.ARM_STRING_LIST[arm],
					   true,-1,exposureLength,filename);
			}
			catch(CCDLibraryNativeException e)
			{
				unLockFile(lampFocusCommand,lampFocusDone,filename);
				frodospec.error(this.getClass().getName()+
						":processCommand:"+lampFocusCommand+":expose failed:"+e.toString());
				lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2807);
				lampFocusDone.setErrorString(e.toString());
				lampFocusDone.setSuccessful(false);
				return false;
			}
		}
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,this.getClass().getName()+
				      ":processCommand:Did not do lamp focus exposure, ccd enable was false.");
		}
		// unlock FITS file lock created by saveFitsHeaders
		if(unLockFile(lampFocusCommand,lampFocusDone,filename) == false)
			return false;
		if(testAbort(lampFocusCommand,lampFocusDone) == true)
			return false;
		frameParameters.setFilename(filename);
		status.setExposureNumber(arm,exposureNumber+1);
		return true;
	}

	/**
	 * Method to send an acknowledgement back to the ISS, after a frame has been exposed.
	 * An instance of FILENAME_ACK is constructed and returned to the client (the ISS).
	 * The filename sent is the one passed to this method, i.e. the last frame exposed.
	 * The timeToComplete is set to the frame <b>exposureLength</b> plus the <b>perExposureOverhead</b>
	 * plus the default acknowledge time.
	 * @param lampFocusCommand The instance of COMMAND that caused this LAMPFOCUS to occur.
	 * @param lampFocusDone The DONE command that will be sent back to the ISS. Filled in with
	 * 	an error message if this method fails.
	 * @param filename The filename of the frame just exposed, to be returned to the ISS in the ACK.
	 * @return The method returns true if it was successful, if it fails (sendAcknowledge throws an
	 * 	IOException) false is returned, and an error is sent to the log, and telFocusDone has
	 * 	it's error flag/message set accordingly.
	 * @see #exposureLength
	 * @see #perExposureOverhead
	 * @see #serverConnectionThread
	 * @see #frodospec
	 */
	private boolean sendFrameAcknowledge(COMMAND lampFocusCommand,COMMAND_DONE lampFocusDone,String filename)
	{
		FILENAME_ACK lampFocusAck = null;

	// send acknowledge to say frame is completed.
		lampFocusAck = new FILENAME_ACK(lampFocusCommand.getId());
		lampFocusAck.setTimeToComplete(exposureLength+perExposureOverhead+
					       serverConnectionThread.getDefaultAcknowledgeTime());
		lampFocusAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(lampFocusAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
				":processCommand:sendFrameAcknowledge:"+lampFocusCommand+":"+e.toString());
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2808);
			lampFocusDone.setErrorString(e.toString());
			lampFocusDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Method to reduce a frame.
	 * @param lampFocusCommand The LAMPFOCUS command that is caused the frame reduction to occur. The Id is used
	 * 	as the EXPOSE_REDUCE command's id.
	 * @param lampFocusDone The instance of LAMPFOCUS_DONE. This is filled in with an error message if the
	 * 	EXPOSE_REDUCE fails.
	 * @param frameParameters The frame parameters for this frame. The filename to reduce is retrieved from this.
	 * 	It's other parameters are set (including reducedFlename) with the EXPOSE_REDUCE return values.
	 * @return The method returns true if the reduction was successfull. Otherwise it returns false,
	 * 	and lampFocusDone's error fields are set.
	 * @see FrodoSpec#sendDpRtCommand
	 * @see #testAbort
	 * @see ngat.message.INST_DP.EXPOSE_REDUCE
	 */
	private boolean reduceFrame(COMMAND lampFocusCommand,COMMAND_DONE lampFocusDone,
				    LAMPFOCUSFrameParameters frameParameters)
	{
		EXPOSE_REDUCE reduceCommand = null;
		EXPOSE_REDUCE_DONE reduceDone = null;
		INST_TO_DP_DONE instToDPDone = null;

		reduceCommand = new EXPOSE_REDUCE(lampFocusCommand.getId());
		reduceCommand.setFilename(frameParameters.getFilename());
		instToDPDone = frodospec.sendDpRtCommand(reduceCommand,serverConnectionThread);
		if(instToDPDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":reduceFrame:"+reduceCommand+":"+
					instToDPDone.getErrorNum()+":"+instToDPDone.getErrorString());
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2809);
			lampFocusDone.setErrorString(instToDPDone.getErrorString());
			lampFocusDone.setSuccessful(false);
			return false;
		}
		if(testAbort(lampFocusCommand,lampFocusDone) == true)
			return false;
	// Copy the DP REDUCE DONE parameters to the frameParameters list
		if(instToDPDone instanceof EXPOSE_REDUCE_DONE)
		{
			reduceDone = (EXPOSE_REDUCE_DONE)instToDPDone;

			frameParameters.setReducedFilename(reduceDone.getFilename());
		}
		else
		{
			frameParameters.setReducedFilename(null);
			frodospec.error(this.getClass().getName()+":reduceFrame:"+reduceCommand+
					":Done messsage not sub-class of EXPOSE_REDUCE_DONE");
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE);
			lampFocusDone.setErrorString(this.getClass().getName()+":reduceFrame:"+reduceCommand+
						     ":Done messsage not sub-class of EXPOSE_REDUCE_DONE");
			lampFocusDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Method to send an acknowledgement back to the ISS, after a frame has been reduced.
	 * An instance of LAMPFOCUS_DP_ACK is constructed and returned to the client (the ISS).
	 * The filename sent is the reduced filename in frameParameters.
	 * The timeToComplete is set to the the <b>perExposureOverhead</b> plus the default acknowledge time.
	 * The other fields are copied from the frameParameters argument.
	 * @param lampFocusCommand The instance of COMMAND that caused this LAMPFOCUS to occur.
	 * @param lampFocusDone The DONE command that will be sent back to the ISS. Filled in with
	 * 	an error message if this method fails.
	 * @param frameParameters The frame parameters for this frame. The following fields are retrieved and copied
	 * 	into the LAMPFOCUS_DP_ACK, reducedFilename, . They should have been
	 * 	set by the reduceFrame method.
	 * @return The method returns true if it succeeded. It returns false if it failed (an IOException occured
	 * 	whilst sending the ACK), an error is logged and telFocusDone is filled in with the relevant error
	 * 	code/string.
	 * @see #exposureLength
	 * @see #perExposureOverhead
	 * @see #serverConnectionThread
	 */
	private boolean sendDpAcknowledge(COMMAND lampFocusCommand,COMMAND_DONE lampFocusDone,
					  LAMPFOCUSFrameParameters frameParameters)
	{
		// should be LAMPFOCUS_DP_ACK containing extra information?
		FILENAME_ACK lampFocusDpAck = null;

	// send acknowledge to say frame is completed.
		lampFocusDpAck = new FILENAME_ACK(lampFocusCommand.getId());
		lampFocusDpAck.setTimeToComplete(exposureLength+perExposureOverhead+
						 serverConnectionThread.getDefaultAcknowledgeTime());
		lampFocusDpAck.setFilename(frameParameters.getReducedFilename());

		try
		{
			serverConnectionThread.sendAcknowledge(lampFocusDpAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:sendDpAcknowledge:"+lampFocusCommand+":"+e.toString());
			lampFocusDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2810);
			lampFocusDone.setErrorString(e.toString());
			lampFocusDone.setSuccessful(false);
			return false;
		}
		return true;
	}

	/**
	 * Inner class for LAMPFOCUS. Stores all the data needed for each frame of the LAMPFOCUS.
	 */
	class LAMPFOCUSFrameParameters
	{
		/**
	 	 * The focus used for this frame.
	 	 */
		private float focus;
		/**
	 	 * The filename used for this frame.
	 	 */
		private String filename;
		/**
	 	 * The filename used for this frame, after it has been reduced.
	 	 */
		private String reducedFilename;

		/**
	 	 * Default constructor.
	 	 */
		public LAMPFOCUSFrameParameters()
		{
			focus = 0.0f;
			filename = null;
			reducedFilename = null;
		}

		/**
		 * Set method for focus.
		 */
		public void setFocus(float f)
		{
			focus = f;
		}

		/**
		 * Get method for focus.
		 */
		public float getFocus()
		{
			return focus;
		}

		/**
		 * Set method for filename.
		 */
		public void setFilename(String s)
		{
			filename = s;
		}

		/**
		 * Get method for filename.
		 */
		public String getFilename()
		{
			return filename;
		}

		/**
		 * Set method for reduced filename.
		 */
		public void setReducedFilename(String s)
		{
			reducedFilename = s;
		}

		/**
		 * Get method for reduced filename.
		 */
		public String getReducedFilename()
		{
			return reducedFilename;
		}
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.6  2011/01/12 11:50:03  cjm
// Adding clazz and source logging to PLC/Lamp API.
//
// Revision 1.5  2011/01/05 14:07:42  cjm
// Added class argument to focusStage.getPosition to improve logging.
// Fixed resetFocus. Fixed comments.
//
// Revision 1.4  2010/06/14 16:29:22  cjm
// Added sendBasicAck to exposeFrame, so longer Arc exposure lengths (60s) do not cause the GUI to time out.
//
// Revision 1.3  2010/03/15 16:48:22  cjm
// Removed stowFold call - now lamp unit has it's own mirror.
//
// Revision 1.2  2010/02/08 11:09:30  cjm
// Added unLockFile calls as saveFitsHeaders now creates FITS file locks.
//
// Revision 1.1  2009/05/07 16:05:47  cjm
// Initial revision
//
//
