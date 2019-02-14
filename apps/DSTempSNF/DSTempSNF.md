Temperature Class DS18
----------------------
This is a total re-write of the DallasTemperature Class (by Adafruit and others), just using the OneWire Class and nothing else. It is very specific to DS18B20 and tested with the ESP8266 and ESP8265.

The class follows the design pattern of "ready-test" as described in the root of my Github. It can handle one or more sensors on the same wire. The use cases are shown below.

**Summary of functions**

**`ds18.search( bood debug )`**
> Searches for all sensors on the one-wire bus. Builds an array of addresses associated with the sensors. The number of sensors is returned by this. Also accessible by `ds18.found()`. The optional debug argument can be used to print diagnostic information relating to search.
> Pointers to the addresses of the respective sensors can be retrieved using the function:
> 
	DevAddr *ds18.getAddress( int id ) 

**`ds18.start( int id )`**
> Call this function in the `loop()` but no more frequently than 800ms. It basically starts temperature measurement of the `id`-th sensor. Such `id` must be smaller than `ds18.found()`. This function will do nothing if `id` is more or equal to `ds18.found`.

**`ds18.ready()`**
> This function returns `TRUE` after 750ms have been elapsed and the temperature conversion has been is completed. Because such conversion might result to an error, use the `ds18.success()' function to double check. If no errors the temperature reading can be retrieved using on of the following functions:
> 
	ds18.getDegreesC()
	ds18.getDegreesF()

> In case of a conversion error, these functions return the floating constant `INVALID_TEMP`

**Example of reading one sensor**  
The skeleton code looks like the following:
    
    void setup()
    {
    	....
    	ds18.search();
    	if( ds18.found < 1 )	// 1 or any other expected number
    		handle the error...
    }
    
    ModuloTic tic( 2 );			// trigger every 2-seocnds
    void loop()
    {
    	if( tic.ready() )
    		ds18.start( 0 );	// start reading of 1st sensor found
    	...
    	if( ds18.ready() && ds18.success() )		
    	{
 			print... ds18.getDegreesC()
    	}
    }

The `ds18.success()` function takes an optional argument of `bool debug`. If this is set to `true`, errors are printed.
 
Alternatively, instead of checking of `ds18.ready()` and `ds18.success()` in one if statement you can access the error code using:

    if( ds18.ready() )
    {
    	if( !ds18.success( true ) )
    		... specific error is ds18.error() ...
		else
			... no errors ....
    }    
    
For testing purposes, to simulate a specific temperature, use at any point the functions 

    ds18.simulC( float tempC );		// for degrees C
	ds18.simulF( float tempF );		// for degrees F
	ds18.simulOff();	
    
If this function is called after the `ds18.start()`, the `ds18.eady()` returns immediately with no errors and the simulated value in 'ds18.getDegreesC()`. If this function is called before `ds18.start()`, no specific access to sensors takes place.

The sensor may be disconnected (for some reason) and then reconnected. The `ds18.start()` and the respective `ds18.ready()` will behave properly and the temperature acquisition to restart after the same sensor is reconnected. Alternatively, you may want to use the following code which recovers with a new sensor:

    if( tic.ready() )
    	ds18.start( 0 );
    if( ds18.ready() )
    {
    	if( ds18.error == DS18ERR_DISCONNECTED )
    		ds18.search()
    	....
    }

**Sample of reading multiple sensors on the same bus**
The `ds18.search()` will set `ds18.found` to more than 1, and will return the number of sensors found. If you do care about which sensor to read, use the previous section with the appropriate `ds18.start( id )`. If you do not care about the order of sensors read, use the following code snippet:

    void loop()
    {
    	static int id = 0;					// must be static to start
    
    	if( tic.ready() )	
    		ds18.start( id );				// id takes values 0...ds18.found-1
    
    	if( ds18.ready() && ds18.success() )
    	{
    		... ds18.getDegreesC() ...
			id = ds18.nextID();				// increment id
    	}
    }

The last statement is equivalent to either one of the following:
	
	 
	if( ++id >= ds18.found() )				// equivalent to above
		id = 0;
	
	id = (++id) % ds18.found();				// equivalent to above