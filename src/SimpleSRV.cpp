#include "SimpleSRV.h"

// -------- Minimum includes for this module --------------------------------------

    #include <ESP8266WebServer.h>
    #include <FS.h>
    #include <cpuClass.h>
	#include <eepClass.h>

// ------ Expected definitions/allocations in main() ------------------------------

    extern ESP8266WebServer server;
    extern CPU cpu;					// NodeMCU class
    extern EEP eep;					// basic eeprom parameters 
	
// --------- Forward References needed by the editor ------------------------------

    void handleFileList();
    bool handleFileRead(String path);
    bool handleFileRead1(String path);
    void handleFileDelete();
    void handleFileCreate();
    void handleFileUpload1();                 
    void handleFileUpload(); 

// ----------------------- Server Callbacks() -------------------------------------

static int g_trace;  	// used only in this file. Elsewhere, use setTrace() and getTrace()
void srvCallbacks()
{
    server.on("/restart", HTTP_GET, 
    [](){
      showArgs();
      String resp("<h3 align='center'>RESTARTED! (control is lost)</h3>");
      server.send(200, "text/html", resp);
      delay( 1000 );
      ESP.restart();
    }); 
    server.on("/reset", HTTP_GET, 
    [](){
        showArgs();
        String resp( "<h3 align='center'>RESET! (control is lost)</h3>");
        server.send(200, "text/html", resp);
      delay( 1000 );
      ESP.reset();
    }); 
    server.on("/status", HTTP_GET, 
    [](){
      showArgs();
      String json("",100);
      json.set( "{" );
      json.add( "'ssid': '%s',", eep.wifi.ssid );
      json.add( " 'pwd': '%s'", eep.wifi.pwd );
      json.add( "}" );
      showJson( json );  
      server.send(200, "text/json", json);
    }); 
    server.on("/eep", HTTP_GET, 
    [](){
      showArgs();
      String resp("",100);
      resp.set( "<h3 align='center'>SSID:%s<br/>PWD:%s<br/>STIP:%s<br/>PORT:%d<br/>"
                     "<a href='/'>Click to navigate</a></h3>", 
                     eep.wifi.ssid,
                     eep.wifi.pwd,
                     eep.wifi.stIP,
                     eep.wifi.port ); 
      showJson( resp );  
      server.send(200, "text/html", resp);
    }); 
    server.on("/trace", HTTP_GET, 
    [](){
        showArgs();
        String json("",80);
        json.set("{'json':'%d', ",  g_trace & T_JSON    ? 1 : 0);
        json.add(  "'req':'%d', ",  g_trace & T_REQUEST ? 1 : 0);
        json.add( "'args':'%d', ",  g_trace & T_ARGS    ? 1 : 0);
        json.add("'fileio':'%d', ", g_trace & T_FILEIO  ? 1 : 0);
        json.add("'actions':'%d'}", g_trace & T_ACTIONS ? 1 : 0);
        showJson( json );  
        server.send(200, "text/json", json);
    }); 
    server.on("/set", HTTP_GET, 
    [](){
        showArgs();
        String resp("",80);
        if( server.args() <= 0 )
            resp.set( "<h3 align='center'>Please use /set?key=value<br/>"
                      "<a href='/'>Click to navigate</a></h3>" );     
        else
        {
            if( server.argName(0) == "trace" )
            {
                g_trace = atoi( server.arg(0).c_str() );
                resp.set( "<h3 align='center'>OK set to %d<br/>"
                          "<a href='/'>Click to navigate</a></h3>",  g_trace );     
            }
            else
            {
                resp.set( "<h3 align='center'>Unsupported key<br/>"
                          "<a href='/'>Click to navigate</a></h3>");     
            }
        }
        showJson( resp );  
        server.send(200, "text/html", resp);
    }); 
    server.on("/dir", HTTP_GET,
    [](){
        showArgs();
        String resp("",400);
        resp.set(   "<h3 align='center'>%s<br/>"
                    "<a href='/'>Click to navigate</a></h3>", fileList( true ).c_str() ); 
                          
        server.send(200, "text/html", resp );     
    });

    // --------------- editor files ------------------------
    server.on("/list", HTTP_GET, handleFileList );
    server.on("/edit", HTTP_GET,[](){if(!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");});
    server.on("/edit", HTTP_PUT, handleFileCreate);
    server.on("/edit", HTTP_DELETE, handleFileDelete);
    server.on("/edit", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);
    // -----------------------------------------------------

    server.on("/ed", HTTP_GET,          
    [](){
        showArgs();
        if(server.args() == 0) 
            return server.send(500, "text/plain", "BAD ARGS");
        String func = server.argName(0);  
        String path = "/"+server.arg(0);

        if( func=="del" )                   // use ed?del=filename
        {
            PR("handleFileDelete: " + path);
            if(!SPIFFS.exists( path ) )
                return server.send(404, "text/plain", "FileNotFound");
            SPIFFS.remove(path);
        }
        else if( func=="new" )              // use ed?new=filename
        {
            PR("handleFileCreate: " + path);
            if(SPIFFS.exists(path))
                return server.send(500, "text/plain", "FILE EXISTS");
            
            File file = SPIFFS.open(path, "w");
            if(file)
                file.close();
            else
                return server.send(500, "text/plain", "CREATE FAILED");
        }
        server.send(200, "text/plain", fileList() );     
    });

//    server.on("/upload", HTTP_GET, 
//    []() {                                                      // if the client requests the upload page
//        showArgs();
//        if (!handleFileRead1("/upload.htm"))                     // send it if it exists
//            server.send(404, "text/plain", "404: Not Found");   // otherwise, respond with a 404 (Not Found) error
//    });

    server.on("/upload", HTTP_GET, 
    []() {                                                      // if the client requests the upload page
        showArgs();
        char *upto = "<html><head><title>File Upload</title><style>body {text-align: center;}input {text-align:center;}</style>"
        "</head><body><form method=\"post\" enctype=\"multipart/form-data\"><h2>UPLOAD FILE<br/><br/><input type=\"file\" name=\"name\">"
        "<input class=\"button\" type=\"submit\" value=\"OK\"></h2></form><a href=\"index.htm\">Goto INDEX</a></body></html>";
        server.send(200, "text/html", upto);  
    });    
    server.on("/upload", HTTP_POST,                         // if the client posts to the upload page
        [](){ showArgs(); server.send(200); },                                                  // Send status 200 (OK) to tell the client we are ready to receive
        handleFileUpload);                                    // Receive and save the file

    server.on("/favicon.ico", HTTP_GET, 
    [](){
        showArgs();
        //PR( "request: "+ server.uri() );
        server.send(200, "image/x-icon", "");
    });
	
	server.on("/currentsetting.htm", HTTP_GET, 
    [](){
        IPAddress ip = WiFi.localIP();
        char r[200];
        sprintf( r, "Model=GeorgeESP\r\nName=%d.%d.%d.%d\r\n", ip[0],ip[1],ip[2],ip[3] );
        server.send( 200, "text/html", r );
        PF("Responded to /currentsetting %s\r\n", r );
    });
	
    server.onNotFound( 
    [](){
        showArgs();
        String path = server.uri() ;
        //PR( "request: "+ path );
        if(SPIFFS.exists(path))
        {
            File file = SPIFFS.open(path, "r");
            server.streamFile(file, "text/html");
            file.close();
        }
        else
            server.send(404, "text/plain", "Serves only .htm files");   
    });
}
// -------------------------------- handlers -----------------------------------------

void setTrace( int value ) {g_trace = value;}
int getTrace() {return g_trace;}

String fileList( bool br )
{
    Dir dir = SPIFFS.openDir("/");
    String output("",500);
    while (dir.next()) 
    {    
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        output.add( "%s (%s)", fileName.c_str(), formatBytes(fileSize).c_str() );
        if( br )
            output.add( "<br/>" );
        output.add("\r\n");
    }  
    return output;
}
// see: https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html

String getContentType(String filename) // convert the file extension to the MIME type
{ 
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".txt")) return "text/plain";
  else if (filename.endsWith(".pdf")) return "image/x-icon";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
void showArgs( )
{
  cpu.heapUpdate();
  if( g_trace & T_REQUEST )
    PR( "*** request: "+ server.uri() );
  if( g_trace & T_ARGS )
  {
    int N=server.args();
    PN("=== arg: "); PR( N );
    if( N>0 )
    {
      for( int i=0; i<N; i++ )
        PF( "Name:%s, Value:%s\r\n", !server.argName(i), !server.arg(i) );
    }
  }
}
void showJson( String &json )
{
  cpu.heapUpdate();
  if( g_trace & T_JSON )
  {
    PN( "--- " ); PR( json );
  }
}
void showJson( char *s )
{
  cpu.heapUpdate();
  if( g_trace & T_JSON )
    PF( "--- %s\r\n", s );
}

//format bytes
String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024))
  {
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024))
  {
    return String(bytes/1024.0/1024.0)+"MB";
  } else 
  {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}
// --------------------------------- Editor Handlers ---------------------------------------
void handleFileList() 
{
  if(!server.hasArg("dir")) 
    {
        server.send(500, "text/plain", "BAD ARGS"); 
        return;
    }
  String path = server.arg("dir");
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);
  path = String();

  String output = "[";
  while(dir.next())
    {
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);
    output += "\"}";
    entry.close();
  }
  output += "]";
  server.send(200, "text/json", output);
}
bool handleFileRead(String path)
{
    if(path.endsWith("/")) 
        path += "index.htm";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";
    
    if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
    {
        if(SPIFFS.exists(pathWithGz))
            path += ".gz";
    
        File file = SPIFFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}
bool handleFileRead1(String path)  // send the right file to the client (if it exists)
{
    Serial.println("handleFileRead: " + path);
    if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
    String contentType = getContentType(path);             // Get the MIME type
    String pathWithGz = path + ".gz";
    if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}
void handleFileDelete()
{
  if(server.args() == 0) 
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}
void handleFileCreate()
{
  if(server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  
  String path = server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(SPIFFS.exists(path))
    return server.send(500, "text/plain", "FILE EXISTS");
  
  File file = SPIFFS.open(path, "w");
  if(file)
    file.close();
  else
    return server.send(500, "text/plain", "CREATE FAILED");
  
  server.send(200, "text/plain", "");
  path = String();
}
File fsUploadFile;
void handleFileUpload()
{                                                       // upload a new file to the SPIFFS
    HTTPUpload& upload = server.upload();
    
    if(upload.status == UPLOAD_FILE_START)
    {
        String filename = upload.filename;
        if(!filename.startsWith("/")) 
            filename = "/"+filename;        
        Serial.print("handleFileUpload Name: "); Serial.println(filename);
        fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
        filename = String();
    } 
    else if(upload.status == UPLOAD_FILE_WRITE)
    {
        if(fsUploadFile)
        {
            fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
            PF("Current Size = %d\r\n", upload.currentSize );
        }
    } 
    else if(upload.status == UPLOAD_FILE_END)
    {
        if(fsUploadFile) 
        {                                                       // If the file was successfully created
          fsUploadFile.close();                                 // Close the file again
          Serial.print("handleFileUpload Size: "); 
          Serial.println(upload.totalSize);
          char *ok = "<html><body><h2 align='center'>OK!<br/><br/><a href='/'>Goto ROOT</a></h2></body></html>";
//          server.sendHeader("Location","/success.htm");        // Redirect the client to the success page
//          server.send(303);
            server.send(200, "text/html", ok );
        } 
        else 
        {
          server.send(500, "text/plain", "500: couldn't create file");
        }
    }
}    
void handleFileUploadOLD()
{
  if(server.uri() != "/edit") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
    {
    String filename = upload.filename;
    if(!filename.startsWith("/")) 
      filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } 
  else if(upload.status == UPLOAD_FILE_WRITE)
    {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } 
  else if(upload.status == UPLOAD_FILE_END)
    {
    if(fsUploadFile)
      fsUploadFile.close();
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}
