#ifndef SIMPLESRV_H
#define SIMPLESRV_H

#include <Arduino.h>
	
// ------- Exported functions from SimpleSVR.cpp ----------------------------------

    void srvCallbacks();

    void setTrace( int value );
    int getTrace();
    #define T_FILEIO  1
    #define T_REQUEST 2
    #define T_JSON    4
    #define T_ARGS    8
    #define T_ACTIONS 16

    void showArgs( );
    void showJson( String &json );
	void showJson( char *s );
    
    String fileList( bool br = false );
    String formatBytes(size_t bytes);
    String getContentType(String filename);
	bool handleFileRead(String path);

#endif
