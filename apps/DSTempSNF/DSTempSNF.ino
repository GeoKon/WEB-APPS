/* ----------------------------------------------------------------------------------
 *  Converts the SONOFF Basic switch V2 to a thermostat
 *  Presents webpage with 
 *        -- Temperature readings
 *        -- Thermostat controls
 *        -- Simulations
 *  Use WEB CLI to modify parameters.
 *  Compatible with the REST API of Radio Thermostat CT-50 and CT-80
 *   
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 *  ---------------------------------------------------------------------------------
 */
#include "DSTempSNF.h"     // necessary includes for this application
#include <timeClass.h>

#define CLI_WAITSEC 10      // how long to wait before RETURN is sensed for CLI
    
//------------------ References and Class Allocations ------------------------

#define NodeMCU 1           // <----------- select hardware ------------------

#ifdef SONOFF
//Inputs
    #define BUTTON       0
    #define DS_PIN       5 /* Sonoff */  // 02, 14 cannot be uesd

    #define GPIO00  0
    #define GPIO02  2       // used in Rev 2 of the Basic instead of GPIO14
    #define GPIO04  4
    #define GPIO05  5
    #define GPIO14 14       // 5th pin on the header
    
// Outputs
    #define LED         13
    #define RELAY       12
#endif

#ifdef NodeMCU
//Inputs
    #define BUTTON       0
    #define DS_PIN       4 /* this is D2 */

// Outputs
    #define LED         16
    #define RELAY       5
#endif



//------------------ References and Class Allocations ------------------------

    ESP8266WebServer server(80);
    CPU cpu;
    CLI cli;
    EXE exe;
    EEP eep;
    OneWire  ds( DS_PIN );      // on pin D3 of the NodeMCU
    DS18 temp( &ds );    // associate DS18B20 class with OneWire
    ModuloTic tic(2);    // one tic per second
    
//--------- Class with the global variables and initialization --------------
    
    Globals myi;

    Globals::Globals()
    {
       wifiOK = TRYING_WIFI;
       trace = false;
       tempC[0]=tempC[1]=tempF[0]=tempF[1]=-127.0;
       threshold = 0.0;
       delta     = 0.9;
       relayON   = false;
       simulON   = false;
       tmode  = NO_RELAY;
    }

    char *help = "Dallas Temperature reader\r\n";           
    
    namespace myiTable               // forward reference
    {
        extern void init( EXE &myexe );
        extern CMDTABLE table[]; 
    }
// ----------------------------- Main Setup -----------------------------------
void setup() 
{
    cpu.init( 115200, LED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );

    pinMode(RELAY, OUTPUT); digitalWrite( RELAY, LOW ); // positive logic
//    pinMode(GPIO14, INPUT);
//    pinMode(GPIO02, INPUT);
//    pinMode(GPIO04, INPUT);
//    pinMode(GPIO05, INPUT);
    for(;0;)                            // TEST of the IO pins
    {
        PF("14=%s 05=%s 04=%s 02=%s\r\n",
        digitalRead( 14 )?"ON":"OFF",
        digitalRead( 5 )?"ON":"OFF",
        digitalRead( 4 )?"ON":"OFF",
        digitalRead( 2 )?"ON":"OFF" );
        delay(400 );
    }
    if( !SPIFFS.begin() )
        cpu.die("No SPIFFS");
    
    // ----- REGISTER CLI HANDLERS -------//   
    myiTable::init( exe );                      // set RESONSE pointer to my CLI functions 
    exe.registerTable( myiTable::table );       // register my CLI table

    // ---- INITIALIZE EEPROM PARMS -----//   
    eepTable::init( exe, eep );
    exe.registerTable( eepTable::table ); 
    if( eep.fetchParms() )                      // fetch wifi parms; if a problem, initialize them
    {
        eep.initWiFiParms();                    // initialize with default WiFis
        eep.saveParms();    
    }
    eep.updateRebootCount();
    eep.printParms("Current Parms");            // print current parms

    // --- INTERACT IF RETURN PRESSED ---//
    PF("Press RETURN to start CLI within %d sec\r\n", CLI_WAITSEC );
    for( int i=0; i<CLI_WAITSEC*100; i++ )
    {
        blinkLED( 100 );                        // fast blink to indicate waiting for RETURN
        if( Serial.read() == 0x0D )
            interactForever();
    }
    // --- START WIFI IN STA MODE ------//
    setupSTA();                                 // Uses EEPROM parms. Flashes quickly while trying
    
    temp.init( NUM_TEMPS, true );               // initialize the Dallas Thermomete
    cli.init( ECHO_ON, "cmd: " );               // initialize CLI, but do not print prompt
}

void loop()
{
    if( WiFi.status() != WL_CONNECTED  )
        blinkLED( 200 );
    else
        server.handleClient(); 
        
    if( cli.ready() )               // handle serial interactions
    {
        exe.dispatch( cli.gets() );
        cli.prompt();
    }
    if( tic.ready() )
    {
        temp.start( temp.thisID() );   // specify index or use thisID()
        cpu.led( ON );
    }
    if( temp.ready() )
    {
        cpu.led( OFF );
        temp.read();                                        // read this temperature

        if( !myi.simulON )
        {
            myi.tempC[ temp.thisID() ] = temp.getDegreesC();     // save in local variables
            myi.tempF[ temp.thisID() ] = temp.getDegreesF();     // save in local variables
        }
        void relayControl( float T );                       // forward reference
        if( temp.thisID() == 0 )                            // use 1st sensor to control thermostat
            relayControl( myi.tempC[0] );
            
        //filter.smooth( 0, temp.getDegreesC() );
        if( myi.trace )
            PF("Relay:%d, (Temp0, Temp1 °C), %5.1f, %5.1f, (Temp0, Temp1 °F), %5.1f, %5.1f\r\n", 
                myi.relayON, myi.tempC[0], myi.tempC[1], myi.tempF[0], myi.tempF[1] );

        temp.nextID();
    }            
}

void relayControl( float T )
{
    switch (myi.tmode )
    {
        case NO_RELAY:
            digitalWrite( RELAY, LOW );
            myi.relayON = false;
            break;

        case COOL_MODE:
            if( T >= myi.threshold )
                digitalWrite( RELAY, myi.relayON = true);
            if( T < myi.threshold-myi.delta )
                digitalWrite( RELAY, myi.relayON = false);
            break;

        case HEAT_MODE:
            if( T <= myi.threshold )
                digitalWrite( RELAY, myi.relayON = true );
            if( T > myi.threshold+myi.delta )
                digitalWrite( RELAY, myi.relayON = false);
            break;
    }
}
    
namespace myiTable              
{
    static EXE *exe;
    void init( EXE &myexe ){exe = &myexe;}                  // needs to be called by main() to associate the EXE class with this table
    void help( int n, char **arg ){exe->help( n, arg );}    // what to do when "h" is entered

    #define MYRESP exe->respond     // Serial.printf

    void files( int n, char **arg )                         // displays list of files  
    {
        MYRESP( "%s", fileList().c_str() );
    }
    void cliTrace( int n, char **arg )                         // displays list of files  
    {
        if( n>1 )  myi.trace = atoi( arg[1] );
        else       myi.trace = false;
        MYRESP( "Trace=%d\r\n", myi.trace );
    }
    void cliSimulC( int n, char **arg )                         // displays list of files  
    {
        if( n>1 ) 
        {
            float value = atof( arg[1] );
            myi.tempC[0]= value;            
            myi.tempF[0]= value*1.8 + 32.0;            
            temp.simulate( 0, value );                          // turn on simulation of 1st temp
            
            MYRESP( "SimON[0] %.1f (%.1f)\r\n", myi.tempC[0], myi.tempF[0] );
        }
        else
        {
            temp.simulOff( 0 );
            MYRESP( "SimOFF[0]\r\n" );
        }
    }
    void cliSimulF( int n, char **arg )                         // displays list of files  
    {
        if( n>1 ) 
        {
            float value = atof( arg[1] );
            myi.tempF[0]=  value;            
            myi.tempC[0]= (value - 32.0)*5.0/9.0;
            temp.simulate( 0, value );                     // turn on all simulations
            
            MYRESP( "SimON[0] %.1f (%.1f)\r\n", myi.tempC[0], myi.tempF[0] );
        }
        else
        {
            temp.simulOff( 0 );
            MYRESP( "SimOFF[0]\r\n" );
        }
    }
    void cliHeatC( int n, char **arg )                         // displays list of files  
    {
        if( n>2 )
            myi.delta = atof( arg[2] );
        if( n>1 )
            myi.threshold = atof( arg[1] );
        myi.tmode = HEAT_MODE;
        if( n<=1 )
        {
            myi.tmode = NO_RELAY;
            MYRESP( "No Relay\r\n" );
        }
        else
            MYRESP( "Heat at %.1f, delta=%.1f\r\n", myi.threshold, myi.delta );
    }
    void cliCoolC( int n, char **arg )                         // displays list of files  
    {
        if( n>2 )
            myi.delta = atof( arg[2] );
        if( n>1 )
            myi.threshold = atof( arg[1] );
        myi.tmode = COOL_MODE;
        if( n<=1 )
        {
            myi.tmode = NO_RELAY;
            MYRESP( "No Relay\r\n" );
        }
        else
            MYRESP( "Cool at %.1f, delta=%.1f\r\n", myi.threshold, myi.delta );
    }
    void cliInit( int n, char **arg )
    {
        cpu.led( BLINK, 2 );
        temp.init( NUM_TEMPS, true );       // initialize the Dallas Thermomete
        cpu.led( BLINK, 2 );
    }
    CMDTABLE table[]= // must be external to be able to used by the cliSupport
    {
        {"h",       "Help! List of all commands",                       help     },
        //{"dir",     "SPIFS List of files",                              files    },
        {"init",    "Initialize DS Sensors",                            cliInit      },
        {"simulC",  "[value] Simulate degC value of 0th temp",          cliSimulC    },
        {"simulF",  "[value] Simulate degF value of 0th temp",          cliSimulF    },
        {"trace",   "value. Serial port trace",                         cliTrace    },
        {"heatC",    "[value] [delta]. Turn heater on",        cliHeatC    },
        {"coolC",    "[value] [delta]. Turn cooler on",        cliCoolC    },
        {NULL, NULL, NULL}
    };
}
