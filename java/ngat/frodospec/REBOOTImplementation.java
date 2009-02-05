// REBOOTImplementation.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/REBOOTImplementation.java,v 1.3 2009-02-05 11:38:59 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.IOException;
import ngat.frodospec.ccd.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.REBOOT;
import ngat.util.ICSDRebootCommand;
import ngat.util.ICSDShutdownCommand;
import ngat.util.logging.*;

/**
 * This class provides the implementation for the REBOOT command sent to a server using the
 * Java Message System.
 * @author Chris Mottram
 * @version $Revision: 1.3 $
 */
public class REBOOTImplementation extends INTERRUPTImplementation implements JMSCommandImplementation
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: REBOOTImplementation.java,v 1.3 2009-02-05 11:38:59 cjm Exp $");
	/**
	 * Class constant used in calculating acknowledge times, when the acknowledge time connot be found in the
	 * configuration file.
	 */
	public final static int DEFAULT_ACKNOWLEDGE_TIME = 		300000;
	/**
	 * String representing the root part of the property key used to get the acknowledge time for 
	 * a certain level of reboot.
	 */
	public final static String ACK_TIME_PROPERTY_KEY_ROOT =	    "frodospec.reboot.acknowledge_time.";
	/**
	 * String representing the root part of the property key used to decide whether a certain level of reboot
	 * is enabled.
	 */
	public final static String ENABLE_PROPERTY_KEY_ROOT =       "frodospec.reboot.enable.";
	/**
	 * Set of constant strings representing levels of reboot. The levels currently start at 1, so index
	 * 0 is currently "NONE". These strings need to be kept in line with level constants defined in
	 * ngat.message.ISS_INST.REBOOT.
	 */
	public final static String REBOOT_LEVEL_LIST[] =  {"NONE","REDATUM","SOFTWARE","HARDWARE","POWER_OFF"};

	/**
	 * Constructor.
	 */
	public REBOOTImplementation()
	{
		super();
	}

	/**
	 * This method allows us to determine which class of command this implementation class implements.
	 * This method returns &quot;ngat.message.ISS_INST.REBOOT&quot;.
	 * @return A string, the classname of the class of ngat.message command this class implements.
	 */
	public static String getImplementString()
	{
		return "ngat.message.ISS_INST.REBOOT";
	}

	/**
	 * This method gets the REBOOT command's acknowledge time. This time is dependant on the level.
	 * This is calculated as follows:
	 * <ul>
	 * <li>If the level is LEVEL_REDATUM, the number stored in &quot; 
	 * frodospec.reboot.acknowledge_time.REDATUM &quot; in the FrodoSpec properties file is the timeToComplete.
	 * <li>If the level is LEVEL_SOFTWARE, the number stored in &quot; 
	 * frodospec.reboot.acknowledge_time.SOFTWARE &quot; in the FrodoSpec properties file is the timeToComplete.
	 * <li>If the level is LEVEL_HARDWARE, the number stored in &quot; 
	 * frodospec.reboot.acknowledge_time.HARDWARE &quot; in the FrodoSpec properties file is the timeToComplete.
	 * <li>If the level is LEVEL_POWER_OFF, the number stored in &quot; 
	 * frodospec.reboot.acknowledge_time.POWER_OFF &quot; in the FrodoSpec properties file is the timeToComplete.
	 * </ul>
	 * If these numbers cannot be found, the default number DEFAULT_ACKNOWLEDGE_TIME is used instead.
	 * <br>Note, this return value is irrelevant in the SOFTWARE,HARDWARE and POWER_OFF cases, 
	 * the client does not expect a DONE message back from
	 * the process as FrodoSpec should restart in the implementation of this command.
	 * However, the value returned here will be how long the client waits before trying to restart communications
	 * with the FrodoSpec server, so a reasonable value here may be useful.
	 * @param command The command instance we are implementing.
	 * @return An instance of ACK with the timeToComplete set to a time (in milliseconds).
	 * @see #DEFAULT_ACKNOWLEDGE_TIME
	 * @see #ACK_TIME_PROPERTY_KEY_ROOT
	 * @see #REBOOT_LEVEL_LIST
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see FrodoSpecStatus#getPropertyInteger
	 */
	public ACK calculateAcknowledgeTime(COMMAND command)
	{
		ngat.message.ISS_INST.REBOOT rebootCommand = (ngat.message.ISS_INST.REBOOT)command;
		ACK acknowledge = null;
		int timeToComplete = 0;

		acknowledge = new ACK(command.getId()); 
		try
		{
			timeToComplete = frodospec.getStatus().getPropertyInteger(ACK_TIME_PROPERTY_KEY_ROOT+
								   REBOOT_LEVEL_LIST[rebootCommand.getLevel()]);
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+":calculateAcknowledgeTime:"+
					rebootCommand.getLevel(),e);
			timeToComplete = DEFAULT_ACKNOWLEDGE_TIME;
		}
	//set time and return
		acknowledge.setTimeToComplete(timeToComplete);
		return acknowledge;
	}

	/**
	 * This method implements the REBOOT command. 
	 * An object of class REBOOT_DONE is returned.
	 * The <i>frodospec.reboot.enable.&lt;level&gt;</i> property is checked to see to whether to really
	 * do the specified level of reboot. This enables us to say, disbale to POWER_OFF reboot, if the
	 * instrument control computer is not connected to an addressable power supply.
	 * The following four levels of reboot are recognised:
	 * <ul>
	 * <li>REDATUM. This shuts down the connection to the controller, and then
	 * 	restarts it.
	 * <li>SOFTWARE. This shuts down the connection to the SDSU CCD Controller and closes the
	 * 	server socket using the FrodoSpec close method. It then exits the FrodoSpec.
	 * <li>HARDWARE. This shuts down the connection to the SDSU CCD Controller and closes the
	 * 	server socket using the FrodoSpec close method. It then issues a reboot
	 * 	command to the underlying operating system, to restart the instrument computer.
	 * <li>POWER_OFF. This shuts down the connection to the SDSU CCD Controller and closes the
	 * 	server socket using the FrodoSpec close method. It then issues a shutdown
	 * 	command to the underlying operating system, to put the instrument computer into a state
	 * 	where power can be switched off.
	 * </ul>
	 * Note: You need to perform at least a SOFTWARE level reboot to re-read the FrodoSpec configuration file,
	 * as it contains information such as server ports.
	 * @param command The command instance we are implementing.
	 * @return An instance of REBOOT_DONE. Note this is only returned on a REDATUM level reboot,
	 * all other levels cause the FrodoSpec to terminate (either directly or indirectly) and a DONE
	 * message cannot be returned.
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_REDATUM
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_SOFTWARE
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_HARDWARE
	 * @see ngat.message.ISS_INST.REBOOT#LEVEL_POWER_OFF
	 * @see #ENABLE_PROPERTY_KEY_ROOT
	 * @see #REBOOT_LEVEL_LIST
	 * @see FrodoSpec#close
	 * @see FrodoSpec#shutdownHardware
	 * @see FrodoSpec#startupHardware
	 */
	public COMMAND_DONE processCommand(COMMAND command)
	{
		ngat.message.ISS_INST.REBOOT rebootCommand = (ngat.message.ISS_INST.REBOOT)command;
		ngat.message.ISS_INST.REBOOT_DONE rebootDone = new ngat.message.ISS_INST.REBOOT_DONE(command.getId());
		ngat.message.INST_DP.REBOOT dprtReboot = new ngat.message.INST_DP.REBOOT(command.getId());
		ICSDRebootCommand icsdRebootCommand = null;
		ICSDShutdownCommand icsdShutdownCommand = null;
		FrodoSpecREBOOTQuitThread quitThread = null;
		boolean enable;

		try
		{
			frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
				      rebootCommand.getClass().getName()+":level = "+rebootCommand.getLevel()+".");
			// is reboot enabled at this level
			enable = frodospec.getStatus().getPropertyBoolean(ENABLE_PROPERTY_KEY_ROOT+
							   REBOOT_LEVEL_LIST[rebootCommand.getLevel()]);
			// if not enabled return OK
			if(enable == false)
			{
				frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
					   rebootCommand.getClass().getName()+":Level:"+rebootCommand.getLevel()+
					   " is not enabled.");
				rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
				rebootDone.setErrorString("");
				rebootDone.setSuccessful(true);
				return rebootDone;
			}
			// do relevent reboot based on level
			switch(rebootCommand.getLevel())
			{
				case REBOOT.LEVEL_REDATUM:
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
						      rebootCommand.getClass().getName()+":Redatum starting.");
					frodospec.shutdownHardware();
					frodospec.reInit();
					frodospec.startupHardware();
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
						      rebootCommand.getClass().getName()+":Redatum finished.");
					break;
				case REBOOT.LEVEL_SOFTWARE:
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
						      rebootCommand.getClass().getName()+":Software reboot starting.");
				// send REBOOT to the data pipeline
					dprtReboot.setLevel(rebootCommand.getLevel());
					frodospec.sendDpRtCommand(dprtReboot,serverConnectionThread);
				// Don't check DpRt done, chances are the DpRt quit before sending the done message.
					frodospec.close();
					quitThread = new FrodoSpecREBOOTQuitThread("quit:"+rebootCommand.getId());
					quitThread.setFrodoSpec(frodospec);
					quitThread.setWaitThread(serverConnectionThread);
					quitThread.start();
					break;
				case REBOOT.LEVEL_HARDWARE:
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
						      rebootCommand.getClass().getName()+":Hardware reboot starting.");
				// send REBOOT to the data pipeline
					dprtReboot.setLevel(rebootCommand.getLevel());
					frodospec.sendDpRtCommand(dprtReboot,serverConnectionThread);
				// Don't check DpRt done, chances are the DpRt quit before sending the done message.
				// We don't call close to minimise computer lock-ups.
				// send reboot to the icsd_inet
					icsdRebootCommand = new ICSDRebootCommand();
					icsdRebootCommand.send();
					break;
				case REBOOT.LEVEL_POWER_OFF:
					frodospec.log(Logger.VERBOSITY_VERY_TERSE,"Command:"+
						   rebootCommand.getClass().getName()+":Power off reboot starting.");
				// send REBOOT to the data pipeline
					dprtReboot.setLevel(rebootCommand.getLevel());
					frodospec.sendDpRtCommand(dprtReboot,serverConnectionThread);
				// Don't check done, chances are the DpRt quit before sending the done message.
				// We don't call close to minimise computer lock-ups.
				// send shutdown to the icsd_inet
					icsdShutdownCommand = new ICSDShutdownCommand();
					icsdShutdownCommand.send();
					break;
				default:
					frodospec.error(this.getClass().getName()+
						":processCommand:"+command+":Illegal level:"+rebootCommand.getLevel());
					rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1400);
					rebootDone.setErrorString("Illegal level:"+rebootCommand.getLevel());
					rebootDone.setSuccessful(false);
					return rebootDone;
			};// end switch
		}
		catch(CCDLibraryNativeException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":",e);
			rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1401);
			rebootDone.setErrorString(e.toString());
			rebootDone.setSuccessful(false);
			return rebootDone;
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":",e);
			rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1402);
			rebootDone.setErrorString(e.toString());
			rebootDone.setSuccessful(false);
			return rebootDone;
		}
		catch(InterruptedException e)
		{
			frodospec.error(this.getClass().getName()+
				":processCommand:"+command+":",e);
			rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1403);
			rebootDone.setErrorString(e.toString());
			rebootDone.setSuccessful(false);
			return rebootDone;
		}
		catch(Exception e)
		{
			frodospec.error(this.getClass().getName()+
					":processCommand:"+command+":",e);
			rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+1404);
			rebootDone.setErrorString(e.toString());
			rebootDone.setSuccessful(false);
			return rebootDone;
		}
	// return done object.
		rebootDone.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_NO_ERROR);
		rebootDone.setErrorString("");
		rebootDone.setSuccessful(true);
		return rebootDone;
	}
}

//
// $Log: not supported by cvs2svn $
// Revision 1.2  2008/12/15 11:49:18  cjm
// Added logging.
//
// Revision 1.1  2008/11/20 11:33:35  cjm
// Initial revision
//
//
