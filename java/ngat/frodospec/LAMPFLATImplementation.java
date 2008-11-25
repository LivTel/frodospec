// LAMPFLATImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/LAMPFLATImplementation.java,v 1.1 2008-11-25 17:16:39 cjm Exp $
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

/**
 * This class provides the implementation for the LAMPFLAT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class LAMPFLATImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: LAMPFLATImplementation.java,v 1.1 2008-11-25 17:16:39 cjm Exp $");
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
	 * <li>It generates some FITS headers from the CCD setup and
	 * the ISS and saves this to disc. 
	 * <li>It performs an exposure and saves the data from this to disc.
	 * </ul>
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#objectName
	 * @see FITSImplementation#getArcExposureLength
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
			resolution = plc.getGratingResolution(arm);
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
		// are we actually talking to the CCD
		ccdEnable = status.getPropertyBoolean("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".enable");
	// setup exposure status.
		status.setExposureCount(arm,1);
		status.setExposureNumber(arm,0);
		status.clearPauseResumeTimes();
	// move the fold mirror to the stowed location
		if(stowFold(lampFlatCommand,lampFlatDone) == false)
			return lampFlatDone;
		if(testAbort(lampFlatCommand,lampFlatDone) == true)
			return lampFlatDone;
	// switch lamp on
	// We must do this before saving the FITS headers, if we want the right LAMPFLUX and LAMP<n>SET values.
	// Turn all lamps off before turning them back on again. This resets any on demand bits that have
	// subsequently timed out and been turned off by the PLC.
		try
		{
			frodospec.getLampController().setLampLock(arm,lampsString,serverConnectionThread);
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
	// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(lampFlatCommand,lampFlatDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_LAMP_FLAT,
				  exposureLength) == false)
		{
			// turn lamps off
			turnLampsOff(arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
		if(getFitsHeadersFromISS(lampFlatCommand,lampFlatDone,arm) == false)
		{
			// turn lamps off
			turnLampsOff(arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
		if(testAbort(lampFlatCommand,lampFlatDone) == true)
		{
			// turn lamps off
			turnLampsOff(arm,lampFlatCommand,lampFlatDone);
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
			turnLampsOff(arm,lampFlatCommand,lampFlatDone);
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
			turnLampsOff(arm,lampFlatCommand,lampFlatDone);
			return lampFlatDone;
		}
	// do lampFlat
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
				turnLampsOff(arm,lampFlatCommand,lampFlatDone);
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
			frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,
				      this.getClass().getName()+
				      ":processCommand:Did not do lamp flat exposure, ccd enable was false.");
		}
		// switch lamp off
		if(turnLampsOff(arm,lampFlatCommand,lampFlatDone) == false)
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
//
