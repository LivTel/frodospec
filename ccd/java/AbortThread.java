// AbortThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/AbortThread.java,v 0.1 1999-01-27 10:54:30 dev Exp $
import java.io.*;

class AbortThread extends Thread
{
	public final static String RCSID = new String("$Id: AbortThread.java,v 0.1 1999-01-27 10:54:30 dev Exp $");
	private Test parent = null;

	public AbortThread(Test test)
	{
		this.parent = test;
	}

	public void run()
	{
		int retval;

		try
		{
			while(System.in.available() == 0)
				Thread.yield();
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
}

//
// $Log: not supported by cvs2svn $
//
//
