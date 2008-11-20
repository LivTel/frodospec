// NewmarkNativeException.java
// $Header: /home/cjm/cvs/frodospec/java/ngat/frodospec/newmark/NewmarkNativeException.java,v 1.1 2008-11-20 11:34:35 cjm Exp $
package ngat.frodospec.newmark;

/**
 * This class extends Exception. Objects of this class are thrown when the underlying C code in Newmark produces an
 * error. The JNI interface itself can also generate these exceptions.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
public class NewmarkNativeException extends Exception
{
	/**
	 * Revision Control System id string, showing the version of the Class
	 */
	public final static String RCSID = new String("$Id: NewmarkNativeException.java,v 1.1 2008-11-20 11:34:35 cjm Exp $");

	/**
	 * Constructor for the exception.
	 * @param errorString The error string.
	 */
	public NewmarkNativeException(String errorString)
	{
		super(errorString);
	}

	/**
	 * Constructor for the exception. Used from C JNI interface.
	 * @param errorString The error string.
	 * @param libfrodospec_newmark The Newmark instance that caused this excecption.
	 */
	public NewmarkNativeException(String errorString,Newmark libfrodospec_newmark)
	{
		super(errorString);
	}
}

//
// $Log: not supported by cvs2svn $
//
