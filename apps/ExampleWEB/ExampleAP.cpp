#include "ExampleWEB.h"

// ----------------------- Exported functions -------------------------------------

void setupAP()
{
    PR("\r\nAP mode");
    cpu.led( ON );
    bool ok = WiFi.softAP("GKE_AP");
    if( !ok )
        cpu.die( "Cannot start AP" );
    
    PN( "ACCESS POINT MODE. Connect to GKE_AP, browse to: "); PR( WiFi.softAPIP());
    // PN( fileList() );
    
    setTrace( T_REQUEST | T_JSON );    
    srvCallbacks();
     apCallbacks();
    
    server.begin( 80 );
    cpu.led( ON );                  // ON indicates AP mode
    PR("HTTP server started");
}
void loopAP()
{
    static int connected = -1;
    
    int numbconx = WiFi.softAPgetStationNum();
    if( connected != numbconx )
    {
        PF("%d connections\r\n", connected = numbconx );
        cpu.led( OFF );
        delay( 300 );
        cpu.led( ON );
    }
    server.handleClient();
}

void apCallbacks()
{
    server.on("/",
    [](){
        server.send(200, "text/html","<head><title>AP Setup</title><style>body {text-align: center;}</style></head>"
                "<body><h1>AP Services</h1>"
                "<h2><a href=\"setap.htm\">Set WiFi Credentials</a></h2>"
                "<h2><a href=\"dir\">List of files (/dir)</a></h2>"
                "<h2><a href=\"upload\">Upload .htm file (/upload)</a></h2>"
                "<h2><a href=\"reset\">Reboot (/reset)</h2>"
                "</body>");
    });
    
    server.on("/setap.htm", HTTP_GET,       // Interactive End Point: /setap?ssid=xx&pwd=yy
    [](){
        String path = server.uri();
        if( server.args() > 1 )
        {
            String Sssid = server.arg(0);
            String Spwd  = server.arg(1);
            PF("Entered %s and %s\r\n", !Sssid, !Spwd );
            strcpy( eep.wifi.ssid, !Sssid );
            strcpy(  eep.wifi.pwd, !Spwd ); 
        }
        if( !handleFileRead( server.uri()) )
            server.send(404, "text/plain", "File /setap.htm");   
    }); 
}
