// ExposureThread.java -*- mode: Fundamental;-*-
// $Header: /home/cjm/cvs/frodospec/ccd/java/ExposureThread.java,v 0.1 1999-01-22 09:55:51 dev Exp $
class ExposureThread extends Thread
{
	public final static String RCSID = new String("$Id: ExposureThread.java,v 0.1 1999-01-22 09:55:51 dev Exp $");
	private CCDLibrary libccd = null;
	private boolean open_shutter;
	private boolean readout_ccd;
	private int ncols;
	private int nrows;
	private int msecs;
	private int algorithm;
	private byte[] data = null;

	public ExposureThread(CCDLibrary libccd,boolean open_shutter,boolean readout_ccd,int ncols,int nrows,
		int msecs,int algorithm)
	{
		this.libccd = libccd;
		this.open_shutter = open_shutter;
		this.readout_ccd = readout_ccd;
		this.ncols = ncols;
		this.nrows = nrows;
		this.msecs = msecs;
		this.algorithm = algorithm;
	}

	public void run()
	{
		data = libccd.CCDExposureExpose(open_shutter,readout_ccd,ncols,nrows,msecs,algorithm);
	}

	public void abort()
	{
//diddly
//		if(exposing)
		libccd.CCDExposureAbort();
//		if(reading_out)
//		libccd.CCDExposureAbortReadout();
		data = null;
	}

	public byte[] getData()
	{
		return data;
	}
}

//
// $Log: not supported by cvs2svn $
//
