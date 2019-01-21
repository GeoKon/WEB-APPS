Using the SimpleSRV Module
--------------------------

Include `srvCallBacks()` in the main/setup() to register the callback handlers
of the endpoints. You must instantiate the classes

	ESP8266WebServer server;
	CPU cpu;				// NodeMCU core functions
	EEP eep;				//basic WiFi eeprom parameters
	
This file also includes a couple of utility/tracing functions such as

    void setTrace( int value );
    int getTrace();

	void showArgs( );
    void showJson( String json );
    
    String fileList( bool br = false );
	bool handleFileRead(String path);
	
For trace, use one of the following constants

    T_FILEIO  1		// to trace fileio 
    T_REQUEST 2		// to monitor web server requests
    T_JSON    4		// to display web responses
    T_ARGS    8		// to display request arguments passed 
    T_ACTIONS 16	   // to trace special functions

This module is to be tested by `ExampleWEB.ino`

SimpleSRV Endpoints
-------------------
 
** Inline endpoints **

| Endpoint           | Function      
| ----------         | -----             
|`/restart, /reset`  | Restarts the processor  
|`/status`           | Displays in json format SSID, and PWD
|`/trace`		     | Displays in json format the serial port trace
|`/set?trace={value}`| Sets trace to specific value REQUEST, ARGS, FILEIO, JSON, or ACTIONS 
|`/dir`              | Displays (in HTML)  directory of all files
|`/eep`				 | Displays HTML the WiFi EEPROM parameters
|`/ed?delete={fname}` | Deletes specified file
|`/ed?new={fname}`    | Creates a new file
|`/upload`            | Uploads a file (overides previous one)
|`/favicon`			 | Your favorite icon

** Endpoints requiring .htm files **

| Endpoint           | Function      
| ----------         | -----             
|`/edit`			 | Invokes an interactive editor of all files in the file system
|`/setAP.htm`		 | Sets STA WiFi credentials
|`/webcli.htm`		 | Invokes the Web CLI 
|`/index.htm`		 | Application specific screen with pointers to other pages

***Please read the conventions followed for these library and applications located in GKESP-L1/READ_CONVENTIONS.md***


