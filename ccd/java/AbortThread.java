// AbortThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/AbortThread.java,v 0.4 2000-01-24 16:21:05 cjm Exp $
import java.io.*;

/**
 * This class is a thread which when run, looks for a keypress on System.in, and then calls the parents
 * abort method.
 * @author Chris Mottram
 * @version $Revision: 0.4 $
 */
class AbortThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: AbortThread.java,v 0.4 2000-01-24 16:21:05 cjm Exp $");
	/**
	 * Parent class, call it's abort method if a keypress is detected.
	 */
	private Test parent = null;
	/**
	 * Boolean used to decide when to stop running this thread.
	 */
	private boolean quit = false;

	/**
	 * Constructor for the thread. 
	 * @param test The parent object whose abort method is called if a keypress is detected.
	 */
	public AbortThread(Test test)
	{
		this.parent = test;
	}

	/**
	 * Run method for this thread. The thread yields while there is no character in System.in's buffer.
	 * If a keypress is detected it is read, and then the parents abort method is called.
	 */
	public void run()
	{
		int retval = 0;

		try
		{
			quit = false;
			while((System.in.available() == 0)&&(quit == false))
				Thread.yield();
			if(System.in.available() != 0)
				retval = System.in.read();
		}
		catch (IOException e)
		{
			System.err.println(this.getClass().getName()+":run:available/read:"+e);
			retval = -1;
		}
		if((retval >= 0)&&(parent != null))
			parent.abort();
	}

	/**
	 * Method to quit (stop) the abort thread. Used to replace the stop method which is
 	 * deprecated in JDK1.2.x.
	 * @see #quit
	 */
	public void quit()
	{
		quit = true;
	}
}
 
//
// $Log: not supported by cvs2svn $
// Revision 0.3  1999/09/08 10:52:40  cjm
// Trying to fix file permissions of these files.
//
// Revision 0.2  1999/05/20 16:37:58  dev
// "Backup"
//
// Revision 0.1  1999/01/27 10:54:30  dev
// initial revision
//
//
//
