// FrodoSpecTCPClientConnectionThread.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecTCPClientConnectionThread.java,v 1.1 2009-05-07 15:35:04 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;
import java.net.*;
import java.util.Date;

import ngat.net.*;
import ngat.message.base.*;

/**
 * The FrodoSpecTCPClientConnectionThread extends TCPClientConnectionThread. 
 * It implements the generic ISS/DP(RT) instrument command protocol with multiple acknowledgements. 
 * FrodoSpec starts one of these threads each time
 * it wishes to send a message to the ISS/DP(RT).
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FrodoSpecTCPClientConnectionThread extends TCPClientConnectionThreadMA
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecTCPClientConnectionThread.java,v 1.1 2009-05-07 15:35:04 cjm Exp $");
	/**
	 * The commandThread was spawned by the FrodoSpec to deal with a FrodoSpec command request. 
	 * As part of the running of
	 * the commandThread, this client connection thread was created. We need to know the server thread so
	 * that we can pass back any acknowledge times from the ISS/DpRt back to the FrodoSpec client (ISS/IcsGUI etc).
	 */
	private FrodoSpecTCPServerConnectionThread commandThread = null;
	/**
	 * The FrodoSpec object.
	 */
	private FrodoSpec frodospec = null;

	/**
	 * A constructor for this class. Currently just calls the parent class's constructor.
	 * @param address The internet address to send this command to.
	 * @param portNumber The port number to send this command to.
	 * @param c The command to send to the specified address.
	 * @param ct The FrodoSpec command thread, the implementation of which spawned this command.
	 */
	public FrodoSpecTCPClientConnectionThread(InetAddress address,int portNumber,COMMAND c,
		FrodoSpecTCPServerConnectionThread ct)
	{
		super(address,portNumber,c);
		commandThread = ct;
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
	 * This routine processes the acknowledge object returned by the server. It
	 * prints out a message, giving the time to completion if the acknowledge was not null.
	 * It sends the acknowledgement to the FrodoSpec client for this sub-command of the command,
	 * so that the FrodoSpec's client does not time out if,say, a zero is returned.
	 * @see FrodoSpecTCPServerConnectionThread#sendAcknowledge
	 * @see #commandThread
	 */
	protected void processAcknowledge()
	{
		if(acknowledge == null)
		{
			frodospec.error(this.getClass().getName()+":processAcknowledge:"+
				command.getClass().getName()+":acknowledge was null.");
			return;
		}
	// send acknowledge to FrodoSpec client.
		try
		{
			commandThread.sendAcknowledge(acknowledge);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+":processAcknowledge:"+
				command.getClass().getName()+":sending acknowledge to client failed:",e);
		}
	}

	/**
	 * This routine processes the done object returned by the server. 
	 * It prints out the basic return values in done.
	 * @see #frodospec
	 */
	protected void processDone()
	{
		ACK acknowledge = null;

		if(done == null)
		{
			frodospec.error(this.getClass().getName()+":processDone:"+
				command.getClass().getName()+":done was null.");
			return;
		}
	// construct an acknowledgement to sent to the FrodoSpec client to tell it how long to keep waiting
	// it currently returns the time the FrodoSpec origianally asked for to complete this command
	// This is because the FrodoSpec assumed zero time for all sub-commands.
		acknowledge = new ACK(command.getId());
		acknowledge.setTimeToComplete(commandThread.getAcknowledgeTime());
		try
		{
			commandThread.sendAcknowledge(acknowledge);
		}
		catch(IOException e)
		{
			frodospec.error(this.getClass().getName()+":processDone:"+
				command.getClass().getName()+":sending acknowledge to client failed:",e);
		}
	}
}
//
// $Log: not supported by cvs2svn $
//
