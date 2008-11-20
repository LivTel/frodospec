// FrodoSpecTCPServer.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecTCPServer.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

import java.lang.*;
import java.io.*;
import java.net.*;

import ngat.net.*;

/**
 * This class extends the TCPServer class for the FrodoSpec application.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FrodoSpecTCPServer extends TCPServer
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecTCPServer.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");
	/**
	 * Field holding the instance of the FrodoSpec currently executing, so we can pass this to spawned threads.
	 */
	private FrodoSpec frodospec = null;

	/**
	 * The constructor.
	 */
	public FrodoSpecTCPServer(String name,int portNumber)
	{
		super(name,portNumber);
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
	 * This routine spawns threads to handle connection to the server. This routine
	 * spawns <a href="FrodoSpecTCPServerConnectionThread.html">FrodoSpecTCPServerConnectionThread</a> threads.
	 * The routine also sets the new threads priority to higher than normal. This makes the thread
	 * reading it's command a priority so we can quickly determine whether the thread should
	 * continue to execute at a higher priority.
	 * @see FrodoSpecTCPServerConnectionThread
	 */
	public void startConnectionThread(Socket connectionSocket)
	{
		FrodoSpecTCPServerConnectionThread thread = null;

		thread = new FrodoSpecTCPServerConnectionThread(connectionSocket);
		thread.setFrodoSpec(frodospec);
		thread.setPriority(frodospec.getStatus().getThreadPriorityInterrupt());
		thread.start();
	}
}
//
// $Log: not supported by cvs2svn $
//
