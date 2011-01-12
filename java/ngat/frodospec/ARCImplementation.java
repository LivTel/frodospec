// ARCImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ARCImplementation.java,v 1.10 2011-01-12 11:50:03 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.Vector;
import ngat.eip.*;
import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.ARC;
import ngat.message.ISS_INST.ARC_DONE;
import ngat.message.ISS_INST.FRODOSPEC_ARC;
import ngat.message.ISS_INST.FRODOSPEC_ARC_DONE;
import ngat.message.ISS_INST.FILENAME_ACK;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the ARC command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.10 $
 */
public class ARCImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: ARCImplementation.java,v 1.10 2011-01-12 11:50:03 cjm Exp $");
	/**
	 * Constructor.
	 */
	public ARCImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_ARC&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_ARC";
	}

	/**
	 * This method returns the FRODOSPEC_ARC command's acknowledge time. 
	 * The frame in the FRODOSPEC_ARC takes 
	 * the configured exposure time plus the default acknowledge time to complete. The default acknowledge time
	 * allows time to setup the camera, get information about the telescope and save the frame to disk.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		FRODOSPEC_ARC arcCommand = (FRODOSPEC_ARC)command;
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		// diddly get exposure time from lamp
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the FRODOSPEC_ARC command. 
	 * <ul>
	 * <li>The arm is extracted from the command.
	 * <li>The PLC's getGratingResolution is called on the relevant arm to find the selected resolution.
	 * <li>We call getArcExposureLength to get the exposure length for that arm/resolution combination.
	 * <li>sendBasicAck sends an Ack back to the client, to stop the connection timing out before the exposure
	 *     is finished.
	 * <li>The setLampLock method in LampController is called. This waits in a loop until the lamp
	 *     can be switched on (synchronised in theory with operations on the other arm), 
	 *     acquires a lock for the specified lamp and then turns it on.
	 * <li>FITS headers for the specified arm are setup with calls to setFitsHeaders and getFitsHeadersFromISS.
	 * <li>The OBJECT FITS header is set to indicate the fact that this is an ARC.
	 * <li>The exposure code, multrun number and run number are set, and a new FITS filename generated.
	 * <li>We resend another ACK to the client to ensure the connection is kept alive 
	 *     whilst we actually take the exposure.
	 * <li>The exposure is taken and saved to the FITS filename.
	 * <li>turnLampsOff is called to turn the Arc lamp off.
	 * <li>unLockFile is called to remove the FITS file lock created by saveFitsHeaders.
	 * <li>A FILENAME_ACK is sent back to the client with the new filename.
	 * <li>reduceCalibrate is called to reduce the arc.
	 * </ul>
	 * @see #getArcExposureLength
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FITSImplementation#objectName
	 * @see CALIBRATEImplementation#sendBasicAck
	 * @see FrodoSpec#getLampUnit
	 * @see FrodoSpec#getLampController
	 * @see FrodoSpec#getPLC
	 * @see Plc#getGratingResolution
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 * @see ngat.frodospec.FrodoSpec#getLampController
	 * @see ngat.frodospec.LampController#setNoLampLock
	 * @see ngat.frodospec.LampController#clearLampLock
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_ARC arcCommand = null;
		FRODOSPEC_ARC_DONE arcDone = new FRODOSPEC_ARC_DONE(command.getId());
		FILENAME_ACK arcAck = null;
		ACK acknowledge = null;
		FitsHeaderCardImage objectCardImage = null;
		CCDLibrary ccd = null;
		Plc plc = null;		
		String lampsString = null;
		String filename = null;
		int exposureLength,arm,resolution;
		boolean ccdEnable;

		if(testAbort(command,arcDone) == true)
			return arcDone;
		if((command instanceof FRODOSPEC_ARC) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Arc command has wrong class:"+
					command.getClass().getName());
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1500);
			arcDone.setErrorString("Arc command has wrong class:"+
							    command.getClass().getName());
			arcDone.setSuccessful(false);
			return arcDone;
		}
		arcCommand = (FRODOSPEC_ARC)command;
		arm = arcCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Illegal arm:"+arm+".");
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1501);
			arcDone.setErrorString("Illegal arm:"+arm+".");
			arcDone.setSuccessful(false);
			return arcDone;
		}
		// get arm resolution 
		plc = frodospec.getPLC();
		try
		{
			resolution = plc.getGratingResolution(this.getClass().getName(),
							      FrodoSpecConstants.ARM_STRING_LIST[arm],arm);
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to get Grating resolution.",e);
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1506);
			arcDone.setErrorString("Failed to get Grating resolution:"+e);
			arcDone.setSuccessful(false);
			return arcDone;
		}
		// get lamp and exposure length
		lampsString = arcCommand.getLamp();
		try
		{
			exposureLength = getArcExposureLength(lampsString,arm,resolution);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to get ARC exposure length for lamps:"
					+lampsString+" arm:"+FrodoSpecConstants.ARM_STRING_LIST[arm]+" resolution:"+
					FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution],e);
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1502);
			arcDone.setErrorString("Failed to get ARC exposure length for lamps:"
					+lampsString+":"+e);
			arcDone.setSuccessful(false);
			return arcDone;
		}
		// send a basic Ack to keep the connection alive whilst we do an exposure
		if(sendBasicAck(arcCommand,arcDone,
				exposureLength+serverConnectionThread.getDefaultAcknowledgeTime()) == false)
			return arcDone;
		// are we actually talking to the CCD
		ccdEnable = status.getPropertyBoolean("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".enable");
	// setup exposure status.
		status.setExposureCount(arm,1);
		status.setExposureNumber(arm,0);
		status.clearPauseResumeTimes();
	// switch lamp on
	// We must do this before saving the FITS headers, if we want the right LAMPFLUX and LAMP<n>SET values.
		try
		{
			frodospec.getLampController().setLampLock(this.getClass().getName(),
								  FrodoSpecConstants.ARM_STRING_LIST[arm],
								  arm,lampsString,serverConnectionThread);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to turn on lamps:"+lampsString,e);
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1507);
			arcDone.setErrorString("Failed to turn on lamps:"+lampsString+":"+e);
			arcDone.setSuccessful(false);
			return arcDone;
		}
		if(testAbort(arcCommand,arcDone) == true)
		{
			// turn lamps off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			return arcDone;
		}
	// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(arcCommand,arcDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_ARC,exposureLength) == false)
		{
			// turn lamps off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			return arcDone;
		}
		if(getFitsHeadersFromISS(arcCommand,arcDone,arm) == false)
		{
			// turn lamps off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			return arcDone;
		}
		if(testAbort(arcCommand,arcDone) == true)
		{
			// turn lamps off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			return arcDone;
		}
	// Modify "OBJECT" FITS header value to distinguish between spectra of the OBJECT
        // and calibration ARCs taken for that observation.
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectCardImage.setValue(new String("ARC: "+lampsString+" for "+objectName));
	// don't do calibrate before
	// setup filename object
		frodospecFilenameList[arm].nextMultRunNumber();
		try
		{
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_ARC);
		}
		catch(Exception e)
		{
			// switch lamp off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			frodospec.error(this.getClass().getName()+
				  ":processCommand:"+command+":"+e.toString());
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1503);
			arcDone.setErrorString(e.toString());
			arcDone.setSuccessful(false);
			return arcDone;
		}
	// get a new filename.
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
	// save FITS headers
		if(saveFitsHeaders(arcCommand,arcDone,arm,filename) == false)
		{
			// switch lamp off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			unLockFile(arcCommand,arcDone,filename);
			return arcDone;
		}
        // send ack of exposurelength + readout before starting exposure
		if(sendBasicAck(arcCommand,arcDone,
				exposureLength+serverConnectionThread.getDefaultAcknowledgeTime()) == false)
		{
			// switch lamp off
			turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,arcCommand,arcDone);
			unLockFile(arcCommand,arcDone,filename);
			return arcDone;
		}
	// do arc
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				// diddly window 1 filename only
				ccd.expose(true,-1,exposureLength,filename);
			}
			catch(CCDLibraryNativeException e)
			{
				// switch lamp off
				turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
					     arm,arcCommand,arcDone);
				unLockFile(arcCommand,arcDone,filename);
				frodospec.error(this.getClass().getName()+
						":processCommand:"+command+":"+e.toString());
				arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1504);
				arcDone.setErrorString(e.toString());
				arcDone.setSuccessful(false);
				return arcDone;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+
				      ":processCommand:Did not do arc exposure, ccd enable was false.");
		}
		// switch lamp off
		if(turnLampsOff(this.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				arm,arcCommand,arcDone) == false)
		{
			return arcDone;
		}
		// unlock FITS file lock created by saveFitsHeaders
		if(unLockFile(arcCommand,arcDone,filename) == false)
			return arcDone;
		status.setExposureNumber(arm,1);
		// send acknowledge to say frame is completed.
		arcAck = new FILENAME_ACK(command.getId());
		arcAck.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		arcAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(arcAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:sendAcknowledge:"+command+":"+e.toString());
			arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1505);
			arcDone.setErrorString(e.toString());
			arcDone.setSuccessful(false);
			return arcDone;
		}
	// call pipeline to process data and get results
		if(reduceCalibrate(arcCommand,arcDone,filename) == false)
			return arcDone;
	// setup return values.
	// meanCounts and peakCounts set by pipelineProcess for last image reduced.
		arcDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		arcDone.setErrorString("");
		arcDone.setSuccessful(true);
		return arcDone;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.9  2010/04/07 15:13:16  cjm
// More documentation.
// sendBasicAck used to ensure client does not time out when a long ARC exposure length is used.
//
// Revision 1.8  2010/03/15 16:48:04  cjm
// Removed stowFold call - now lamp unit has it's own mirror.
//
// Revision 1.7  2010/02/08 11:09:48  cjm
// Added unLockFile calls as saveFitsHeaders now creates FITS file locks.
//
// Revision 1.6  2009/10/07 11:25:43  cjm
// Added ACK before exposure start, to stop command timing out if computed arc exposure length
// > default ack time - readout.
//
// Revision 1.5  2009/08/05 14:42:19  cjm
// Moved setLampLock before stowFold, so fold is not stowed until lamp lock is acquired
// (and therefore any MULTRUNs on the other arm are finished).
//
// Revision 1.4  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.3  2008/11/25 18:28:06  cjm
// Fixed logging.
//
// Revision 1.2  2008/11/24 14:59:57  cjm
// Added code to modify OBJECT FITS header keyword for calibration ARCs using
// previously extracted objectName.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
