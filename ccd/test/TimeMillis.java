
import java.lang.*;
import java.io.*;

public class TimeMillis
{

    public static void main(String args[])
    {
	long millis;

	millis = System.currentTimeMillis();
	System.out.println("Current time:milliseconds:"+millis);
	System.out.println("Max long:"+Long.MAX_VALUE);
	System.exit(0);
    }

};
