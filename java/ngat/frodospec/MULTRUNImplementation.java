// MULTRUNImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/MULTRUNImplementation.java,v 1.3 2009-08-05 14:42:17 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.File;
import java.io.IOException;
import java.util.Date;
import java.util.Vector;
import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.MULTRUN;
import ngat.message.ISS_INST.MULTRUN_ACK;
import ngat.message.ISS_INST.MULTRUN_DP_ACK;
import ngat.message.ISS_INST.MULTRUN_DONE;
import ngat.message.ISS_INST.FRODOSPEC_MULTRUN;
import ngat.message.ISS_INST.FRODOSPEC_MULTRUN_DONE;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the MULTRUN command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.3 $
 */
public class MULTRUNImplementation extends EXPOSEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: MULTRUNImplementation.java,v 1.3 2009-08-05 14:42:17 cjm Exp $");
	/**
	 * Constructor.
	 */
	public MULTRUNImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_MULTRUN&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_MULTRUN";
	}

	/**
	 * This method returns the FRODOSPEC_MULTRUN command's acknowledge time. 
	 * Each frame in the FRODOSPEC_MULTRUN takes 
	 * the exposure time plus the default acknowledge time to complete. The default acknowledge time
	 * allows time to setup the camera, get information about the telescope and save the frame to disk.
	 * This method returns the time for the first frame in the FRODOSPEC_MULTRUN only, as a MULTRUN_ACK message
	 * is returned to the client for each frame taken.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 * @see FRODOSPEC_MULTRUN#getExposureTime
	 * @see FRODOSPEC_MULTRUN#getNumberExposures
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		FRODOSPEC_MULTRUN multRunCommand = (FRODOSPEC_MULTRUN)command;
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(multRunCommand.getExposureTime()+
			serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the FRODOSPEC_MULTRUN command. 
	 * <ul>
	 * <li>If calibrate before is set, doCalibration is called to do some BIAS/DARKs/ARCs.
	 * <li>Uses setNoLampLock to acquire the lock for no lamps being on. This ensures the other
	 *     arm is not doing an ARC etc (with the fold mirror stowed).
	 * <li>It moves the fold mirror to the correct location.
	 * <li>It starts the autoguider.
	 * <li>For each exposure it performs the following:
	 *	<ul>
	 * 	<li>It generates some FITS headers from the CCD setup and the ISS. 
	 * 	<li>Sets the time of exposure and saves the Fits headers.
	 * 	<li>It performs an exposure and saves the data from this to disc.
	 * 	<li>Keeps track of the generated filenames in the list.
	 * 	</ul>
	 * <li>It releases the "no lamp lock" using clearLampLock.
	 * <li>It stops the autoguider.
	 * <li>If calibrate after is set, doCalibration is called to do some BIAS/DARKs/ARCs.
	 * <li>It calls the Real Time Data Pipeline to reduce the data for each exposure taken.
	 * <li>It sets up the return values to return to the client.
	 * </ul>
	 * The resultant filename or the relevant error code is put into the an object of class MULTRUN_DONE and
	 * returned. During execution of these operations the abort flag is tested to see if we need to
	 * stop the implementation of this command.
	 * @see #frodospec
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#frodospecFilenameList
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FrodoSpecStatus#setExposureCount
	 * @see FrodoSpecStatus#setExposureNumber
	 * @see ngat.frodospec.ccd.CCDLibrary#expose
	 * @see EXPOSEImplementation#doCalibration
	 * @see EXPOSEImplementation#reduceExpose
	 * @see HardwareImplementation#redCCD
	 * @see HardwareImplementation#blueCCD
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.frodospec.FrodoSpec#getLampController
	 * @see ngat.frodospec.LampController#setNoLampLock
	 * @see ngat.frodospec.LampController#clearLampLock
	 * @see ngat.fits.FitsFilename#setExposureCode
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_MULTRUN frodospecMultRunCommand = null;
		MULTRUN_ACK multRunAck = null;
		MULTRUN_DP_ACK multRunDpAck = null;
		FRODOSPEC_MULTRUN_DONE frodospecMultRunDone = new FRODOSPEC_MULTRUN_DONE(command.getId());
		CCDLibrary ccd = null;
		String obsType = null;
		String filename = null;
		Vector filenameList = null;
		Vector reduceFilenameList = null;
		int index,arm;
		boolean retval = false,ccdEnable;

		if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
			return frodospecMultRunDone;
		if((command instanceof FRODOSPEC_MULTRUN) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Multrun command has wrong class:"+
					command.getClass().getName());
			frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1204);
			frodospecMultRunDone.setErrorString("Multrun command has wrong class:"+
							    command.getClass().getName());
			frodospecMultRunDone.setSuccessful(false);
			return frodospecMultRunDone;
		}
		frodospecMultRunCommand = (FRODOSPEC_MULTRUN)command;
		arm = frodospecMultRunCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Illegal arm:"+arm+".");
			frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1207);
			frodospecMultRunDone.setErrorString("Illegal arm:"+arm+".");
			frodospecMultRunDone.setSuccessful(false);
			return frodospecMultRunDone;
		}
	//  setup exposure status.
		status.setExposureCount(arm,frodospecMultRunCommand.getNumberExposures());
		status.setExposureNumber(arm,0);
	// setup multrun number
		frodospecFilenameList[arm].nextMultRunNumber();
       // setup reduced filename list
		reduceFilenameList = new Vector();
       // do any calibrate before here
		frodospec.log(Logger.VERBOSITY_INTERMEDIATE,"Command:"+
			     frodospecMultRunDone.getClass().getName()+
			     ":calibrate before = "+status.getConfigCalibrateBefore(arm)+".");
		if(status.getConfigCalibrateBefore(arm))
		{
			if(doCalibration(arm,frodospecMultRunCommand,frodospecMultRunDone,true,
					 frodospecMultRunCommand.getExposureTime(),reduceFilenameList) == false)
				return frodospecMultRunDone;
		}
		if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
			return frodospecMultRunDone;
	// ensure all lamps are off
	// before moving fold, so fold is not moved until lamp lock acquired
		try
		{
			frodospec.getLampController().setNoLampLock(arm,serverConnectionThread);
		}
		catch(Exception e)
		{
			autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
			frodospec.error(this.getClass().getName()+
				  ":processCommand:"+command+":Failed to set No lamp lock:"+e.toString());
			frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1200);
			frodospecMultRunDone.setErrorString(e.toString());
			frodospecMultRunDone.setSuccessful(false);
			return frodospecMultRunDone;
		}
		if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
			return frodospecMultRunDone;
		}
	// move the fold mirror to the correct location
	// after calibrate before which will stow the fold for ARCs
		if(moveFold(frodospecMultRunCommand,frodospecMultRunDone) == false)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
			return frodospecMultRunDone;
		}
		if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);			
			return frodospecMultRunDone;
		}
	// setup filename obs type/exposure code
	// must be done after calibrate before, which will change these
		try
		{
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
			if(frodospecMultRunCommand.getStandard())
			{
				frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_STANDARD);
				obsType = FitsHeaderDefaults.OBSTYPE_VALUE_STANDARD;
			}
			else
			{
				frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_EXPOSURE);
				obsType = FitsHeaderDefaults.OBSTYPE_VALUE_EXPOSURE;
			}
		}
		catch(Exception e)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
			frodospec.error(this.getClass().getName()+
				  ":processCommand:"+command+":"+e.toString());
			frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1206);
			frodospecMultRunDone.setErrorString(e.toString());
			frodospecMultRunDone.setSuccessful(false);
			return frodospecMultRunDone;
		}
	// autoguider on
		if(autoguiderStart(frodospecMultRunCommand,frodospecMultRunDone) == false)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
			return frodospecMultRunDone;
		}
		if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
		{
			// actually removing NO_LAMP lock
			turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
			autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
			return frodospecMultRunDone;
		}
	// do exposures
		index = 0;
		retval = true;
		while(retval&&(index < frodospecMultRunCommand.getNumberExposures()))
		{
		// initialise list of FITS filenames for this frame
			filenameList = new Vector();
		// clear pause and resume times.
			status.clearPauseResumeTimes();
		// get a new filename.
			frodospecFilenameList[arm].nextRunNumber();
			filename = frodospecFilenameList[arm].getFilename();
// diddly window 1 only
		// get fits headers
			clearFitsHeaders(arm);
			if(setFitsHeaders(frodospecMultRunCommand,frodospecMultRunDone,arm,obsType,
					  frodospecMultRunCommand.getExposureTime(),
					  frodospecMultRunCommand.getNumberExposures()) == false)
			{
				// actually removing NO_LAMP lock
				turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
				autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
				return frodospecMultRunDone;
			}
			if(getFitsHeadersFromISS(frodospecMultRunCommand,frodospecMultRunDone,arm) == false)
			{
				// actually removing NO_LAMP lock
				turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
				autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
				return frodospecMultRunDone;
			}
			if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
			{
				// actually removing NO_LAMP lock
				turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
				autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
				return frodospecMultRunDone;
			}
		// save FITS headers
			if(saveFitsHeaders(frodospecMultRunCommand,frodospecMultRunDone,arm,filenameList) == false)
			{
				// actually removing NO_LAMP lock
				turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
				autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
				return frodospecMultRunDone;
			}
		// do exposure.
// diddly window 1 only
// diddly one arm only
			status.setExposureFilename(arm,filename);
			if(ccdEnable)
			{
				try
				{
					// diddly window 1 filename only
					ccd.expose(true,-1,frodospecMultRunCommand.getExposureTime(),filenameList);
				}
				catch(CCDLibraryNativeException e)
				{
					frodospec.error(this.getClass().getName()+
							":processCommand:"+command+":"+e.toString());
					frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+
									 1201);
					frodospecMultRunDone.setErrorString(e.toString());
					frodospecMultRunDone.setSuccessful(false);
					// actually removing NO_LAMP lock
					turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
					autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
					return frodospecMultRunDone;
				}
			}// end if ccdEnable
			else
			{
				frodospec.log(Logger.VERBOSITY_VERY_TERSE,
					      this.getClass().getName()+
					      ":processCommand:Did not do multrun exposure, ccd enable was false.");
			}
		// send acknowledge to say frame is completed.
			multRunAck = new MULTRUN_ACK(command.getId());
			multRunAck.setTimeToComplete(frodospecMultRunCommand.getExposureTime()+
				serverConnectionThread.getDefaultAcknowledgeTime());
// diddly window 1 filename only
			multRunAck.setFilename(filename);
			try
			{
				serverConnectionThread.sendAcknowledge(multRunAck);
			}
			catch(IOException e)
			{
				retval = false;
				frodospec.error(this.getClass().getName()+
					":processCommand:sendAcknowledge:"+command+":"+e.toString());
				frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1202);
				frodospecMultRunDone.setErrorString(e.toString());
				frodospecMultRunDone.setSuccessful(false);
				// actually removing NO_LAMP lock
				turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone);
				autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
				return frodospecMultRunDone;
			}
			status.setExposureNumber(arm,index+1);
		// add filename to list for data pipeline processing.
// diddly window 1 filename only
			reduceFilenameList.addAll(filenameList);
		// test whether an abort has occured.
			if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
			{
				retval = false;
			}
			index++;
		}
	// clear lock on lamps
		if(turnLampsOff(arm,frodospecMultRunCommand,frodospecMultRunDone) == false)
		{
			autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,false);
			return frodospecMultRunDone;
		}
	// autoguider off
		if(autoguiderStop(frodospecMultRunCommand,frodospecMultRunDone,true) == false)
			return frodospecMultRunDone;
	// if a failure occurs, return now
		if(!retval)
			return frodospecMultRunDone;
		index = 0;
		retval = true;
	// calibrate after here
		frodospec.log(Logger.VERBOSITY_INTERMEDIATE,"Command:"+
			     frodospecMultRunDone.getClass().getName()+
			     ":calibrate after = "+status.getConfigCalibrateAfter(arm)+".");
		if(status.getConfigCalibrateAfter(arm))
		{
			if(doCalibration(arm,frodospecMultRunCommand,frodospecMultRunDone,false,
					 frodospecMultRunCommand.getExposureTime(),reduceFilenameList) == false)
				return frodospecMultRunDone;
		}
	// call pipeline to process data and get results
		if(frodospecMultRunCommand.getPipelineProcess())
		{
			while(retval&&(index < frodospecMultRunCommand.getNumberExposures()))
			{
				filename = (String)reduceFilenameList.get(index);
			// do reduction.
				retval = reduceExpose(frodospecMultRunCommand,frodospecMultRunDone,filename);
			// send acknowledge to say frame has been reduced.
				multRunDpAck = new MULTRUN_DP_ACK(command.getId());
				multRunDpAck.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
			// copy Data Pipeline results from DONE to ACK
				multRunDpAck.setFilename(frodospecMultRunDone.getFilename());
				multRunDpAck.setCounts(frodospecMultRunDone.getCounts());
				multRunDpAck.setSeeing(frodospecMultRunDone.getSeeing());
				multRunDpAck.setXpix(frodospecMultRunDone.getXpix());
				multRunDpAck.setYpix(frodospecMultRunDone.getYpix());
				multRunDpAck.setPhotometricity(frodospecMultRunDone.getPhotometricity());
				multRunDpAck.setSkyBrightness(frodospecMultRunDone.getSkyBrightness());
				multRunDpAck.setSaturation(frodospecMultRunDone.getSaturation());
				try
				{
					serverConnectionThread.sendAcknowledge(multRunDpAck);
				}
				catch(IOException e)
				{
					retval = false;
					frodospec.error(this.getClass().getName()+
						":processCommand:sendAcknowledge(DP):"+command+":"+e.toString());
					frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+
									 1203);
					frodospecMultRunDone.setErrorString(e.toString());
					frodospecMultRunDone.setSuccessful(false);
					return frodospecMultRunDone;
				}
				if(testAbort(frodospecMultRunCommand,frodospecMultRunDone) == true)
				{
					retval = false;
				}
				index++;
			}// end while on MULTRUN exposures
		}// end if Data Pipeline is to be called
		else
		{
		// no pipeline processing occured, set return value to something bland.
		// set filename to last filename exposed.
			frodospecMultRunDone.setFilename(filename);
			frodospecMultRunDone.setCounts(0.0f);
			frodospecMultRunDone.setSeeing(0.0f);
			frodospecMultRunDone.setXpix(0.0f);
			frodospecMultRunDone.setYpix(0.0f);
			frodospecMultRunDone.setPhotometricity(0.0f);
			frodospecMultRunDone.setSkyBrightness(0.0f);
			frodospecMultRunDone.setSaturation(false);
		}
	// if a failure occurs, return now
		if(!retval)
			return frodospecMultRunDone;
	// setup return values.
	// setCounts,setFilename,setSeeing,setXpix,setYpix 
	// setPhotometricity, setSkyBrightness, setSaturation set by reduceExpose for last image reduced.
		frodospecMultRunDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		frodospecMultRunDone.setErrorString("");
		frodospecMultRunDone.setSuccessful(true);
	// return done object.
		return frodospecMultRunDone;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.2  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
