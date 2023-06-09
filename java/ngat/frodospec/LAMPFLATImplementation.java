// LAMPFLATImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/LAMPFLATImplementation.java,v 1.8 2011-01-17 10:48:10 cjm Exp $
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
import ngat.message.ISS_INST.LAMPFLAT;
import ngat.message.ISS_INST.LAMPFLAT_DONE;
import ngat.message.ISS_INST.FRODOSPEC_LAMPFLAT;
import ngat.message.ISS_INST.FRODOSPEC_LAMPFLAT_DONE;
import ngat.message.ISS_INST.FILENAME_ACK;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the LAMPFLAT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.8 $
 */
public class LAMPFLATImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: LAMPFLATImplementation.java,v 1.8 2011-01-17 10:48:10 cjm Exp $");
	/**
	 * Constructor.
	 */
	public LAMPFLATImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_LAMPFLAT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_LAMPFLAT";
	}

	/**
	 * This method returns the FRODOSPEC_LAMPFLAT command's acknowledge time. 
	 * The frame in the FRODOSPEC_LAMPFLAT takes 
	 * the configured exposure time plus the default acknowledge time to complete. The default acknowledge time
	 * allows time to setup the camera, get information about the telescope and save the frame to disk.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		FRODOSPEC_LAMPFLAT lampFlatCommand = (FRODOSPEC_LAMPFLAT)command;
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		// diddly get exposure time from lamp
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the FRODOSPEC_LAMPFLAT command. 
	 * <ul>
	 * <li>The arm is extracted from the command.
	 * <li>The currently configured resolution for the specified arm is queried from the Plc.
	 * <li>getArcExposureLength is used to determine the exposure length needed.
	 * <li>sendBasicAck sends an Ack back to the client, to stop the connection timing out before the exposure
	 *     is finished.
	 * <li>We use the setLampLock LampController method to wait and acquire the rights to use the arc lamp,
	 *     and then to turn the lamp on.
	 * <li>We generate some FITS headers from the CCD setup and the ISS 
	 *     (setFitsHeaders and getFitsHeadersFromISS). 
	 * <li>We change the OBJECT FITS header to indicate this is a LAMPFLAT.
	 * <li>We increment the relevant arm's Multrun number and run number.
	 * <li>We save the FITS headers to the generated filename.
	 * <li>We resend another ACK to the client to ensure the connection is kept alive 
	 *     whilst we actually take the exposure.
	 * <li>It performs an exposure and saves the data from this to disc.
	 * <li>turnLampsOff is called to turn the lamp off.
	 * <li>unLockFile is called to remove the file lock created by saveFitsHeaders.
	 * <li>A FILENAME_ACK is sent back to the client with the new filename.
	 * <li>reduceCalibrate is called to reduce the LAMP flat.
	 * </ul>
	 * @see CommandImplementation#testAbort
	 * @see #frodospecFilenameList
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FITSImplementation#objectName
	 * @see FITSImplementation#getArcExposureLength
	 * @see CALIBRATEImplementation#sendBasicAck
	 * @see CALIBRATEImplementation#reduceCalibrate
	 * @see FrodoSpec#getLampUnit
	 * @see FrodoSpec#getLampController
	 * @see FrodoSpec#getPLC
	 * @see Plc#getGratingResolution
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 * @see ngat.frodospec.FrodoSpec#getLampController
	 * @see ngat.frodospec.LampController#setNoLampLock
	 * @see ngat.frodospec.LampController#clearLampLock
	 * @see ngat.fits.FitsHeaderDefaults#OBSTYPE_VALUE_LAMP_FLAT
	 * @see ngat.fits.FitsFilename#EXPOSURE_CODE_LAMP_FLAT
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_LAMPFLAT lampFlatCommand = null;
		FRODOSPEC_LAMPFLAT_DONE lampFlatDone = new FRODOSPEC_LAMPFLAT_DONE(command.getId());
		FILENAME_ACK lampFlatAck = null;
		FitsHeaderCardImage objectCardImage = null;
		CCDLibrary ccd = null;
		Plc plc = null;		
		String lampsString = null;
		String filename = null;
		int exposureLength,arm,resolution;
		boolean ccdEnable;

		if(testAbort(command,lampFlatDone) == true)
			return lampFlatDone;
		if((command instanceof FRODOSPEC_LAMPFLAT) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Lamp Flat command has wrong class:"+
					command.getClass().getName());
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2700);
			lampFlatDone.setErrorString("Lamp Flat command has wrong class:"+
							    command.getClass().getName());
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
		lampFlatCommand = (FRODOSPEC_LAMPFLAT)command;
		arm = lampFlatCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Illegal arm:"+arm+".");
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2701);
			lampFlatDone.setErrorString("Illegal arm:"+arm+".");
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
		// get arm resolution 
		plc = frodospec.getPLC();
		try
		{
			resolution = plc.getGratingResolution(lampFlatCommand.getClass().getName(),
							      FrodoSpecConstants.ARM_STRING_LIST[arm],arm);
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to get Grating resolution.",e);
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2702);
			lampFlatDone.setErrorString("Failed to get Grating resolution:"+e);
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
		// get lamp and exposure length
		lampsString = lampFlatCommand.getLamp();
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
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2703);
			lampFlatDone.setErrorString("Failed to get LAMPFLAT exposure length for lamps:"
					+lampsString+":"+e);
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
		// send a basic Ack to keep the connection alive whilst we do an exposure
		if(sendBasicAck(lampFlatCommand,lampFlatDone,
				exposureLength+serverConnectionThread.getDefaultAcknowledgeTime()) == false)
			return lampFlatDone;
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
			frodospec.getLampController().setLampLock(lampFlatCommand.getClass().getName(),
								  FrodoSpecConstants.ARM_STRING_LIST[arm],
								  arm,lampsString,serverConnectionThread);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Failed to turn on lamps:"+lampsString,e);
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2704);
			lampFlatDone.setErrorString("Failed to turn on lamps:"+lampsString+":"+e);
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
		if(testAbort(lampFlatCommand,lampFlatDone) == true)
		{
			// turn lamps off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
	// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(lampFlatCommand,lampFlatDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_LAMP_FLAT,
				  exposureLength) == false)
		{
			// turn lamps off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
		if(getFitsHeadersFromISS(lampFlatCommand,lampFlatDone,arm) == false)
		{
			// turn lamps off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
		if(testAbort(lampFlatCommand,lampFlatDone) == true)
		{
			// turn lamps off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
	// Modify "OBJECT" FITS header value to distinguish between spectra of the OBJECT
        // and calibration LAMPFLATs taken for that observation.
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectCardImage.setValue(new String("LAMPFLAT: "+lampsString+" for "+objectName));
	// don't do calibrate before
	// setup filename object
		frodospecFilenameList[arm].nextMultRunNumber();
		try
		{
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_LAMP_FLAT);
		}
		catch(Exception e)
		{
			// switch lamp off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			frodospec.error(this.getClass().getName()+
				  ":processCommand:"+command+":"+e.toString());
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2705);
			lampFlatDone.setErrorString(e.toString());
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
	// get a new filename.
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
	// save FITS headers
		if(saveFitsHeaders(lampFlatCommand,lampFlatDone,arm,filename) == false)
		{
			// switch lamp off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			unLockFile(lampFlatCommand,lampFlatDone,filename);
			return lampFlatDone;
		}
        // send ack of exposurelength + readout before starting exposure
		if(sendBasicAck(lampFlatCommand,lampFlatDone,
				exposureLength+serverConnectionThread.getDefaultAcknowledgeTime()) == false)
		{
			// switch lamp off
			turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				     arm,lampFlatCommand,lampFlatDone);
			unLockFile(lampFlatCommand,lampFlatDone,filename);
			return lampFlatDone;
		}
	// do lampFlat
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				// diddly window 1 filename only
				ccd.expose("LAMPFLAT",FrodoSpecConstants.ARM_STRING_LIST[arm],
					   true,-1,exposureLength,filename);
			}
			catch(CCDLibraryNativeException e)
			{
				// switch lamp off
				turnLampsOff(lampFlatCommand.getClass().getName(),
					     FrodoSpecConstants.ARM_STRING_LIST[arm],arm,lampFlatCommand,lampFlatDone);
				unLockFile(lampFlatCommand,lampFlatDone,filename);
				frodospec.error(this.getClass().getName()+
						":processCommand:"+command+":"+e.toString());
				lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2706);
				lampFlatDone.setErrorString(e.toString());
				lampFlatDone.setSuccessful(false);
				return lampFlatDone;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,
				      this.getClass().getName()+
				      ":processCommand:Did not do lamp flat exposure, ccd enable was false.");
		}
		// switch lamp off
		if(turnLampsOff(lampFlatCommand.getClass().getName(),FrodoSpecConstants.ARM_STRING_LIST[arm],
				arm,lampFlatCommand,lampFlatDone) == false)
			return lampFlatDone;
		// unlock FITS file lock created by saveFitsHeaders
		if(unLockFile(lampFlatCommand,lampFlatDone,filename)  == false)
			return lampFlatDone;
		status.setExposureNumber(arm,1);
		// send acknowledge to say frame is completed.
		lampFlatAck = new FILENAME_ACK(command.getId());
		lampFlatAck.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		lampFlatAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(lampFlatAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:sendAcknowledge:"+command+":"+e.toString());
			lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2707);
			lampFlatDone.setErrorString(e.toString());
			lampFlatDone.setSuccessful(false);
			return lampFlatDone;
		}
	// call pipeline to process data and get results
		if(reduceCalibrate(lampFlatCommand,lampFlatDone,filename) == false)
			return lampFlatDone;
	// setup return values.
	// meanCounts and peakCounts set by pipelineProcess for last image reduced.
		lampFlatDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		lampFlatDone.setErrorString("");
		lampFlatDone.setSuccessful(true);
		return lampFlatDone;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.7  2011/01/12 11:50:03  cjm
// Adding clazz and source logging to PLC/Lamp API.
//
// Revision 1.6  2010/04/07 15:13:15  cjm
// More documentation.
// sendBasicAck used to ensure client does not time out when a long ARC exposure length is used.
//
// Revision 1.5  2010/03/15 16:47:47  cjm
// Removed stowFold call - now lamp unit has it's own mirror.
//
// Revision 1.4  2010/02/08 11:09:19  cjm
// Added unLockFile calls as saveFitsHeaders now creates FITS file locks.
//
// Revision 1.3  2009/08/05 14:42:20  cjm
// Moved setLampLock before stowFold, so fold is not stowed until lamp lock is acquired
// (and therefore any MULTRUNs on the other arm are finished).
//
// Revision 1.2  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.1  2008/11/25 17:16:39  cjm
// Initial revision
//
//
