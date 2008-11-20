// BIASImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/BIASImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import ngat.frodospec.ccd.*;
import ngat.fits.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.FRODOSPEC_BIAS;
import ngat.message.ISS_INST.FRODOSPEC_BIAS_DONE;
import ngat.phase2.FrodoSpecConfig;

/**
 * This class provides the implementation for the BIAS command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class BIASImplementation extends CALIBRATEImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: BIASImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");

	/**
	 * Constructor.
	 */
	public BIASImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_BIAS&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_BIAS";
	}

	/**
	 * This method gets the BIAS command's acknowledge time. The BIAS command has no exposure time, 
	 * so this returns the server connection threads default acknowledge time if available.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set.
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecTCPServerConnectionThread#getDefaultAcknowledgeTime
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ACK acknowledge = null;

		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(serverConnectionThread.getDefaultAcknowledgeTime());
		return acknowledge;
	}

	/**
	 * This method implements the BIAS command. It generates some FITS headers from the CCD setup and
	 * the ISS and saves this to disc. It performs a bias exposure and saves the data from this to disc.
	 * It sends the generated FITS data to the Real Time Data Pipeline to get some data from it.
	 * The resultant data or the relevant error code is put into the an object of class BIAS_DONE and
	 * returned. During execution of these operations the abort flag is tested to see if we need to
	 * stop the implementation of this command.
	 * @see CommandImplementation#testAbort
	 * @see FITSImplementation#frodospecFilenameList
	 * @see FITSImplementation#checkNonWindowedSetup
	 * @see FITSImplementation#clearFitsHeaders
	 * @see FITSImplementation#setFitsHeaders
	 * @see FITSImplementation#getFitsHeadersFromISS
	 * @see FITSImplementation#saveFitsHeaders
	 * @see ngat.frodospec.ccd.CCDLibrary#bias
	 * @see CALIBRATEImplementation#reduceCalibrate
	 * @see FrodoSpecConstants#ARM_STRING_LIST
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		FRODOSPEC_BIAS frodospecBiasCommand = null;
		FRODOSPEC_BIAS_DONE frodospecBiasDone = new FRODOSPEC_BIAS_DONE(command.getId());
		CCDLibrary ccd = null;
		String filename = null;
		int arm;
		boolean ccdEnable;

		if((command instanceof FRODOSPEC_BIAS) == false)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Bias command has wrong class:"+
					command.getClass().getName());
			frodospecBiasDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+702);
			frodospecBiasDone.setErrorString("Bias command has wrong class:"+
							    command.getClass().getName());
			frodospecBiasDone.setSuccessful(false);
			return frodospecBiasDone;
		}
		// get arm from command
		frodospecBiasCommand = (FRODOSPEC_BIAS)command;
		arm = frodospecBiasCommand.getArm();
		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		if(testAbort(command,frodospecBiasDone) == true)
			return frodospecBiasDone;
		if(checkNonWindowedSetup(arm,frodospecBiasDone) == false)
			return frodospecBiasDone;
	// fits headers
		clearFitsHeaders(arm);
		if(setFitsHeaders(command,frodospecBiasDone,arm,FitsHeaderDefaults.OBSTYPE_VALUE_BIAS,0) == false)
			return frodospecBiasDone;
		if(getFitsHeadersFromISS(command,frodospecBiasDone,arm) == false)
			return frodospecBiasDone;
		if(testAbort(command,frodospecBiasDone) == true)
			return frodospecBiasDone;
	// get a filename to store frame in
		frodospecFilenameList[arm].nextMultRunNumber();
		try
		{
			ccdEnable = status.getPropertyBoolean("frodospec.ccd."+
							      FrodoSpecConstants.ARM_STRING_LIST[arm]+".enable");
			frodospecFilenameList[arm].setExposureCode(FitsFilename.EXPOSURE_CODE_BIAS);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":processCommand:"+
				command+":"+e.toString());
			frodospecBiasDone.setFilename(filename);
			frodospecBiasDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+701);
			frodospecBiasDone.setErrorString(e.toString());
			frodospecBiasDone.setSuccessful(false);
			return frodospecBiasDone;
		}
		frodospecFilenameList[arm].nextRunNumber();
		filename = frodospecFilenameList[arm].getFilename();
		if(saveFitsHeaders(command,frodospecBiasDone,arm,filename) == false)
			return frodospecBiasDone;
	// do exposure
		if(ccdEnable)
		{
			try
			{
				ccd.bias(filename);
			}
			catch(CCDLibraryNativeException e)
			{
				frodospec.error(this.getClass().getName()+":processCommand:"+
						command+":"+e.toString());
				frodospecBiasDone.setFilename(filename);
				frodospecBiasDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+700);
				frodospecBiasDone.setErrorString(e.toString());
				frodospecBiasDone.setSuccessful(false);
				return frodospecBiasDone;
			}
		}// end if ccdEnable
		else
		{
			frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,this.getClass().getName()+
				      ":processCommand:Did not do bias, ccd enable was false.");
		}
	// Test abort status.
		if(testAbort(command,frodospecBiasDone) == true)
			return frodospecBiasDone;
	// Call pipeline to reduce data.
		if(reduceCalibrate(command,frodospecBiasDone,filename) == false)
			return frodospecBiasDone;
	// set the done object to indicate success.
		frodospecBiasDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		frodospecBiasDone.setErrorString("");
		frodospecBiasDone.setSuccessful(true);
	// return done object.
		return frodospecBiasDone;
	}
}

//
// $Log: not supported by cvs2svn $
//
