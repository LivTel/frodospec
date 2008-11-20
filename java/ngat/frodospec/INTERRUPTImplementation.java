// INTERRUPTImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/INTERRUPTImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import ngat.frodospec.ccd.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.INTERRUPT_DONE;

/**
 * This class provides the generic implementation for INTERRUPT commands sent to a server using the
 * Java Message System. It extends FITSImplementation, as INTERRUPT commands need to be able to
 * abort SDSU CCD Controller commands.
 * @see FITSImplementation
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class INTERRUPTImplementation extends FITSImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: INTERRUPTImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");

	/**
	 * This method gets the INTERRUPT command's acknowledge time. It returns the server connection 
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
	 * This method is a generic implementation for the INTERRUPT command, that does nothing.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
	       	// do nothing 
		INTERRUPT_DONE interruptDone = new INTERRUPT_DONE(command.getId());

		interruptDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		interruptDone.setErrorString("");
		interruptDone.setSuccessful(true);
		return interruptDone;
	}
}

//
// $Log: not supported by cvs2svn $
//
