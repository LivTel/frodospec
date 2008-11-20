// HardwareImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/HardwareImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import ngat.message.base.*;
import ngat.frodospec.ccd.*;
import ngat.phase2.*;

/**
 * This class provides the generic implementation of commands that use hardware to control a mechanism.
 * This is the SDSU CCD controller.
 * @version $Revision: 1.1 $
 */
public class HardwareImplementation extends CommandImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: HardwareImplementation.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");
	/**
	 * A reference to the CCDLibrary class instance used to communicate with the SDSU CCD Controller.
	 */
	protected CCDLibrary redCCD = null;
	/**
	 * A reference to the CCDLibrary class instance used to communicate with the SDSU CCD Controller.
	 */
	protected CCDLibrary blueCCD = null;

	/**
	 * This method calls the super-classes method. It then tries to fill in the reference to the hardware
	 * objects.
	 * @param command The command to be implemented.
	 * @see #frodospec
	 * @see #redCCD
	 * @see #blueCCD
	 * @see FrodoSpec#getCCD
	 */
	public void init(COMMAND command)
	{
		super.init(command);
		if(frodospec != null)
		{
			redCCD = frodospec.getCCD(FrodoSpecConfig.RED_ARM);
			blueCCD = frodospec.getCCD(FrodoSpecConfig.BLUE_ARM);
		}
	}

	/**
	 * This method is used to calculate how long an implementation of a command is going to take, so that the
	 * client has an idea of how long to wait before it can assume the server has died.
	 * @param command The command to be implemented.
	 * @return The time taken to implement this command, or the time taken before the next acknowledgement
	 * is to be sent.
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		return super.calculateAcknowledgeTime(command);
	}

	/**
	 * This routine performs the generic command implementation.
	 * @param command The command to be implemented.
	 * @return The results of the implementation of this command.
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		return super.processCommand(command);
	}
}

//
// $Log: not supported by cvs2svn $
//
