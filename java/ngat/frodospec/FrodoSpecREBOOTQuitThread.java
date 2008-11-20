// FrodoSpecREBOOTQuitThread.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/FrodoSpecREBOOTQuitThread.java,v 1.1 2008-11-20 11:33:35 cjm Exp $
package ngat.frodospec;

/**
 * This class is a thread that is started when the FrodoSpec is to terminate.
 * A thread is passed in, which must terminate before System.exit is called.
 * This is used in, for instance, the REBOOTImplementation, so that the 
 * REBOOT's DONE mesage is returned to the client before the FrodoSpec is terminated.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class FrodoSpecREBOOTQuitThread extends Thread
{
	/**
	 * Revision Control System id string, showing the version of the Class.
	 */
	public final static String RCSID = new String("$Id: FrodoSpecREBOOTQuitThread.java,v 1.1 2008-11-20 11:33:35 cjm Exp $");
	/**
	 * The Thread, that has to terminatre before this thread calls System.exit
	 */
	private Thread waitThread = null;
	/**
	 * Field holding the instance of the frodospec currently executing, used to access error handling routines etc.
	 */
	private FrodoSpec frodospec = null;

	/**
	 * The constructor.
	 * @param name The name of the thread.
	 */
	public FrodoSpecREBOOTQuitThread(String name)
	{
		super(name);
	}

	/**
	 * Routine to set this objects pointer to the frodospec object.
	 * @param c The frodospec object.
	 */
	public void setFrodoSpec(FrodoSpec c)
	{
		this.frodospec = c;
	}

	/**
	 * Method to set a thread, such that this thread will not call System.exit until
	 * that thread has terminated.
	 * @param t The thread to wait for.
	 * @see #waitThread
	 */
	public void setWaitThread(Thread t)
	{
		waitThread = t;
	}

	/**
	 * Run method, called when the thread is started.
	 * If the waitThread is non-null, we try to wait until it has terminated.
	 * System.exit(0) is then called.
	 * @see #waitThread
	 */
	public void run()
	{
		if(waitThread != null)
		{
			try
			{
				waitThread.join();
			}
			catch (InterruptedException e)
			{
				frodospec.error(this.getClass().getName()+":run:",e);
			}
		}
		System.exit(0);
	}
}
//
// $Log: not supported by cvs2svn $
//
