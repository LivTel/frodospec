// EXPOSEImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/EXPOSEImplementation.java,v 1.9 2011-01-12 11:50:03 cjm Exp $
package ngat.frodospec;

import java.io.*;
import java.util.*;

import ngat.eip.*;
import ngat.fits.*;
import ngat.frodospec.ccd.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.EXPOSE;
import ngat.message.ISS_INST.EXPOSE_DONE;
import ngat.message.ISS_INST.FILENAME_ACK;
import ngat.message.INST_DP.*;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the generic implementation for EXPOSE commands sent to a server using the
 * Java Message System. It extends FITSImplementation, as EXPOSE commands needs access to
 * resources to make FITS files.
 * @see FITSImplementation
 * @author Chris Mottram
 * @version $Revision: 1.9 $
 */
public class EXPOSEImplementation extends FITSImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: EXPOSEImplementation.java,v 1.9 2011-01-12 11:50:03 cjm Exp $");

	/**
	 * This method gets the EXPOSE command's acknowledge time. It returns the server connection 
	 * threads min acknowledge time. This method should be over-written in sub-classes.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getMinAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getMinAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method is a generic implementation for the EXPOSE command, that does nothing.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
	       	// do nothing 
		EXPOSE_DONE exposeDone = new EXPOSE_DONE(command.getId());

		exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		exposeDone.setErrorString("");
		exposeDone.setSuccessful(true);
		return exposeDone;
	}

	/**
	 * Do calibration associated with this exposure. 
	 * <ul>
	 * <li>The type of calibration is read from the property: 
	 *     <b>frodospec.calibrate.&lt;red|blue&gt;.&lt;before|after&gt;.type.&lt;index&gt;</b>
	 * <li>If the type is "DARK", call doCalibrationDark to do an equal length dark.
	 * <li>If the type is "ARC", the lamps to use for the ARC are read from:
	 *     <b>frodospec.calibrate.&lt;before|after&gt;.lamps.&lt;index&gt;</b>.
	 *      doCalibrationArc is then called to do the arc.
	 * </ul>
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @param arm Which arm to do the calibration for, either RED_ARM or BLUE_ARM.
	 * @param exposeCommand The command requiring this configuration to be done.
	 * @param exposeDone The done message, filled in if with a suitable error of the configuration failed.
	 * @param isBefore Boolean, if true this is a calibrateBefore, otherwise it is a calibrateAfter.
	 * @param exposureLength The length of the calibration dark to do, in milliseconds.
	 * @param reduceFilenameList A previously created List, to which all the calibration frames taken
	 *        my this method should be added. Used to send frame filenames to the data pipeline.
	 * @see #doCalibrationDark
	 * @see #doCalibrationArc
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected boolean doCalibration(String clazz,String source,
					int arm,EXPOSE exposeCommand,EXPOSE_DONE exposeDone,boolean isBefore,
					int exposureLength,List reduceFilenameList)
	{
		String beforeAfterString = null;
		String keywordString = null;
		String typeString = null;
		String lampsString = null;
		int index;
		boolean done;

		if(isBefore)
			beforeAfterString = "before";
		else
			beforeAfterString = "after";
		done = false;
		index = 0;
		while(done == false)
		{
			keywordString = new String("frodospec.calibrate."+FrodoSpecConstants.ARM_STRING_LIST[arm]+"."+
						   beforeAfterString+".type."+index);
			typeString = status.getProperty(keywordString);
			if(typeString != null)
			{
				if(typeString.equals("ARC"))
				{
					lampsString = status.getProperty("frodospec.calibrate."+
									 FrodoSpecConstants.ARM_STRING_LIST[arm]+
									 "."+beforeAfterString+".lamps."+index);
					if(doCalibrationArc(clazz,source,arm,lampsString,exposeCommand,exposeDone,
							    reduceFilenameList) == false)
						return false;
				}
				else if(typeString.equals("DARK"))
				{
					if(doCalibrationDark(clazz,source,arm,exposeCommand,exposeDone,exposureLength,
							     reduceFilenameList) == false)
						return false;
				}
				else
				{
					frodospec.error(this.getClass().getName()+":doCalibration:"+exposeCommand+
					      ":Unknown type string "+typeString+" from keyword "+keywordString+".");
					exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+602);
					exposeDone.setErrorString("Unknown type string "+typeString+" from keyword "+
								  keywordString+".");
					exposeDone.setSuccessful(false);
				}
				index++;
			}
			else
				done = true;
		}// end while
		return true;
	}

	/**
	 * Do DARK calibration associated with this exposure. In this case, do a dark with the same exposure length
	 * as the exposure itself.
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @param arm Which arm to do the calibration for, either RED_ARM or BLUE_ARM.
	 * @param exposeCommand The command requiring this configuration to be done.
	 * @param exposeDone The done message, filled in if with a suitable error of the configuration failed.
	 * @param exposureLength The length of the calibration dark to do, in milliseconds.
	 * @param reduceFilenameList A previously created List, to which all the calibration frames taken
	 *        my this method should be added. Used to send frame filenames to the data pipeline.
	 * @return The method returns true on success and false on failure.
	 * @see #status
	 * @see #frodospecFilenameList
	 * @see #frodospecFitsHeaderList
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FitsHeaderDefaults#OBSTYPE_VALUE_DARK
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#testAbort
	 * @see FITSImplementation#objectName
	 * @see #serverConnectionThread
	 * @see ngat.frodospec.ccd.CCDLibrary#expose
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected boolean doCalibrationDark(String clazz,String source,
					    int arm,EXPOSE exposeCommand,EXPOSE_DONE exposeDone,int exposureLength,
					    List reduceFilenameList)
	{
		FILENAME_ACK filenameAck = null;
		CCDLibrary ccd = null;
		FitsHeaderCardImage objectCardImage = null;
		String filename = null;
		boolean ccdEnable;

		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":doCalibrationArc:"+exposeCommand+":Illegal arm:"+arm+".");
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+603);
			exposeDone.setErrorString("doCalibrationArc:Illegal arm:"+arm+".");
			exposeDone.setSuccessful(false);
			return false;
		}
		// do exposure length dark (how many?)
		// clear pause and resume times.
		status.clearPauseResumeTimes();
		// get a new filename.
		try
		{
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_DARK);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
				       ":doCalibrationDark:"+exposeCommand+":",e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+604);
			exposeDone.setErrorString(e.toString());
			exposeDone.setSuccessful(false);
			return false;
		}
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
		// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(exposeCommand,exposeDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_DARK,
				  exposureLength) == false)
			return false;
		if(getFitsHeadersFromISS(exposeCommand,exposeDone,arm) == false)
			return false;
		if(testAbort(exposeCommand,exposeDone) == true)
			return false;
	// Modify "OBJECT" FITS header value to distinguish between spectra of the OBJECT
        // and calibration DARKs taken for that observation.
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectCardImage.setValue(new String("DARK for "+objectName));
		// save FITS headers
		if(saveFitsHeaders(exposeCommand,exposeDone,arm,filename) == false)
		{
			unLockFile(exposeCommand,exposeDone,filename);
			return false;
		}
		// do dark exposure.
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				ccd.expose(false,-1,exposureLength,filename);
			}
			catch(CCDLibraryNativeException e)
			{
				frodospec.error(this.getClass().getName()+
						":doCalibrationDark:"+exposeCommand+":",e);
				exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+601);
				exposeDone.setErrorString("doCalibrationDark:"+e.toString());
				exposeDone.setSuccessful(false);
				unLockFile(exposeCommand,exposeDone,filename);
				return false;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,clazz,source,this.getClass().getName()+
				      ":doCalibrationDark:Did not do dark, ccd enable was false.");
		}
		if(unLockFile(exposeCommand,exposeDone,filename) == false)
			return false;			
		// send acknowledge to say frame is completed.
		// Note, should really be MULTRUN_ACK/RUNAT_ACK.
		filenameAck = new FILENAME_ACK(exposeCommand.getId());
		filenameAck.setTimeToComplete(exposureLength+serverConnectionThread.getDefaultAcknowledgeTime());
		filenameAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(filenameAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
				       ":doCalibrationDark:sendAcknowledge:"+exposeCommand+":",e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+605);
			exposeDone.setErrorString("doCalibrationDark:sendAcknowledge:"+e.toString());
			exposeDone.setSuccessful(false);
			return false;
		}
		// add filename to list for data pipeline processing.
		reduceFilenameList.add(filename);
		// test whether an abort has occured.
		if(testAbort(exposeCommand,exposeDone) == true)
			return false;
		return true;
	}

	/**
	 * Do ARC calibration associated with this exposure. 
	 * @param clazz The class string used for generating log records from this operation.
	 * @param source The source string used for generating log records from this operation.
	 * @param arm Which arm to do the calibration for, either RED_ARM or BLUE_ARM.
	 * @param exposeCommand The command requiring this configuration to be done.
	 * @param exposeDone The done message, filled in if with a suitable error of the configuration failed.
	 * @param reduceFilenameList A previously created List, to which all the calibration frames taken
	 *        my this method should be added. Used to send frame filenames to the data pipeline.
	 * @return The method returns true on success and false on failure.
	 * @see #status
	 * @see #frodospecFilenameList
	 * @see #frodospecFitsHeaderList
	 * @see #getArcExposureLength
	 * @see #testAbort
	 * @see #turnLampsOff
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see FitsHeaderDefaults#OBSTYPE_VALUE_ARC
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#testAbort
	 * @see FITSImplementation#objectName
	 * @see #serverConnectionThread
	 * @see ngat.frodospec.ccd.CCDLibrary#expose
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 * @see FrodoSpecConstants#RESOLUTION_STRING_LIST
	 * @see LampController#setLampLock
	 * @see FrodoSpec#getLampController
	 * @see ngat.phase2.FrodoSpecConfig#RED_ARM
	 * @see ngat.phase2.FrodoSpecConfig#BLUE_ARM
	 */
	protected boolean doCalibrationArc(String clazz,String source,
					   int arm,String lampsString,EXPOSE exposeCommand,EXPOSE_DONE exposeDone,
					   List reduceFilenameList)
	{
		FILENAME_ACK filenameAck = null;
		CCDLibrary ccd = null;
		Plc plc = null;		
		FitsHeaderCardImage objectCardImage = null;
		String filename = null;
		int exposureLength,resolution;
		boolean ccdEnable;

		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":doCalibrationArc:"+exposeCommand+":Illegal arm:"+arm+".");
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+606);
			exposeDone.setErrorString("doCalibrationArc:Illegal arm:"+arm+".");
			exposeDone.setSuccessful(false);
			return false;
		}
		// get arm resolution 
		plc = frodospec.getPLC();
		try
		{
			resolution = plc.getGratingResolution(clazz,source,arm);
		}
		catch(EIPNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":doCalibrationArc:"+exposeCommand+":Failed to get Grating resolution.",e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+607);
			exposeDone.setErrorString("Failed to get Grating resolution:"+e);
			exposeDone.setSuccessful(false);
			return false;
		}
		// get exposure length dependant of lamps selected, central wavelength and binning
		try
		{
			exposureLength = getArcExposureLength(lampsString,arm,resolution);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":doCalibrationArc:"+exposeCommand+
					":Failed to get ARC exposure length for lamps:"+lampsString+" arm:"+
					FrodoSpecConstants.ARM_STRING_LIST[arm]+
					" resolution:"+FrodoSpecConstants.RESOLUTION_STRING_LIST[resolution],e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+608);
			exposeDone.setErrorString("Failed to get ARC exposure length for lamps:"+lampsString+":"+
						  e.toString());
			exposeDone.setSuccessful(false);
			return false;
		}
		// are we actually talking to the CCD
		ccdEnable = status.getPropertyBoolean("frodospec.ccd."+FrodoSpecConstants.ARM_STRING_LIST[arm]+
						      ".enable");
		// clear pause and resume times.
		status.clearPauseResumeTimes();
		// get a new filename.
		try
		{
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_ARC);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
				       ":doCalibrationArc:"+exposeCommand+":",e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+609);
			exposeDone.setErrorString("doCalibrationArc:"+e.toString());
			exposeDone.setSuccessful(false);
			return false;
		}
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
	// switch lamp on
	// We must do this before saving the FITS headers, if we want the right LAMPFLUX and LAMP<n>SET values.
	// Turn all lamps off before turning them back on again. This resets any on demand bits that have
	// subsequently timed out and been turned off by the PLC.
		try
		{
			frodospec.getLampController().setLampLock(clazz,source,arm,lampsString,serverConnectionThread);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":doCalibrationArc:"+exposeCommand+":Failed to turn on lamps:"+lampsString,e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+610);
			exposeDone.setErrorString("doCalibrationArc:Failed to turn on lamps:"+lampsString+":"+e);
			exposeDone.setSuccessful(false);
			return false;
		}
		if(testAbort(exposeCommand,exposeDone) == true)
			return false;
		// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(exposeCommand,exposeDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_ARC,
				  exposureLength) == false)
		{
			turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
			return false;
		}
		if(getFitsHeadersFromISS(exposeCommand,exposeDone,arm) == false)
		{
			turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
			return false;
		}
		if(testAbort(exposeCommand,exposeDone) == true)
		{
			turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
			return false;
		}
	// Modify "OBJECT" FITS header value to distinguish between spectra of the OBJECT
        // and calibration ARCs taken for that observation.
		objectCardImage = frodospecFitsHeaderList[arm].get("OBJECT");
		objectCardImage.setValue(new String("ARC: "+lampsString+" for "+objectName));
		// save FITS headers
		if(saveFitsHeaders(exposeCommand,exposeDone,arm,filename) == false)
		{
			turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
			unLockFile(exposeCommand,exposeDone,filename);
			return false;
		}
		// do arc.
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				ccd.expose(true,-1,exposureLength,filename);
			}
			catch(CCDLibraryNativeException e)
			{
				turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
				frodospec.error(this.getClass().getName()+
						":doCalibrationArc:"+exposeCommand+":",e);
				exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+611);
				exposeDone.setErrorString("doCalibrationArc:"+e.toString());
				exposeDone.setSuccessful(false);
				unLockFile(exposeCommand,exposeDone,filename);
				return false;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,clazz,source,
				      this.getClass().getName()+
				      ":doCalibrationArc:Did not do arc exposure, ccd enable was false.");
		}
		// switch lamp off
		turnLampsOff(clazz,source,arm,exposeCommand,exposeDone);
		// unlock FITS file lock created by saveFitsHeaders
		if(unLockFile(exposeCommand,exposeDone,filename) == false)
			return false;
		// send acknowledge to say frame is completed.
		// Note, should really be MULTRUN_ACK/RUNAT_ACK.
		filenameAck = new FILENAME_ACK(exposeCommand.getId());
		// Send ACK
		filenameAck.setTimeToComplete(exposureLength+
					      serverConnectionThread.getDefaultAcknowledgeTime());
		filenameAck.setFilename(filename);
		try
		{
			serverConnectionThread.sendAcknowledge(filenameAck);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
					":doCalibrationDark:sendAcknowledge:"+exposeCommand+":",e);
			exposeDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+612);
			exposeDone.setErrorString(e.toString());
			exposeDone.setSuccessful(false);
			return false;
		}
		// add filename to list for data pipeline processing.
		reduceFilenameList.add(filename);
		// test whether an abort has occured.
		if(testAbort(exposeCommand,exposeDone) == true)
			return false;
		return true;
	}

	/**
	 * This routine calls the Real Time Data Pipeline to process the expose FITS image we have just captured.
	 * If an error occurs the done objects field's are set accordingly. If the operation succeeds, and the
	 * done object is of class EXPOSE_DONE, the done object is filled with data returned from the 
	 * reduction command.
	 * @param command The command being implemented that made this call to the DP(RT). This is used
	 * 	for error logging.
	 * @param done A COMMAND_DONE subclass specific to the command being implemented. If an
	 * 	error occurs the relevant fields are filled in with the error.
	 * @return The routine returns a boolean to indicate whether the operation was completed
	 *  	successfully.
	 * @see FrodoSpec#sendDpRtCommand
	 */
	public boolean reduceExpose(COMMAND command,COMMAND_DONE done,String filename)
	{
		EXPOSE_REDUCE reduce = new EXPOSE_REDUCE(command.getId());
		INST_TO_DP_DONE instToDPDone = null;
		EXPOSE_REDUCE_DONE reduceDone = null;
		EXPOSE_DONE exposeDone = null;

		reduce.setFilename(filename);
		instToDPDone = frodospec.sendDpRtCommand(reduce,serverConnectionThread);
		if(instToDPDone.getSuccessful() == false)
		{
			frodospec.error(this.getClass().getName()+":reduce:"+
				command+":"+instToDPDone.getErrorNum()+":"+instToDPDone.getErrorString());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+600);
			done.setErrorString(instToDPDone.getErrorString());
			done.setSuccessful(false);
			return false;
		}
		// Copy the DP REDUCE DONE parameters to the FrodoSpec EXPOSE DONE parameters
		if(instToDPDone instanceof EXPOSE_REDUCE_DONE)
		{
			reduceDone = (EXPOSE_REDUCE_DONE)instToDPDone;
			if(done instanceof EXPOSE_DONE)
			{
				exposeDone = (EXPOSE_DONE)done;
				exposeDone.setFilename(reduceDone.getFilename());
				exposeDone.setSeeing(reduceDone.getSeeing());
				exposeDone.setCounts(reduceDone.getCounts());
				exposeDone.setXpix(reduceDone.getXpix());
				exposeDone.setYpix(reduceDone.getYpix());
				exposeDone.setPhotometricity(reduceDone.getPhotometricity());
				exposeDone.setSkyBrightness(reduceDone.getSkyBrightness());
				exposeDone.setSaturation(reduceDone.getSaturation());
			}
		}
		return true;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.8  2011/01/05 14:07:42  cjm
// Fixed javadocs.
//
// Revision 1.7  2010/03/15 16:48:40  cjm
// Removed stowFold call - now lamp unit has it's own mirror.
//
// Revision 1.6  2010/02/08 11:09:26  cjm
// Added unLockFile calls as saveFitsHeaders now creates FITS file locks.
//
// Revision 1.5  2009/08/19 13:54:48  cjm
// Moved stowFold in doCalibrationArc inside LampController lock code,
// to stop fold being moved whilst the other arm is doing an exposure.
//
// Revision 1.4  2009/05/07 15:32:07  cjm
// Fixed comments.
//
// Revision 1.3  2009/02/05 11:38:59  cjm
// Swapped Bitwise for Absolute logging levels.
//
// Revision 1.2  2008/11/24 14:59:20  cjm
// Added code to modify OBJECT FITS header keyword for calibration ARCs and DARKs using
// previously extracted objectName.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
