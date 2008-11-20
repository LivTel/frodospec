// FrodoSpecTCPServerConnectionThread.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecTCPServerConnectionThread.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.*;

import ngat.net.*;
import ngat.message.base.*;
import ngat.message.ISS_INST.*;
import ngat.message.INST_DP.CALIBRATE_REDUCE;
import ngat.message.INST_DP.CALIBRATE_REDUCE_DONE;
import ngat.message.INST_DP.EXPOSE_REDUCE;
import ngat.message.INST_DP.EXPOSE_REDUCE_DONE;
import ngat.message.INST_DP.INST_TO_DP_DONE;
import ngat.phase2.*;

/**
 * This class extends the TCPServerConnectionThread class for the FrodoSpec application.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FrodoSpecTCPServerConnectionThread extends TCPServerConnectionThread
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecTCPServerConnectionThread.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");
	/**
	 * Default time taken to respond to a command. This is a class-wide field.
	 */
	private static int defaultAcknowledgeTime = 60*1000;
	/**
	 * Time taken to respond to a command not implemented on this instrument. This is a class-wide field.
	 */
	private static int minAcknowledgeTime = 30*1000;
	/**
	 * The FrodoSpec object.
	 */
	private FrodoSpec frodospec = null;
	/**
	 * This reference stores the instance of the class that implements the command passed to this thread.
	 * This can be retrieved from the frodospec main object, which has a Hashtable with mappings between
	 * ngat.message class names and implementations of these commands.
	 * @see JMSCommandImplementation
	 * @see FrodoSpec#getImplementation
	 */
	JMSCommandImplementation commandImplementation = null;
	/**
	 * Variable used to track whether we should continue processing the command this process is meant to
	 * process. If the ISS has sent an ABORT message this variable is set to true using
	 * <a href="#setAbortProcessCommand">setAbortProcessCommand</a>, and then the 
	 * <a href="#processCommand">processCommand</a> routine should tidy up and return to stop this thread.
	 * @see #setAbortProcessCommand
	 * @see #getAbortProcessCommand
	 * @see #processCommand
	 */
	private boolean abortProcessCommand = false;
	/**
	 * Field holding the results of the JMSCommandImplementation.calculateAcknowledgeTime call in
	 * the calculateAcknowledgeTime method over-ridden from the default. We need this when
	 * calculating remaining acknowledge time after a sub-command call to the ISS/DpRt.
	 * @see JMSCommandImplementation#calculateAcknowledgeTime
	 * @see #commandImplementation
	 */
	private int acknowledgeTime = 0;

	/**
	 * Constructor of the thread. This just calls the superclass constructors.
	 * @param connectionSocket The socket the thread is to communicate with.
	 */
	public FrodoSpecTCPServerConnectionThread(Socket connectionSocket)
	{
		super(connectionSocket);
	}

	/**
	 * Class method to set the value of <a href="#defaultAcknowledgeTime">defaultAcknowledgeTime</a>. 
	 * @see #defaultAcknowledgeTime
	 */
	public static void setDefaultAcknowledgeTime(int m)
	{
		defaultAcknowledgeTime = m;
	}

	/**
	 * Class method to get the value set for the <a href="#defaultAcknowledgeTime">defaultAcknowledgeTime</a>. 
	 * @see #defaultAcknowledgeTime
	 */
	public static int getDefaultAcknowledgeTime()
	{
		return defaultAcknowledgeTime;
	}

	/**
	 * Class method to set the value of <a href="#minAcknowledgeTime">minAcknowledgeTime</a>. 
	 * @see #minAcknowledgeTime
	 */
	public static void setMinAcknowledgeTime(int m)
	{
		minAcknowledgeTime = m;
	}

	/**
	 * Class method to get the value set for the <a href="#minAcknowledgeTime">minAcknowledgeTime</a>. 
	 * @see #minAcknowledgeTime
	 */
	public static int getMinAcknowledgeTime()
	{
		return minAcknowledgeTime;
	}

	/**
	 * Routine to set this objects pointer to the frodospec object.
	 * @param f The frodospec object.
	 */
	public void setFrodoSpec(FrodoSpec f)
	{
		this.frodospec = f;
	}

	/**
	 * Routine called by another thread to stop this
	 * thread implementing a command it has been sent. This variable should cause the processCommand
	 * method to return as soon as possible. The processCommand should still create a COMMAND_DONE
	 * object and fill it in with a suitable abort message. The processCommand should also undo any
	 * operation it has half completed - e.g. switch the autoguider off.
	 * The rest of this thread's run method should then execute
	 * to send the DONE message back to the client.
	 * @see #abortProcessCommand
	 */
	public synchronized void setAbortProcessCommand()
	{
		abortProcessCommand = true;
	}

	/**
	 * Method to return whether this thread has been requested to stop what it is processing.
	 * @see #abortProcessCommand
	 */
	public synchronized boolean getAbortProcessCommand()
	{
		return abortProcessCommand;
	}

	/**
	 * This method is called after the clients command is read over the socket. It allows us to
	 * initialise this threads response to a command. This method changes the threads priority now 
	 * that the command's class is known, if it is a sub-class of INTERRUPT the priority is higher.<br>
	 * It also finds the command implementation used to run this command, got from the mapping
	 * stored in the FrodoSpec object. It sets up the implementation objects references to the FrodoSpec main
	 * object and this connection thread. It then runs the command implementation's init routine, to
	 * initialise the implementation.
	 * @see INTERRUPT
	 * @see Thread#setPriority
	 * @see FrodoSpec#getImplementation
	 * @see CommandImplementation#setFrodoSpec
	 * @see CommandImplementation#setServerConnectionThread
	 * @see JMSCommandImplementation#init
	 * @see #commandImplementation
	 */
	protected void init()
	{
	// set the threads priority
		if(command instanceof INTERRUPT)
			this.setPriority(frodospec.getStatus().getThreadPriorityInterrupt());
		else
			this.setPriority(frodospec.getStatus().getThreadPriorityNormal());
	// get the implementation - this never returns null.
		commandImplementation = frodospec.getImplementation(command.getClass().getName());
	// initialises the command implementations response
		((CommandImplementation)commandImplementation).setFrodoSpec(frodospec);
		((CommandImplementation)commandImplementation).setServerConnectionThread(this);
		commandImplementation.init(command);
	}

	/**
	 * This method calculates the time it will take for the command to complete and is called
	 * from the classes inherited run method. It calls the command implementation to calculate this
	 * as this will calculate a suitable value. It stores the result in the acknowledgTime field
	 * for later reference before returning.
	 * @return An instance of a (sub)class of ngat.message.base.ACK is returned, with the timeToComplete
	 * 	field set to the time the command will take to process.
	 * @see ngat.message.base.ACK
	 * @see ngat.message.base.ACK#setTimeToComplete
	 * @see #commandImplementation
	 * @see JMSCommandImplementation#calculateAcknowledgeTime
	 * @see #acknowledgeTime
	 */
	protected ACK calculateAcknowledgeTime()
	{
		ACK acknowledge = null;

		acknowledge =  commandImplementation.calculateAcknowledgeTime(command);
		if(acknowledge != null)
			acknowledgeTime = acknowledge.getTimeToComplete();
		else
			acknowledgeTime = 0;
		return acknowledge;
	}

	/**
	 * This method overrides the processCommand method in the ngat.net.TCPServerConnectionThread class.
	 * It is called from the inherited run method. It is responsible for performing the commands
	 * sent to it by the ISS. It should also construct the done object to describe the results of the command.<br>
	 * <ul>
	 * <li>This method checks whether the command in null and returns a generic done error message if this is the 
	 * case.
	 * <li>If suitable logging is enabled the command is logged.
	 * <li>The thread checks whether the command passed to it can be run, using the commandCanBeRun method.
	 * If it cannot a suitable done error message is returned.
	 * <li>If the command is not an interrupt command sub-class it sets the FrodoSpec's status to reflect
	 * the command/thread(this one) currently doing the processing.
	 * <li>This method delagates the command processing to the command implementation found for the command
	 * message class.
	 * <li>The FrodoSpec's status is again updated to reflect this command/thread has finished processing. (If it's
	 * not a sub-class of INTERRUPT again).
	 * <li>If suitable logging is enabled the command is logged as completed.
	 * </ul>
	 * @see FrodoSpecStatus#getLogLevel
	 * @see FrodoSpec#log
	 * @see FrodoSpecStatus#commandCanBeRun
	 * @see FrodoSpecStatus#setCurrentCommand
	 * @see #commandImplementation
	 * @see JMSCommandImplementation#processCommand
	 */
	protected void processCommand()
	{
	// setup a generic done object until the command specific one is constructed.
		done = new COMMAND_DONE(command.getId());

		if(command == null)
		{
			frodospec.error("processCommand:command was null.");
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+100);
			done.setErrorString("processCommand:command was null.");
			done.setSuccessful(false);
			return;
		}
		frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,"Command:"+command.getClass().getName()+
			" Started.");
		if(!frodospec.getStatus().commandCanBeRun((ISS_TO_INST)command))
		{
			// frodospec.getStatus().getCurrentCommand() may have been set to null between
			// the commandCanBeRun test above and the getCurrentCommand routine below.
			// We must allow for this when generating the error string.
			ISS_TO_INST currentCommand = frodospec.getStatus().getCurrentCommand((ISS_TO_INST)command);
			String currentCommandString = null;

			if(currentCommand != null)
				currentCommandString = new String(currentCommand.getClass().getName());
			else
				currentCommandString = new String("Unknown Command");
			String s = new String("processCommand:command `"+command.getClass().getName()+
				"'could not be run:Command `"+
				currentCommandString+"' already running.");
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+101);
			done.setErrorString(s);
			done.setSuccessful(false);
			frodospec.error(s);
			return;
		}
	// This test says interupt class commands should not become current command.
	// This class of commands probably want to see what the current command is anyway.
		if(!(command instanceof INTERRUPT))
		{
			frodospec.getStatus().setCurrentCommand((ISS_TO_INST)command);
		}
	// setup return object.
		try
		{
			done = commandImplementation.processCommand(command);
		}
	// We want to catch unthrown exceptions here - so that we can (almost) guarantee
	// the status's current command is reset to null.
		catch(Exception e)
		{
			String s = new String(this.getClass().getName()+":processCommand failed:");
			frodospec.error(s,e);
			done.setErrorNum(FrodoSpecConstants.FRODOSPEC_ERROR_CODE_BASE+102);
			done.setErrorString(s+e);
			done.setSuccessful(false);
		}
	// change FrodoSpec status once command has been done
		if(!(command instanceof INTERRUPT))
		{
			frodospec.getStatus().clearCurrentCommand((ISS_TO_INST)command);
		}
	// log command/done
		frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_COMMANDS,"Command:"+command.getClass().getName()+
			" Completed.");
		frodospec.log(FrodoSpecConstants.FRODOSPEC_LOG_LEVEL_REPLIES,"Done:"+done.getClass().getName()+
			":successful:"+done.getSuccessful()+
			":error number:"+done.getErrorNum()+":error string:"+done.getErrorString());
	}

	/**
	 * This routine sends an acknowledge back to the client.
	 * @param acknowledge The acknowledge object to send back to the client.
	 * @param setThreadAckTime If true, the local instance of <b>acknowledgeTime</b> is updated
	 * 	with the acknowledge object's time to complete.
	 * @exception NullPointerException If the acknowledge object is null this exception is thrown.
	 * @exception IOException If the acknowledge object fails to be sent an IOException results.
	 * @see #acknowledgeTime
	 * @see ngat.net.TCPServerConnectionThread#sendAcknowledge
	 */
	public void sendAcknowledge(ACK acknowledge,boolean setThreadAckTime) throws IOException
	{
		if(setThreadAckTime)
			acknowledgeTime = acknowledge.getTimeToComplete();
		super.sendAcknowledge(acknowledge);
	}

	/**
	 * Return the initial time the implementation thought it would take to complete this command.
	 * @return The acknowledge time, zero if the calculateAcknowledgTime routine has not been called yet,
	 *	or an acknowledge object was not returned.
	 * @see #acknowledgeTime
	 */
	public int getAcknowledgeTime()
	{
		return acknowledgeTime;
	}

}
//
// $Log: not supported by cvs2svn $
//
