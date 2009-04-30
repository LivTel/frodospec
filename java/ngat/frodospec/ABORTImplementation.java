// ABORTImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/ABORTImplementation.java,v 1.1 2009-04-30 09:53:40 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.phase2.FrodoSpecConfig;
import ngat.frodospec.ccd.*;

/**
 * This class provides the implementation for the ABORT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class ABORTImplementation extends INTERRUPTImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: ABORTImplementation.java,v 1.1 2009-04-30 09:53:40 cjm Exp $");

	/**
	 * Constructor.
	 */
	public ABORTImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.FRODOSPEC_ABORT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.FRODOSPEC_ABORT";
	}

	/**
	 * This method gets the ABORT command's acknowledge time. This takes the default acknowledge time to implement.
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
	 * This method implements the ABORT command. 
	 * <ul>
	 * <li>If the command is an instance of FRODOSPEC_ABORT, the arm parameter is extracted and abortArm
	 *     called for the specified arm.
	 * <li>If the command is an instance of ABORT, abortArm is called for each arm.
	 * <li>If the command is not an instance of the above, an ABORT_DONE is constructed and an error returned.
	 * </ul>
	 * Note this method can only be called with instances of FRODOSPEC_ABORT at the current time as thats what
	 * getImplementString returns. The ABORT (both-arm) implementation should really be moved to another
	 * implementation class if we ever want that functionality.
	 * @param command The abort command.
	 * @return An object of a subclass of COMMAND_DONE is returned (either ABORT_DONE or FRODOSPEC_ABORT_DONE).
	 * @see #abortArm
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		COMMAND_DONE done = null;
		ABORT abortCommand = null;
		ABORT_DONE abortDone = null;
		FRODOSPEC_ABORT frodospecAbortCommand = null;
		FRODOSPEC_ABORT_DONE frodospecAbortDone = null;

		// what class of command? cast command and setup done as appropriate
		if(command instanceof FRODOSPEC_ABORT)
		{
			frodospecAbortCommand = (FRODOSPEC_ABORT)command;
			frodospecAbortDone = new FRODOSPEC_ABORT_DONE(command.getId());
			done = (COMMAND_DONE)frodospecAbortDone;
			if(abortArm(frodospecAbortCommand.getArm(),frodospecAbortCommand,frodospecAbortDone) == false)
				return frodospecAbortDone;
		}
		else if (command instanceof ABORT)
		{
			abortCommand = (ABORT)command;
			abortDone = new ABORT_DONE(command.getId());
			done = (COMMAND_DONE)abortDone;
			for(int arm = FrodoSpecConfig.RED_ARM; arm <= FrodoSpecConfig.BLUE_ARM; arm++)
			{
				if(abortArm(arm,abortCommand,abortDone) == false)
					return abortDone;
			}
		}
		else
		{
			done = new ABORT_DONE(command.getId());
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Abort command has wrong class:"+
					command.getClass().getName());
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2400);
			done.setErrorString("Abort command has wrong class:"+
							    command.getClass().getName());
			done.setSuccessful(false);
			return done;
		}
	// return done object.
		done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		done.setErrorString("");
		done.setSuccessful(true);
		return done;
	}

	/**
	 * Abort any command operating on the specified arm.
	 * <ul>
	 * <li>The current exposure status is retrieved.
	 * <li>If current exposing, we try to stop the exposure.
	 * <li>If a setup is in progress, we try to abort the setup.
	 * <li>If this arm's grating resolution is unknown, we assume the grating is being moved and abort movement.
	 * <li>An ABORT commmand is sent to the DpRt.
	 * </ul>
	 * @param arm Which arm to abort, one of FrodoSpecConfig.RED_ARM or FrodoSpecConfig.BLUE_ARM.
	 * @param command The command instance, one of ABORT or FRODOSPEC_ABORT.
	 * @param done The command done instance (to fill in with errors if the abort fails), one of
	 *             ABORT_DONE or FRODOSPEC_ABORT_DONE.
	 * @see #ccd
	 * @see #frodospec
	 * @see #status
	 * @see #plc
	 * @see ngat.frodospec.ccd.CCDLibrary#getExposureStatus
	 * @see ngat.frodospec.ccd.CCDLibrary#abort
	 * @see ngat.frodospec.ccd.CCDLibrary#getSetupInProgress
	 * @see ngat.frodospec.ccd.CCDLibrary#setupAbort
	 * @see FrodoSpecStatus#getCurrentThread
	 * @see FrodoSpecTCPServerConnectionThread#setAbortProcessCommand
	 * @see FrodoSpec#getPLC
	 * @see FrodoSpec#sendDpRtCommand
	 * @see Plc#getGratingResolution
	 * @see Plc#abort
	 */
	protected boolean abortArm(int arm,COMMAND command,COMMAND_DONE done)
	{
		ngat.message.INST_DP.ABORT dprtAbort = null;
	        FrodoSpecTCPServerConnectionThread thread = null;
		CCDLibrary ccd = null;
		Plc plc = null;
		int currentStatus;

		// set ccd library based on arm
		if(arm == FrodoSpecConfig.RED_ARM)
			ccd = redCCD;
		else if(arm == FrodoSpecConfig.BLUE_ARM)
			ccd = blueCCD;
		else
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":Illegal arm:"+arm+".");
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2401);
			done.setErrorString("Illegal arm:"+arm+".");
			done.setSuccessful(false);
			return false;
		}
	// tell the thread itself to abort at a suitable point
		thread = (FrodoSpecTCPServerConnectionThread)status.getCurrentThread(arm);
		if(thread != null)
			thread.setAbortProcessCommand();
       // get current CCD status
		currentStatus = ccd.getExposureStatus();
		switch(currentStatus)
		{
			case CCDLibrary.EXPOSURE_STATUS_WAIT_START:
				break;
			case CCDLibrary.EXPOSURE_STATUS_EXPOSE:
				try
				{
					ccd.abort();
				}
				catch(CCDLibraryNativeException e)
				{
					frodospec.error(this.getClass().getName()+":Aborting exposure failed:",e);
					done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2402);
					done.setErrorString(e.toString());
					done.setSuccessful(false);
					return false;
				}
				break;
			case CCDLibrary.EXPOSURE_STATUS_PRE_READOUT:
				break;
			case CCDLibrary.EXPOSURE_STATUS_PRE_EXPOSE_READOUT:
			case CCDLibrary.EXPOSURE_STATUS_READOUT:
				break;
			case CCDLibrary.EXPOSURE_STATUS_POST_READOUT:
				break;
			case CCDLibrary.EXPOSURE_STATUS_NONE:
			default:
				break;
		}
	// abort CCD setup
		if(ccd.getSetupInProgress())
			ccd.setupAbort();
	// abort grating movement ops
		try
		{
			plc = frodospec.getPLC();
			if(plc.getGratingResolution(arm) == 0)// 0 means unknown/in-transit
			{
				// This will abort movement on either arm.
				plc.abort();
			}
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":Aborting grating failed:",e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+2403);
			done.setErrorString(e.toString());
			done.setSuccessful(false);
			return false;
		}
	// no need to abort newmark (focus stage) opes as these only occur at startup at the moment
	// abort the dprt
		dprtAbort = new ngat.message.INST_DP.ABORT(command.getId());
		frodospec.sendDpRtCommand(dprtAbort,serverConnectionThread);
		return true;
	}
}

//
// $Log: not supported by cvs2svn $
//
