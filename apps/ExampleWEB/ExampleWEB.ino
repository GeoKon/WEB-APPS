/* ---------------------------------------------------------------------------
 *  Example of a simple WEB application. Should be used as a template
 *  to create other applications.
 *  
 *  Use/Modify the ExampleSTA.cpp to include the specific STA callbacks
 *  Use/Modify the ExampleAP.cpp to include AP mode functionality
 *  The above should include SimpleSRV.cpp with the baseline WEB functions
 *  --------------------------------------------------------------------------
 */
#include "ExampleWEB.h"     // necessary includes for this application

#define CLI_WAITSEC 10      // how long to wait before RETURN is sensed for CLI
    
//------------------ References and Class Allocations ------------------------

    ESP8266WebServer server(80);
    CPU cpu;
    CLI cli;
    EXE exe;
    EEP eep;
    
//--------- Class with the global variables and initialization --------------
    
    enum wifi_state { TRYING_WIFI=0, STA_MODE=2, AP_MODE=1 };
    
    class Globals
    {
      public:
        wifi_state wifiOK; 
        Globals(){ wifiOK = TRYING_WIFI; }
    } myi;

    char *help = 
    "Generic version of AP and STA web server"
    "\r\n";           
    
    namespace myiTable               // forward reference
    {
        extern void init( EXE &myexe );
        extern CMDTABLE table[]; 
    }
// ----------------------------- Main Setup -----------------------------------
void setup() 
{
    cpu.init(); 
    if( !SPIFFS.begin() )
        cpu.die("No SPIFFS");
    
    myiTable::init( exe );                      // register CLI tables
    eepTable::init( exe, eep );             
    exe.registerTable( myiTable::table );
    exe.registerTable( eepTable::table ); 

    if( eep.fetchParms() )              // fetch wifi parms; if a problem, initialize them
    {
        eep.initWiFiParms();            // initialize with default WiFis
        eep.saveParms();    
    }
    eep.updateRebootCount();
    eep.printParms("Current Parms");    // print current parms
    
    PF( "%s----- Within %dsec: press CR for CLI, or push BUTTON for AP Setup (192.168.4.1)\r\n", help, CLI_WAITSEC );

    myi.wifiOK = STA_MODE;
    
    for( int i=0; i<CLI_WAITSEC*100; i++ )
    {
        if( (i%100)==0 ) PN(".");
        if( Serial.read() == 0x0D )
            interactForever();
        if( cpu.button() )
            {myi.wifiOK=AP_MODE; break;}
        delay(10);
    }
    if( myi.wifiOK == STA_MODE )
        setupSTA();
    if( myi.wifiOK == AP_MODE )
        setupAP();
}
void loop()
{
    if( myi.wifiOK==AP_MODE )
        loopAP();
    if( myi.wifiOK==STA_MODE )
        loopSTA();
}
namespace myiTable              
{
    static EXE *exe;
    void init( EXE &myexe ){exe = &myexe;}                  // needs to be called by main() to associate the EXE class with this table
    void help( int n, char **arg ){exe->help( n, arg );}    // what to do when "h" is entered

    void files( int n, char **arg )                         // displays list of files  
    {
        exe->respond( fileList().c_str() );
    }
    CMDTABLE table[]= // must be external to be able to used by the cliSupport
    {
        {"h",       "Help! List of all commands",             help     },
        {"dir",     "SPIFS List of files",                    files    },
        {NULL, NULL, NULL}
    };
}
