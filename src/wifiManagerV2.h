#pragma once
#ifndef J54J6_SIMPLEWIFIMANAGERV2_H
#define J54J6_SIMPLEWIFIMANAGERV2_H

#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include "led.h"
#include "filemanager.h"
#include "fallbackFileDefines.h"
#include "logger.h"
#include "webserverManager.h"


#define WiFiCheckDelay 50
#define FPM_SLEEP_MAX_TIME 0xFFFFFFF


//WiFi Libary Defined
#define WiFiNoShieldErrorLedDelay 300
#define WiFiIdleStatusWiFiLedDelay 100
#define WiFiNoSSIDAvalLedDelay 1000


struct macAdress {
    uint8_t macAddr[6];  
};

class WiFiManager {
    private:
        Filemanager* FM;
        #ifdef compileLoggingClassCritical
            SysLogger* logger = new SysLogger(FM, "wifiManager");
        #endif
        WebManager* webManager;
        ESP8266WebServer* webserver;
        DNSServer* dnsServer;

        LED wifiLED;

        //Local variables
        int classCheckDelay = 50; //exec. run function every 50ms
        unsigned long lastCall = 0;

        boolean wifiLEDAvail = false;
        int wifiLEDPin = -1;
        int currentDiscoveredNetworks = 0;
        unsigned long connectionTimeout = 10;


        boolean wifiAPEnabled = true;
        boolean wifiSTAEnabled = true;
        boolean wifiAlreadyConfigured = false;
        boolean wifiSTAIsConnected = false;
        boolean wifiSTAIsConnecting = false;
        
        String apSSID = "no.SSID";
        String apPSK = "no.PSK";
        String hostname = "";

        String homeSSID = "n.A";
        String homePSK = "n.A";
        int32_t homeChannel = 0;
        uint8_t homeBSSID = -1;


        IPAddress currentAPIP;
        IPAddress currentAPSubnet;
        IPAddress currentAPGateway;

        WiFiClient localWiFiClient;

        unsigned long lastConnectedToWiFi = 0;
        wl_status_t lastWiFiState = WL_DISCONNECTED;
        boolean WiFiStateHaschanged = false;
        boolean APForRegisterActive = false;

        boolean hostnameSetOnWiFi = false;
        // internal used functions
        
    protected:
        boolean loadLEDConfig(); //check for the installed LED file and read pin specified for "wifiLED"
        boolean loadWifiConfig(); //load wifiConfig into RAM -> SSID, PSK and setup Data if needed
        boolean createWifiConfig(); //creates wifi Config with default values

        void setOpticalMessage();
        boolean setWiFiMode();
        boolean loadInternalAPConfig();


        void addWiFiManagementSite(boolean forRegister = false);
        void sendWiFiSite();
        void wifiSiteProcessor(DynamicJsonDocument &keys);
        void saveWiFiCredentials();
        void addWebInterface();

    public:
        /*
            Constructor
        */
        WiFiManager(Filemanager* FM, WebManager* webManager); //LED is read out from config (checked on startup -> if LED exist the led will be used: name: "wifiLED")
        boolean init();


        //Get Stuff
        String getMacAddress();
        boolean isWiFiConfigured();
        boolean getWifiIsConnected();
        boolean getClientIsConnectedOnAP();
        uint8_t getNumberOfClientsConnectedToAP();
        bool isWiFiEnabled();
        String getWiFiHostname();
        wl_status_t getWiFiState();
        WiFiClient* getWiFiClient();
        WiFiClient& getRefWiFiClient();
        String getLocalIP();
        int getConnectionTimeout();

        IPAddress getCurrentAPIP();
        IPAddress getCurrentAPGWIP();
        IPAddress getCurrentAPSubnet();

        //  Set Stuff
        boolean setWifiHostname(String newHostname);
        void setAPForRegisterMode(boolean newMode = true);
        void setDNSServer(DNSServer* dnsServer);

        // Wifi Station control

        /*
            Start Wifi-Station

            This function will establish a connection to the registered Network.

            SSID, PSK and auto connect are read from wifiConfig file.
            
            Network Conf is checked in networkConfig - like DHCP or Static IP
        */
        boolean startWifiStation(); //start wifistation - SSID PSK and other properties are read out from FS.
        boolean stopWifiStation(bool disableWifiShield = false); //If true the "hardware" wifi will be disabled
        boolean changeWifiSTACreds(String ssid, String psk, boolean reloadConfigAfterSave = true, int32_t channel = -1, uint8_t bssid = -1);
        boolean wifiStationInstantConnect(String SSID, String psk);



        //boolean enableEnterpriseMode(); //Not implemented! - use Certificates instead of SSID and PSK for network connection


        // AP Station control
        /*
            if persistant = false -> Changes are not written to FS -> after restart old state (e.g off) is restored
        */
        boolean startWifiAP(boolean persistant = true, boolean registerWiFIMode = false, String useAPConfig = ""); //start wifi AP - SSID, PSK etc. read out from FS - if no config is passed - system look in default Path if there is no config (general config) - default values are used
        boolean stopWifiAP(bool disableWifiShield = false); // if true the "hardware" wifi will be disabled



        //other stuff
        void checkForWiFiReopen();
        void setCorrectHostname();


        //RUN
        void run();
};

#endif