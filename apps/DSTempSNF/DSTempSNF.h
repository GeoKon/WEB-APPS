/* ----------------------------------------------------------------------------------
 *  Copyright (c) George Kontopidis 1990-2019 All Rights Reserved
 *  You may use this code as you like, as long as you attribute credit to the author.
 * ---------------------------------------------------------------------------------
 */
#ifndef DSTEMPSNF_H
#define DSTEMPSNF_H

#define DS18B20     // either DS18B22 or DHT22
//#define DHT22

//#define NODEMCU     // either NODEMCU or SONOFF
#define SONOFF

// --------------------- Includes needed for this application -----------------

    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <FS.h>

    #include <cpuClass.h>
    #include <eepClass.h>
    #include <cpuClass.h>
    #include <cliClass.h>
    #include <eepClass.h>
    #include <timeClass.h>
    #include <eepTable.h>
    #include <ds18Class.h>
    #include <filtClass.h>
    #include "SimpleSRV.h"                // baseline for any web-server application
    
// ------ Expected definitions/allocations in main() ------------------------------

    extern ESP8266WebServer server;
    extern CPU cpu;                    // NodeMCU class
    extern EEP eep;                 // basic eeprom parameters 
    extern CLI cli;                 // console interaction`
    extern EXE exe;                 // CLI dispatch and execution
    extern DS18 temp;
    
    enum wifi_state { TRYING_WIFI=0, STA_MODE=2, AP_MODE=1 };
    
    enum thermode_t { NO_RELAY=0, HEAT_MODE=1, COOL_MODE=2 };
    
    #define NUM_TEMPS 2
    class Globals
    {
      public:
        wifi_state wifiOK; 
        bool trace;
        float tempC;
        float tempF;
        float humidity;
        
        bool relayON;
        bool simulON;
        
        struct                      // do not change the order of this
        {
            int tmode;              // (thermode_t) move these to EEPROM !
            int prdelay;            // propagation delay. Use 0 for no delay
            float threshold;        // always in deg C
            float delta;            // in deg C
        } gp;
        Globals();        
        void initUserParms();
    };
    #define GP_FORMAT "tmode:%d filter:%d target:%f°C delta:%f°C\r\n"
    
    extern Globals myi;
    
// Exported functions by ExampleSTA.cpp
    void blinkLED( int ms, uint32_t dly=0 );
    void setupSTA();
    bool loopSTA();
    void staCallbacks();
	void interactForever();  
    bool checkWiFi();

#endif
