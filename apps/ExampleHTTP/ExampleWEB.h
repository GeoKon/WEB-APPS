#ifndef EXAMPLEWEB_H
#define EXAMPLEWEB_H

// --------------------- Includes needed for this application -----------------

    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <FS.h>

    #include <cpuClass.h>
    #include <eepClass.h>
    #include <cpuClass.h>
    #include <cliClass.h>
    #include <eepClass.h>
    #include <eepTable.h>
    #include "SimpleSRV.h"                // baseline for any web-server application

// ------ Expected definitions/allocations in main() ------------------------------

    extern ESP8266WebServer server;
    extern CPU cpu;                    // NodeMCU class
    extern EEP eep;                 // basic eeprom parameters 
    extern CLI cli;                 // console interaction`
    extern EXE exe;                 // CLI dispatch and execution
    
// Exported functions by ExampleSTA.cpp

    void setupSTA();
    void loopSTA();
    void staCallbacks();
	void interactForever();      

// Exported functions by ExampleAP.cpp

    void setupAP();
    void loopAP();
    void apCallbacks();

#endif
