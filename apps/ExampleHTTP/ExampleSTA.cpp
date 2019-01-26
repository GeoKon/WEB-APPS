#include <ESP8266HTTPClient.h>
#include "ExampleWEB.h"				// baseline for any web-server application

#define WIFI_TIMEOUT_SEC 5

Buf cliresp(1000);               // allocate buffer for CLI response
HTTPClient http;                //Object of class HTTPClient

void setupSTA()
{
    PR("\r\nSTA mode");
    WiFi.mode(WIFI_STA);

    char *staticIP = eep.wifi.stIP;
    if( *staticIP )
    {
        IPAddress ip, gateway(192,168,0,1), subnet( 255,255,255,0 );
        ip.fromString( staticIP );
        WiFi.config( ip, gateway, subnet );
    }
    WiFi.begin( eep.wifi.ssid, eep.wifi.pwd );
    
    for( int i=0; (i<WIFI_TIMEOUT_SEC*2)||(WIFI_TIMEOUT_SEC==0); i++  )
    {
        if( WiFi.status() == WL_CONNECTED )
        {
            PF( "\r\nConnected to %s. Hostname is %s. ", WiFi.SSID().c_str(), WiFi.hostname().c_str() );
            PN( "IP Address " ); PR( WiFi.localIP() );
            //PN( fileList() );
            break;
        }
        delay(250);
        if( i%2 ) cpu.led( ON );
        else cpu.led( OFF );
    }
    
    Buf name(16);
    name.set("GKELAB%d", eep.head.reboots );
    if (!MDNS.begin( !name )) 
        cpu.die("Error setting up MDNS responder!", 3 );
    MDNS.addService("GKETCP", "tcp", eep.wifi.port );
    MDNS.addService("GKEUDP", "udp", 5353);
    PF("mDNS responder started. Ping for %s.local:%d\r\n", !name, eep.wifi.port); 
    
    setTrace( T_REQUEST | T_JSON );  
    srvCallbacks();
    staCallbacks();
 
    server.begin( 80 );
    PR("HTTP server started. Hit RETURN to activate CLI during normal operation\r\n");

    cli.init( ECHO_ON, "cmd: " );
    cpu.led( BLINK, 2 );            // indicates successful WiFi connection 
}
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
static bool pending = false;
/* example of scanf decoding
 *  #include <stdio.h>
char *s = "{\"state\":\"CHARG\", \"volts\":13.487}";
char p1[100];
char p2[100];
char p3[100];
float x;
#define LABEL "\"%[^\"]\""
#define COLON ": "
#define COMMA ", "
#define FLOAT " %f"
#define LEFTBR "{ "
int main()
{
    printf("Hello World %s" LABEL "\r\n", "test" );
    sscanf( s, LEFTBR LABEL COLON LABEL COMMA LABEL COLON FLOAT, p1, p2, p3, &x);
    printf("decoded %s=%s, %s=%f\r\n", p1, p2, p3, x);
    return 0;
}
*/
void loopSTA()
{
    if( cli.ready() )               // handle serial interactions
    {
        //exe.dispatch( cli.gets() );
        char *p = cli.gets();
        http.begin( p );    // "http://192.168.0.70:8067/status");
        pending = true;
    }
    if( pending )
    {
        int httpCode = http.GET();
        if (httpCode > 0) 
        {
            String buffer;
            buffer = http.getString();
            PF("Received: %s\r\n", !buffer );
            pending = false;
            http.end(); //Close connection
            cli.prompt();
        }
    }
    server.handleClient();    
}

void staCallbacks()
{
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
}
