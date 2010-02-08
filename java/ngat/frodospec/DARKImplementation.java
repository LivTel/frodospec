// DARKImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/DARKImplementation.java,v 1.3 2010-02-08 11:09:54 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.FRODOSPEC_DARK;
import ngat.message.ISS_INST.FRODOSPEC_DARK_DONE;
import ngat.phase2.FrodoSpecConfig;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the DARK command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.3 $
 */
public class DARKImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: DARKImplementation.java,v 1.3 2010-02-08 11:09:54 cjm Exp $");

	/**
	 * Constructor.
	 */
	public DARKImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.DARK&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_DARK";
	}

	/**
	 * This method gets the DARK command's acknowledge time. This returns the server connection threads 
	 * default acknowledge time plus the dark exposure time.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;
		FRODOSPEC_DARK darkCommand = (FRODOSPEC_DARK)command;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(darkCommand.getExposureTime()+
			serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the DARK command. It generates some FITS headers from the CCD setup and
	 * the ISS and saves this to disc. It performs a dark exposure and saves the data from this to disc.
	 * It sends the generated FITS data to the Real Time Data Pipeline to get some data from it.
	 * The resultant data or the relevant error code is put into the an object of class DARK_DONE and
	 * returned. During execution of these operations the abort flag is tested to see if we need to
	 * stop the implementation of this command.
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#frodospecFilenameList
	 * @see FITSImplementation#checkNonWindowedSetup
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see FITSImplementation#unLockFile
	 * @see ngat.frodospec.ccd.CCDLibrary#expose
	 * @see CALIBRATEImplementation#reduceCalibrate
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_DARK darkCommand = null;
		FRODOSPEC_DARK_DONE darkDone = new FRODOSPEC_DARK_DONE(command.getId());
		CCDLibrary ccd = null;
		String filename = null;
		int arm;
		boolean ccdEnable;

		if((command instanceof FRODOSPEC_DARK) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Dark command has wrong class:"+
					command.getClass().getName());
			darkDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+902);
			darkDone.setErrorString("Dark command has wrong class:"+
							    command.getClass().getName());
			darkDone.setSuccessful(false);
			return darkDone;
		}
		darkCommand = (FRODOSPEC_DARK)command;
		arm = darkCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		if(testAbort(command,darkDone) == true)
			return darkDone;
		if(checkNonWindowedSetup(arm,darkDone) == false)
			return darkDone;
	// Clear the pause and resume times.
		status.clearPauseResumeTimes();
	// get fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(command,darkDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_DARK,
			darkCommand.getExposureTime()) == false)
			return darkDone;
		if(getFitsHeadersFromISS(command,darkDone,arm) == false)
			return darkDone;
		if(testAbort(command,darkDone) == true)
			return darkDone;
	// get a filename to store frame in
		frodospecFilenameList[arm].nextMultRunNumber();
		try
		{
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_DARK);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
				command+":"+e.toString());
			darkDone.setFilename(filename);
			darkDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+901);
			darkDone.setErrorString(e.toString());
			darkDone.setSuccessful(false);
			return darkDone;
		}
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
		if(saveFitsHeaders(command,darkDone,arm,filename) == false)
		{
			unLockFile(command,darkDone,filename);
			return darkDone;
		}
	// do exposure
		status.setExposureFilename(arm,filename);
		if(ccdEnable)
		{
			try
			{
				ccd.expose(false,-1,darkCommand.getExposureTime(),filename);
			}
			catch(CCDLibraryNativeException e)
			{
				frodospec.error(this.getClass().getName()+":processCommand:"+
						command+":",e);
				darkDone.setFilename(filename);
				darkDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+900);
				darkDone.setErrorString(e.toString());
				darkDone.setSuccessful(false);
				unLockFile(command,darkDone,filename);
				return darkDone;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,this.getClass().getName()+
				      ":processCommand:Did not do dark, ccd enable was false.");
		}
		// unlock FITS file lock created by saveFitsHeaders
		if(unLockFile(command,darkDone,filename) == false)
			return darkDone;
	// Test abort status.
		if(testAbort(command,darkDone) == true)
			return darkDone;
	// Call pipeline to reduce data.
		if(reduceCalibrate(command,darkDone,filename) == false)
			return darkDone; 
	// set return values to indicate success.
		darkDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		darkDone.setErrorString("");
		darkDone.setSuccessful(true);
	// return done object.
		return darkDone;
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
