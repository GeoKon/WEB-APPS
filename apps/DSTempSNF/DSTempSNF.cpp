/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#include <ESP8266HTTPClient.h>
#include "DSTempSNF.h"				// baseline for any web-server application

#define WIFI_TIMEOUT_SEC 1

// ---------------- Local variable/class allocation ---------------------

    Buf cliresp(1000);                      // allocate buffer for CLI response
    ModuloTic mydelay( WIFI_TIMEOUT_SEC);   // waiting tic for WiFi Connection

// ------------- main Callbacks (add your changes here) -----------------

void staCallbacks()
{
    server.on("/tstat",
    [](){
        showArgs();
        Buf json(100);
        json.add("{'temp':%.1f, 'temp2':%.1f, 'mode':%d, 'relay':%d, 'thres':%.1f}", 
            myi.tempF[0], myi.tempF[1], myi.tmode, myi.relayON, myi.threshold*9.0/5.0+32.0);
        json.quotes();
        showJson( json.c_str() );
        server.send(200, "application/json", json.c_str() );
    });
    
    server.on("/",
    [](){
        server.send(200, "text/html",
        "<h1 align=\"center\">STA Services<br/><a href=\"index.htm\">Click for INDEX</a></h1>"
        );
    });

//    server.on("/currentsetting.htm", HTTP_GET, 
//    [](){
//        IPAddress ip = WiFi.localIP();
//        char r[200];
//        sprintf( r, "Model=GeorgeESP\r\nName=%d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3] );
//        server.send( 200, "text/html", r );
//        PF("Responded to /currentsetting %s\r\n", r );
//    });
    
    
    server.on("/webcli.htm", HTTP_GET,              // when command is submitted, webcli.htm is called  
    [](){
        showArgs();
        if( server.args() )                             // if command given
        {
            cliresp.init();                             // initialize the response buffer
            cliresp.set("(Max:%d bytes)\r\n", cliresp.maxsiz );
            exe.dispatch( !server.arg(0), &cliresp );   // command is executed here and response is saved in 'cliresp' buffer
            cliresp.add("(Used:%d bytes)\r\n", cliresp.length());
        }        
        showJson( !cliresp );                           // show the same on the console
        handleFileRead("/webcli.htm" );                 // reprint same html
    });
    server.on("/clirsp", HTTP_GET,      
    [](){
        showArgs();
        showJson( !cliresp );
        server.send(200, "text/html", !cliresp );
    });
    server.on("/cli", HTTP_GET,              // when command is submitted, webcli.htm is called  
    [](){
        showArgs();
        if( server.args() )                             // if command given
        {
            cliresp.init();                             // initialize the response buffer
            exe.dispatch( !server.arg(0), &cliresp );   // command is executed here and response is saved in 'cliresp' buffer
        }        
        showJson( !cliresp );                           // show the same on the console
        //handleFileRead("/webcli.htm" );                 // reprint same html
    });
}

// --------------- DO NOT MODIFY THE CODE BELOW --------------------------------

void interactForever()
{
    PR("");
    PR("CLI Mode. Press 'h' for help\r\n");
    cpu.led( ON );
    
    cli.init( ECHO_ON, "cmd: " );
    
    cli.prompt();
    for(;;)
    {
        if( cli.ready() )
        {
            char *cmd = cli.gets();
            //PF("Command %s\r\n", cmd );
            exe.dispatch( cmd );
            cli.prompt();
        }
    }
}

// Call repeately. Blinks LED every 'ms'
void blinkLED( int ms, uint32_t dly )
{
    static uint32_t T0=0;
    uint32_t msL = ms;
    static bool toggle = false;
    
    if( millis()-T0 > msL )     // most likely true the first time
    {
        toggle = !toggle;
        cpu.led( (onoff_t) toggle );
        T0 = millis();          
    }
    if( dly==0 )
        dly = msL/10;
    delay( dly );
}
void setupSTA()
{

    PN("Starting STA mode...\r\nMAC=");
    PR( WiFi.macAddress() );                   // mac is used later to define mDNS
    
    WiFi.mode(WIFI_STA);
    
    char *staticIP = eep.wifi.stIP;
    if( *staticIP )
    {
        IPAddress ip, gateway(192,168,0,1), subnet( 255,255,255,0 );
        ip.fromString( staticIP );
        WiFi.config( ip, gateway, subnet );
    }
    WiFi.begin( eep.wifi.ssid, eep.wifi.pwd );

    while( WiFi.status() != WL_CONNECTED  )         // fast blink while waiting for WiFi connection
        blinkLED( 200 );

    PN("IP=");PR( WiFi.localIP() );
    
    byte mac[6];
    WiFi.macAddress( mac );
    Buf name;
    name.set("GKE-%02x-%02x", mac[0], mac[5] );     // MSB and LSB of mac address
    if (!MDNS.begin( !name )) 
        cpu.die("Error setting up MDNS responder!", 3 );
    PF("mDNS advertising %s.local:%d", !name, eep.wifi.port ); 
        
    setTrace( T_REQUEST | T_JSON );  
    srvCallbacks();
    staCallbacks();
 
    server.begin( eep.wifi.port );
    PR("HTTP server started. Hit RETURN to activate CLI during normal operation\r\n");
}
