/* ----------------------------------------------------------------------------------
 *  Converts the SONOFF Basic switch V2 to a thermostat
 *  Presents webpage with 
 *        -- Temperature readings
 *        -- Thermostat controls
 *        -- Simulations
 *  Use WEB CLI to modify parameters.
 *  Compatible with the REST API of Radio Thermostat CT-50 and CT-80
 *   
 *  Boot sequence:
 *      1. fast blink 100/100 waiting for SERIAL connection
 *      2. slower blink 5sec waiting for WiFi connection
 *      3. for
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 *  ---------------------------------------------------------------------------------
 */

  #define DS18B20           // either DS18B22 or DHT22
//#define DHT22
//#define NODEMCU           // either NODEMCU or SONOFF
  #define SONOFF
 
#include "DSTempSNF.h"      // necessary includes for this application
#include "SimpleDHT.h"
#include <timeClass.h>

#define CLI_WAITSEC 10      // how long to wait before RETURN is sensed for CLI
    
//------------------ References and Class Allocations ------------------------
/* GPIO2 (D4 on MCU), it must be HIGH to boot processor.
 * It is while FLASHING code, i.e. peripheral might get confused.
 * Ut use, set it first OUTOUT HIGH; after few ms, set is an input.
 */
#ifdef SONOFF
//Inputs
    #define BUTTON       0
    #define DS_PIN       14     // Do not use 02 (must e high during RESET)

    #define GPIO00  0
    #define GPIO02  2           // used in Rev 2 of the Basic instead of GPIO14
    #define GPIO04  4
    #define GPIO05  5
    #define GPIO14 14           // 5th pin on the header of Rev 1
    
// Outputs
    #define LED         13
    #define RELAY       12
#endif

#ifdef NODEMCU
//Inputs
    #define BUTTON       0
    #define DS_PIN       4      // this is D2, aka SDA

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
    FILT filter;                // define smoothing filter
    
#ifdef DS18B20
    ModuloTic tic(2);           // one measurement per 2-seconds
    OneWire  ds( DS_PIN ); 
    DS18 temp( &ds );           // associate DS18B20 class with OneWire
#endif

#ifdef DHT22
    ModuloTic tic(4);           // one measurement per 4-seconds
    SimpleDHT22 dht22( DS_PIN );
#endif

//--------- Class with the global variables and initialization --------------

    Globals myi;

    Globals::Globals()
    {
       wifiOK = TRYING_WIFI;
       trace = false;
       tempC=tempC=tempF=tempF=INVALID_TEMP;
       humidity  = 0.0;
       relayON   = false;
       simulON   = false;
       initUserParms();
    }
    void Globals::initUserParms()
    {
       gp.tmode     = (thermode_t) NO_RELAY;
       gp.prdelay   = 0;
       gp.threshold = 0.0;
       gp.delta     = 0.9; 
    }
    char *gp_format = GP_FORMAT;
    
    char *help = "Temperature/humidity reader\r\n";           

    namespace myiTable               // forward reference
    {
        extern void init( EXE &myexe );
        extern CMDTABLE table[]; 
    }
// ----------------------------- Main Setup -----------------------------------
void setup() 
{
    pinMode( 2, OUTPUT);  // needed to boot from FLASH
    
    cpu.init( 115200, LED+NEGATIVE_LOGIC /* LED */, BUTTON+NEGATIVE_LOGIC /* push button */ );

    pinMode(RELAY, OUTPUT); digitalWrite( RELAY, LOW ); // positive logic

//    pinMode(14, INPUT);
//    pinMode(2, INPUT);
//    pinMode(4, INPUT);
//    pinMode(5, INPUT);
//    for(;;)                            // TEST of the IO pins
//    {
//        PF("GPIO14=%s GPIO5=%s GPIO4=%s GPIO2=%s\r\n",
//        digitalRead( 14 )?"ON":"OFF",
//        digitalRead( 5 )?"ON":"OFF",
//        digitalRead( 4 )?"ON":"OFF",
//        digitalRead( 2 )?"ON":"OFF" );
//        delay(400 );
//    }
    if( !SPIFFS.begin() )
        cpu.die("No SPIFFS");
    
    // ----- REGISTER CLI HANDLERS -------//   
    myiTable::init( exe );                      // set RESONSE pointer to my CLI functions 
    exe.registerTable( myiTable::table );       // register my CLI table

    // ---- INITIALIZE EEPROM PARMS -----//   
    eepTable::init( exe, eep );                 // initialize EEPROM base
    exe.registerTable( eepTable::table );       // register EEPROM commands

    eep.registerUserParms( &myi.gp, sizeof( myi.gp ), gp_format );
    eep.registerInitCallBk( [](){ myi.initUserParms(); } );
        
    if( eep.fetchParms() )                      // fetch wifi parms; if a problem, initialize them
    {
        eep.initWiFiParms();                    // initialize with default WiFis
        eep.initUserParms();
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

#ifdef DS18B20
    temp.search( );                             // initialize the Dallas Thermomete
    PF("Found %d sensors\r\n", temp.count() );
#endif
    
    filter.setPropDelay( myi.gp.prdelay );         // propagation delay 
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
#ifdef DS18B20
    if( tic.ready() )
    {
        cpu.led( ON );
        temp.start( 0 ); 
        cpu.led( OFF );
    }
    if( temp.ready() && temp.success(true) )
    {
        float C = temp.getDegreesC();       // save in local variables
        float F = C_TO_F( C );

        myi.tempC  = filter.smooth( C );
        myi.tempF = C_TO_F( myi.tempC );    // save in local variables

        if( myi.trace )
            PF("Relay:%d, Temp:%5.1f°C Smooth:%5.1f (%5.1f°F %5.1f°F)\r\n", 
                myi.relayON, C, myi.tempC, F, myi.tempF );

        void relayControl( float T );       // forward reference
        relayControl( C );
    }            
#endif
#ifdef DHT22
    if( tic.ready() )
    {
        cpu.led( ON );
        noInterrupts();
        int err = dht22.read2( &myi.tempC, &myi.humidity, NULL);
        interrupts();
        myi.tempF = myi.tempC*9.0/5.0 + 32.0;
        if( err != SimpleDHTErrSuccess )
            PF("Error %d\r\n", err );
        else
            PF("Temp %5.1f, humidity=%5.1f%%\r\n", myi.tempC, myi.humidity );
        cpu.led( OFF );
    }
#endif
}
// -------------------------- RELAY CONTROL --------------------------------------------    
void relayControl( float T )
{
    switch (myi.gp.tmode )
    {
        case NO_RELAY:
            digitalWrite( RELAY, LOW );
            myi.relayON = false;
            break;

        case COOL_MODE:
            if( T >= myi.gp.threshold )
                digitalWrite( RELAY, myi.relayON = true);
            if( T < myi.gp.threshold-myi.gp.delta )
                digitalWrite( RELAY, myi.relayON = false);
            break;

        case HEAT_MODE:
            if( T <= myi.gp.threshold )
                digitalWrite( RELAY, myi.relayON = true );
            if( T > myi.gp.threshold+myi.gp.delta )
                digitalWrite( RELAY, myi.relayON = false);
            break;
    }
}

// -------------------------- COMMAND LINE HANDLERS --------------------------------------------    
namespace myiTable              
{
    static EXE *exe;
    void init( EXE &myexe ){exe = &myexe;}                  // needs to be called by main() to associate the EXE class with this table
    void help( int n, char **arg ){exe->help( n, arg );}    // what to do when "h" is entered

    #define MYRESP Serial.printf //exe->respond     // 

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
#ifdef DS18B20 
    void cliSimulC( int n, char **arg )                         // displays list of files  
    {
        if( n>1 ) 
        {
            float value = atof( arg[1] );
            temp.simulC( value );                               // turn on simulation of 1st temp            
            MYRESP( "SimON %.1f°C\r\n", value );
        }
        else
        {
            temp.simulOff();
            MYRESP( "SimOFF\r\n" );
        }
    }
    void cliSimulF( int n, char **arg )                         // displays list of files  
    {
        if( n>1 ) 
        {
            float value = atof( arg[1] );
            temp.simulF( value );                               // turn on all simulations            
            MYRESP( "SimON %.1f°F\r\n", value );
        }
        else
        {
            temp.simulOff();
            MYRESP( "SimOFF\r\n" );
        }
    }
    void cliInit( int n, char **arg )
    {
        cpu.led( BLINK, 2 );
        temp.search( true );       // initialize the Dallas Thermomete

        MYRESP("Found %d sensors\r\n", temp.count() );
    }
    void cliFilter( int n, char **arg )
    {
        int pd = 0;
        if( n>=1 )
            pd = atoi( arg[1] );
        
        filter.setPropDelay( myi.gp.prdelay = pd );
        
        MYRESP( "Prop delay set to %d-samples\r\n", pd );
        eep.saveParms( USER_PARMS );
    }
    void cliHeatC( int n, char **arg )                         // displays list of files  
    {
        if( n>2 ) myi.gp.delta = atof( arg[2] );
        if( n>1 ) myi.gp.threshold = atof( arg[1] );
        myi.gp.tmode = HEAT_MODE;
        
        if( n<=1 )
        {
            myi.gp.tmode = NO_RELAY;
            MYRESP( "No Relay\r\n" );
        }
        else
            MYRESP( "Heat at %.1fC, delta=%.1fC\r\n", myi.gp.threshold, myi.gp.delta );
        eep.saveParms( USER_PARMS );
    }
    #define FTOC(A) ((A)-32.0)*5.0/9.0
    void cliHeatF( int n, char **arg )                         // displays list of files  
    {
        if( n>2 ) myi.gp.delta =  FTOC(atof( arg[2] ));
        if( n>1 ) myi.gp.threshold = FTOC(atof( arg[1] ));
        myi.gp.tmode = HEAT_MODE;
        
        if( n<=1 )
        {
            myi.gp.tmode = NO_RELAY;
            MYRESP( "No Relay\r\n" );
        }
        else
            MYRESP( "Heat at %.1fC, delta=%.1fC\r\n", myi.gp.threshold, myi.gp.delta );
        eep.saveParms( USER_PARMS );
    }
    
    void cliCoolC( int n, char **arg )                         // displays list of files  
    {
        if( n>2 )
            myi.gp.delta = atof( arg[2] );
        if( n>1 )
            myi.gp.threshold = atof( arg[1] );
        myi.gp.tmode = COOL_MODE;
        if( n<=1 )
        {
            myi.gp.tmode = NO_RELAY;
            MYRESP( "No Relay\r\n" );
        }
        else
            MYRESP( "Cool at %.1f, delta=%.1f\r\n", myi.gp.threshold, myi.gp.delta );
        eep.saveParms( USER_PARMS );
    }
#endif

    CMDTABLE table[]= // must be external to be able to used by the cliSupport
    {
        {"h",       "Help! List of all commands",                       help     },
        //{"dir",     "SPIFS List of files",                              files    },
   
#ifdef DS18B20
        {"init",    "Initialize DS Sensors",                            cliInit      },
        {"trace",   "value. Serial port trace",                         cliTrace    },
        {"filter",  "N. Define filter of N-samples",                    cliFilter    },
        
        {"simulC",  "[value] Simulate degC",                            cliSimulC    },
        {"simulF",  "[value] Simulate degF",                            cliSimulF    },

        {"targetC",  "[value] [delta]. Turn target heater degC on",     cliHeatC    },
        {"targetF",  "[value] [delta]. Turn target heater degF on",     cliHeatF    },

        {"coolC",    "[value] [delta]. Turn cooler on",        cliCoolC    },
#endif
        {NULL, NULL, NULL}
    };
}
