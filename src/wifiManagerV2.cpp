#include "WifiManagerV2.h"

WiFiManager::WiFiManager(Filemanager* FM, WebManager* webManager)
{
    this->FM = FM;
    this->webManager = webManager;
    this->webserver = this->webManager->getWebserver();
    //this->init();
}

boolean WiFiManager::init()
{
    WiFi.persistent(true);
    WiFi.setAutoConnect(true);
    
    loadLEDConfig();

    if(this->wifiLEDPin == -1)
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("init"), F("No wifi LED present."), 2);
        #endif
        this->wifiLED = LED(this->wifiLEDPin);
    }
    else
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("init"), F("LED Pin readed from Config - LED Pin is: ") + String(this->wifiLEDPin));
        #endif
        this->wifiLED = LED(this->wifiLEDPin);
        this->wifiLED.blink(1000, false, 3);
    }
    boolean configLoaded = loadWifiConfig();

    if(!configLoaded)
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("init"), F("Error while loading config!"), 5);
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("init"), F("Wifi config successfully loaded!"));
    #endif
    return true;
}

boolean WiFiManager::loadLEDConfig()
{
    if(!FM->fExist(LEDConfig))
    {
        #ifndef defaultWiFiLEDPin
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadLEDConfig"), F("No LED Config file found. Looks like there are no LED's on this device..."));
            #endif
            return true;
        #else
            this->FM->appendJsonKey(LEDConfig, "wifiLED", String(defaultWiFiLEDPin).c_str());
            return loadLEDConfig();
        #endif
        
    }
    else
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("loadLEDConfig"), F("LED Config file found... look for wifi led pin..."));
        #endif
        if(!this->FM->checkForKeyInJSONFile(LEDConfig, "wifiLED"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadLEDConfig"), F("seems like this device doen't have a wifi LED..."));
            #endif
            return true;
        }
        else
        {
            String value = this->FM->readJsonFileValue(LEDConfig, "wifiLED");
            int wifiLEDPin = value.toInt();
            this->wifiLEDPin = wifiLEDPin;
            this->wifiLEDAvail = true;
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadLEDConfig"), F("Successfully loaded wifi LED Pin - Pin number is: ") + String(this->wifiLEDPin));
            #endif
            return true;
        }
    }
    return false;
}

boolean WiFiManager::loadWifiConfig()
{
    boolean success = true;
    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("loadWifiConfig"), F("Wifi Configfile doesn't exist - create Config with default values!"));
        #endif
        boolean configCreated = this->createWifiConfig();
        
        if(configCreated)
        {
            return loadWifiConfig();
        }
        else
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("loadWifiConfig"), F("Can't create wifi config... See log above"), 5);
            #endif
            return false;
        }
    }
    else
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("loadWifiConfig"), F("Config file exist - load config..."));
        #endif

        //check if wifi is already configured

        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "configured"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No 'configured' value in config File! - recreate config"));
            #endif
            if(createWifiConfig())
            {
                return loadWifiConfig();
            }
            else
            {
                return false;
            }
        }
        else
        {
            String value = this->FM->readJsonFileValueAsString(wifiConfig, "configured");
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("Readed 'configured' value from config. Value: ") + value);
            #endif
            this->wifiAlreadyConfigured = this->FM->returnAsBool(value.c_str());
        }

        /*
            Read connection timeout

        */
        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "connectionTimeout"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("no connection timeout found in config File - use default value (60) and add it to config"));
            #endif
            boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "connectionTimeout", "60");

            if(!keyAdded)
            {
                #ifdef compileLoggingClassError
                    this->logger->logIt(F("loadWifiConfig"), F("Can't add key to wifiConfig! - See log above"), 5);
                #endif
                success = false;
                this->connectionTimeout = 60;
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("Key connectionTimeout added to wifiConfig!"), 3);
                #endif
                this->connectionTimeout = 60;
            }
        }
        else
        {
            String timeout = this->FM->readJsonFileValueAsString(wifiConfig, "connectionTimeout");
            if(timeout == "")
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("No value readed from wifiConfig JSON File - change it to default value (60)"));
                #endif
                this->FM->changeJsonValueFile(wifiConfig, "connectionTimeout", "60");
                this->connectionTimeout = 60;
                success = false;
            }
            else
            {
                this->connectionTimeout = timeout.toInt();
            }
        }

        /*
            Read AP SSID
        */
        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apSSID"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No 'apSSID' value in config File! - recreate config"));
            #endif
            if(createWifiConfig())
            {
                return loadWifiConfig();
            }
            else
            {
                return false;
            }
        }
        else
        {
            String value = this->FM->readJsonFileValueAsString(wifiConfig, "apSSID");
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("Readed 'apSSID' value from config. Value: ") + value);
            #endif
            this->apSSID = value;
        }

        /*
            Read AP PSK from FS
        */
        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apPSK"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No 'apPSK' value in config File! - recreate config"));
            #endif
            if(createWifiConfig())
            {
                return loadWifiConfig();
            }
            else
            {
                return false;
            }
        }
        else
        {
            String value = this->FM->readJsonFileValueAsString(wifiConfig, "apPSK");
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("Readed 'apPSK' value from config. Value: ") + value);
            #endif
            this->apPSK = value;
        }

        if(this->wifiAlreadyConfigured)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("wifi already configured. Load SSID and PSK of home network"));
            #endif
            //SSID
            if(!FM->checkForKeyInJSONFile(wifiConfig, "SSID"))
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("no SSID found in wifi config! - set wifiConfigured to false!"));
                #endif
              this->FM->changeJsonValueFile(wifiConfig, "configured", "false");  
              success = false;
            }
            else
            {
                this->homeSSID = this->FM->readJsonFileValueAsString(wifiConfig, "SSID");
                #ifdef compileLoggingClassDebug
                    this->logger->logIt(F("loadWifiConfig"), "Readed home SSID from Config: " + this->homeSSID, 2);
                #endif
            }
            //PSK
            if(!this->FM->checkForKeyInJSONFile(wifiConfig, "PSK"))
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("no PSK found in wifi config! - maybe it is a hotspot..."));
                #endif
              this->homePSK = "";
            }
            else
            {
                this->homePSK = this->FM->readJsonFileValueAsString(wifiConfig, "PSK");
                #ifdef compileLoggingClassDebug
                    this->logger->logIt(F("loadWifiConfig"), F("Readed home PSK from Config: ") + this->homePSK, 2);
                #endif
            }
            //check for adv. config (BSSID and Channel)

            //Channel
            if(!this->FM->checkForKeyInJSONFile(wifiConfig, "StaChannel"))
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("no StaChannel found in wifi config! - SKIP"));
                #endif
            }
            else
            {
                this->homeChannel = this->FM->readJsonFileValueAsString(wifiConfig, "StaChannel").toInt();
                #ifdef compileLoggingClassDebug
                    this->logger->logIt(F("loadWifiConfig"), F("Readed StaChannel from Config: ") + String(this->homeChannel), 2);
                #endif
            }

            //BSSID
            if(!this->FM->checkForKeyInJSONFile(wifiConfig, "StaBSSID"))
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("no StaBSSID found in wifi config! - SKIP"));
                #endif
            }
            else
            {
                this->homeBSSID = this->FM->readJsonFileValueAsString(wifiConfig, "StaBSSID").toInt();
                #ifdef compileLoggingClassDebug
                    this->logger->logIt(F("loadWifiConfig"), F("Readed StaBSSID from Config: ") + String(this->homeBSSID), 2);
                #endif
            }
        }
        
        //Read current wifi Shield states
        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apEnabled"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No apEnabled key found! - use default value and add it to config "));
            #endif
            boolean result = this->FM->appendJsonKey(wifiConfig, "apEnabled", "true");
            this->wifiAPEnabled = true;
            if(!result)
            {
                #ifdef compileLoggingClassWarn
                    this->logger->logIt(F("loadWifiConfig"), F("Can't add apEnabled value to config file!"), 4);
                #endif
                success = false;
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("apEnabled value successfully added to Config!"), 3);
                #endif
            }
        }
        else
        {
            String apEnabled = this->FM->readJsonFileValueAsString(wifiConfig, "apEnabled");
            this->wifiAPEnabled = this->FM->returnAsBool(apEnabled.c_str());

            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("loadWifiConfig"), F("apEnabled successfully read from Config file - value: ") + apEnabled, 2);
            #endif
        }

        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "staEnabled"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No staEnabled key found! - use default value and add it to config"));
            #endif
            boolean result = this->FM->appendJsonKey(wifiConfig, "staEnabled", "true");
            this->wifiSTAEnabled = true;
            if(!result)
            {
                #ifdef compileLoggingClassWarn
                    this->logger->logIt(F("loadWifiConfig"), F("Can't add staEnabled value to config file!"), 4);
                #endif
                success = false;
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("staEnabled value successfully added to Config!"), 3);
                #endif
            }
        }
        else
        {
            String staEnabled = this->FM->readJsonFileValueAsString(wifiConfig, "staEnabled");
            this->wifiSTAEnabled = this->FM->returnAsBool(staEnabled.c_str());

            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("loadWifiConfig"), F("staEnabled successfully read from Config file - value: ") + staEnabled, 2);
            #endif
        }

        //read/set hostname (default mac Address)
        if(!this->FM->checkForKeyInJSONFile(wifiConfig, "hostname"))
        {
            
            String macAddress = WiFi.macAddress();
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadWifiConfig"), F("No hostname found in config - add and use default hostname (") + macAddress + F(")"));
            #endif
            boolean result = this->FM->appendJsonKey(wifiConfig, "hostname", String(macAddress).c_str());
            this->hostname = macAddress;
            if(!result)
            {
                #ifdef compileLoggingClassWarn
                    this->logger->logIt(F("loadWifiConfig"), F("Can't add hostname value to config file!"), 4);
                #endif
                success = false;
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("loadWifiConfig"), F("hostname value successfully added to Config!"), 3);
                #endif
            }
        }
        else
        {
            String readedHostname = this->FM->readJsonFileValueAsString(wifiConfig, "hostname");
            this->hostname = readedHostname;
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("loadWifiConfig"), F("hostname successfully read from Config file - value: ") + readedHostname, 2);
            #endif
        }

        return success; 
    }
    return false;
}

boolean WiFiManager::createWifiConfig()
{
    boolean success = true;

    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("createWifiConfig"), F("No config availiable... create new Config!"));
    #endif
    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("creteWifiConfig"), F("Create config File on FS"), 2);
        #endif
        boolean fileCreated = this->FM->createFile(wifiConfig);

        if(!fileCreated)
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("createWifiConfig"), F("Can't create Config File on FS!"), 5);
            #endif
            return false;
        }
    }

    //Check for all required keys
    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "configured"))
    {
        //default false -> on startup instantly start wifi AP
        boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "configured", "false");

        if(!keyAdded)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting 'configured' key to wifi config"), 4);
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apSSID"))
    {
        //default newSensor -> on startup instantly start wifi AP with this SSID
        boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "apSSID", "newSensor");

        if(!keyAdded)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting 'apSSID' key to wifi config"), 4);
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apPSK"))
    {
        //default newSensor -> on startup instantly start wifi AP with this SSID
        boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "apPSK", "dzujkhgffzojh");

        if(!keyAdded)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting 'apPSK' key to wifi config"), 4);
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "apEnabled"))
    {
        //default newSensor -> on startup instantly start wifi AP with this SSID
        boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "apEnabled", "true");

        if(!keyAdded)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting 'apEnabled' key to wifi config"), 4);
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "staEnabled"))
    {
        //default newSensor -> on startup instantly start wifi AP with this SSID
        boolean keyAdded = this->FM->appendJsonKey(wifiConfig, "staEnabled", "true");

        if(!keyAdded)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting 'staEnabled' key to wifi config"), 4);
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "connectionTimeout"))
    {
        boolean keyadded = this->FM->appendJsonKey(wifiConfig, "connectionTimeout", "60");

        if(!keyadded)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting key 'connectionTimeout' to wificonfig"));
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "autoReconnect"))
    {
        boolean keyadded = this->FM->appendJsonKey(wifiConfig, "autoReconnect", "true");

        if(!keyadded)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting key 'autoReconnect' to wificonfig"));
            #endif
            success = false;
        }
    }

    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "autoReconnect"))
    {
        boolean keyadded = this->FM->appendJsonKey(wifiConfig, "autoReconnect", "true");

        if(!keyadded)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("createWifiConfig"), F("Error while inserting key 'autoReconnect' to wificonfig"));
            #endif
            success = false;
        }
    }
    return success;
}

void WiFiManager::setOpticalMessage()
{
    if(!this->isWiFiConfigured())
    {

        if(WiFi.softAPgetStationNum() > 0)
        {
            this->wifiLED.blink(wifiNotConfiguredDeviceConnectedToAP);
        }
        else
        {
            this->wifiLED.blink(wifiNotConfigured);
        }
    }
    else
    {
        wl_status_t wifiState = WiFi.status();

        switch(wifiState)
        {
            case WL_CONNECTED:
                this->wifiLED.ledOn();
                break;
            case  WL_NO_SHIELD:
                this->wifiLED.ledOff();
                break;
            case WL_IDLE_STATUS:
                this->wifiLED.blink(wifi_idle);
                break;
            case WL_NO_SSID_AVAIL:
                this->wifiLED.blink(wifi_no_ssid_avail);
                break;   
            case WL_SCAN_COMPLETED:
                this->wifiLED.blink(wifi_scan_completed);
                break;  
            case WL_CONNECT_FAILED:
                this->wifiLED.blink(wifi_connect_failed);
                break;   
            case WL_CONNECTION_LOST:
                this->wifiLED.blink(wifi_connection_lost);
                break;
            case WL_DISCONNECTED:
                this->wifiLED.blink(wifi_disconnected);
                break;
            case WL_WRONG_PASSWORD:
                this->wifiLED.blink(200, false, 20);
                break;
        }
    }
}

boolean WiFiManager::setWiFiMode()
{
    //check for wifiConfig
    if(!FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("setWiFiMode"), F("Can't set WiFi Mode - No WiFi Config found!"), 5);
        #endif
        return false;
    }

    //Config exist - read all param

    /*
    boolean configLoaded = this->loadWifiConfig();

    if(!configLoaded)
    {
        this->logger->logIt(F("setWiFiMode"), F("Can't load Wifi Config! - loading failed!", 5);
        return false;
    }
    */

    boolean modeSuccessfullySet = false;

    if(this->wifiSTAEnabled && this->wifiAPEnabled)
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("setWiFiMode"), F("Set WiFI Mode to AP_STA"), 2);
        #endif
        modeSuccessfullySet = WiFi.mode(WIFI_AP_STA);
    }
    else if(this->wifiSTAEnabled && !this->wifiAPEnabled)
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("setWiFiMode"), F("Set WiFI Mode to STA"), 2);
        #endif
        modeSuccessfullySet = WiFi.mode(WIFI_STA);
    }
    else if(!this->wifiSTAEnabled && this->wifiAPEnabled)
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("setWiFiMode"), F("Set WiFI Mode to AP"), 2);
        #endif
        modeSuccessfullySet = WiFi.mode(WIFI_AP);
    }
    else
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("setWiFiMode"), F("Set WiFI Mode to OFF"), 2);
            this->logger->logIt(F("setWiFiMode"), F("STAENabled: ") + String(this->wifiSTAEnabled) + ", APEnabled: " + String(this->wifiAPEnabled), 2);
        #endif
        modeSuccessfullySet = WiFi.mode(WIFI_OFF);
    }

    if(modeSuccessfullySet)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("setWiFiMode"), F("New wifi mode successfully set!"), 3);
        #endif
        return true;
    }
    else
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("setWiFiMode"), F("Error while changing WiFI Mode!"), 5);
        #endif
        return false;
    }

    //Only for correct C 
    return false;
}


/*
    GET Stuff
*/
String WiFiManager::getMacAddress()
{
    String macAddress = WiFi.macAddress();
    return macAddress;
}

boolean WiFiManager::isWiFiConfigured()
{
    return this->wifiAlreadyConfigured;
}


boolean WiFiManager::getWifiIsConnected()
{
    if(WiFi.status() == WL_CONNECTED)
    {
        return true;
    }
    return false;
}

boolean WiFiManager::getClientIsConnectedOnAP()
{
    if(WiFi.softAPgetStationNum() > 0)
    {
        return true;
    }
    return false;
}

uint8_t WiFiManager::getNumberOfClientsConnectedToAP()
{
    if(!this->wifiAPEnabled && (WiFi.getMode() == WIFI_OFF || WiFi.getMode() == WIFI_STA))
    {
        //Wifi AP cant be active (Shield is disabled or only STA is active)
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("getNumberOfClientsConnectedToAp"), F("WiFi AP is disabled - No nodes active"));
        #endif
        return 0;
    }
    else
    {
        uint8_t connectedStations = WiFi.softAPgetStationNum();
        return connectedStations;
    }
}

bool WiFiManager::isWiFiEnabled()
{
    return this->wifiSTAEnabled;
}

String WiFiManager::getWiFiHostname()
{
    return WiFi.hostname();
}

wl_status_t WiFiManager::getWiFiState()
{
    return WiFi.status();
}

WiFiClient* WiFiManager::getWiFiClient()
{
    return &this->localWiFiClient;
}

WiFiClient& WiFiManager::getRefWiFiClient()
{
    return this->localWiFiClient;
}

String WiFiManager::getLocalIP()
{
    return WiFi.localIP().toString();
}

int WiFiManager::getConnectionTimeout()
{
    return this->connectionTimeout;
}

IPAddress WiFiManager::getCurrentAPIP()
{
    return this->currentAPIP;
}

IPAddress WiFiManager::getCurrentAPGWIP()
{
    return this->currentAPGateway;
}

IPAddress WiFiManager::getCurrentAPSubnet()
{
    return this->currentAPSubnet;
}


//Set Stuff
boolean WiFiManager::setWifiHostname(String newHostname)
{
    return WiFi.hostname(newHostname);
}

void WiFiManager::setAPForRegisterMode(boolean newMode)
{
    this->APForRegisterActive = newMode;
}

void WiFiManager::setDNSServer(DNSServer* dnsServer)
{
    this->dnsServer = dnsServer;
}


/*
    WiFi Station control
*/

boolean WiFiManager::startWifiStation()
{
    
    //check state in file
    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("There is no config File on the FS! - Create it with default values!"));
        #endif
        if(!createWifiConfig())
        {
            return false;
        }
    }
    if(!this->FM->checkForKeyInJSONFile(wifiConfig, "staEnabled"))
    {
        #ifdef compileLoggingClassWarn
            this->logger->logIt(F("startWifiStation"), F("There is no key called 'staEnabled' in the config File - try to append this key"), 4);
        #endif

        boolean keyAppended = this->FM->appendJsonKey(wifiConfig, "staEnabled", "true");

        if(!keyAppended)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiStation"), F("Error while appending key to wifiConfig!"));
            #endif
        }
    }
    else
    {
        boolean keyChanged = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "true");

        if(!keyChanged)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("startWifiStation"), F("Can't change staEnabled key in Config File!"), 4);
            #endif
            return false;
        }
    }
    
    //Set coorect wifi Shield mode
    boolean shieldModeSet = this->setWiFiMode();

    if(!shieldModeSet)
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("startWifiStation"), F("Error while changing Shield Mode!"), 5);
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("startWifiStation"), F("Wifi Shieldmode changed!"));
    #endif

    //enable wifi station
    if(!WiFi.enableSTA(true))
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("startWifiStation"), F("Can't enable WiFi Station - enableSTA return false"), 5);
        #endif
        return false;
    }

    //check for network config and setup wifi STA
    //int32_t channel = 0;
    //uint8_t bssid = _NULL;

    //try to load ssid and psk from config

    if(!FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("No Wifi Config found! - can't start wifistation!"));
        #endif
        return false;
    }
    
    if(!wifiAlreadyConfigured)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("Seems like wifi is not configured at the time! - can't start WiFI Station!"));
        #endif
        return false;
    }

    //read from network config if exist and configure Wifi Station

    boolean useNetworkConfig = false;
    if(this->FM->fExist(networkConfig))
    {
        if(this->FM->checkForKeyInJSONFile(networkConfig, "ipConfig"))
        {
            if(this->FM->readJsonFileValueAsString(networkConfig, "ipConfig") != "DHCP")
            {
                useNetworkConfig = true;
            }
            else
            {
                boolean ipIsDHCP = WiFi.config(IPAddress(0,0,0,0), IPAddress(0,0,0,0), IPAddress(0,0,0,0));

                if(!ipIsDHCP)
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("startWifiStation"), F("Cant config wifi AP for DHCP use! - Config Fkt. return false"));
                    #endif
                    return false;
                }
            }
        }
    }

    if(useNetworkConfig)
    {
        //use network configuration from FS
        /*
            Readed from Config (possible options)
              - local_ip (IP Adress for Device)
              - gateway_ip
              - subnet
              - dns1
              - dns2
        */
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("Manual Network configuration detected. Load Config and configure STA"));
        #endif
       String ipAddress = "";
       String gatewayIp = ""; 
       String subnetIP  = ""; 
       String dns1      = ""; 
       String dns2      = "";

       if(!this->FM->fExist(networkConfig))
       {    
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiStation"), F("There is no network configuration File! - try to use DHCP"));
            #endif

            //Use DHCP instead
            useNetworkConfig = false;
       }
       else //Configure WIFi with given properties from networkConfigFile
       {
           //Check for all required properties in network COnfig
           if(this->FM->checkForKeyInJSONFile(networkConfig, "staIPAddress") && this->FM->checkForKeyInJSONFile(networkConfig, "staGatewayIP") && this->FM->checkForKeyInJSONFile(networkConfig, "staSubnet"))
           {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("startWifiStation"), F("Found valid IP Config in Network config file! - configure WIFI"));
                #endif

                ipAddress = this->FM->readJsonFileValueAsString(networkConfig, "staIPAddress");
                gatewayIp = this->FM->readJsonFileValueAsString(networkConfig, "staGatewayIP");
                subnetIP =  this->FM->readJsonFileValueAsString(networkConfig, "staSubnet");

                if(ipAddress != "" && gatewayIp != "" && subnetIP != "")
                {
                    //Convert Strings from FS to IP Addresses
                    IPAddress staIP;
                    IPAddress staGWIP;
                    IPAddress staSubnet;

                    bool staIPConverted = staIP.fromString(ipAddress);
                    bool staGWIPConverted = staGWIP.fromString(gatewayIp);
                    bool staSubnetConverted = staSubnet.fromString(subnetIP);

                    if(!staIPConverted || !staGWIPConverted || !staSubnetConverted)
                    {
                        #ifdef compileLoggingClassError
                            this->logger->logIt(F("startWifiStation"), F("Error while converting network configuration to IP Address fomat!"), 5);
                        #endif
                        return false;
                    }


                    boolean useDNS1 = false;
                    boolean useDNS2 = false;
                    //Check for dns Server IP's
                    if(this->FM->checkForKeyInJSONFile(networkConfig, "staDNS1IP"))
                    {
                        //DNS 1 Server IP Avail
                        dns1 = this->FM->readJsonFileValueAsString(networkConfig, "staDNS1IP");
                        useDNS1 = true;
                    }

                    if(this->FM->checkForKeyInJSONFile(networkConfig, "staDNS2IP"))
                    {
                        //DNS 2 Server IP Avail
                        dns2 = this->FM->readJsonFileValueAsString(networkConfig, "staDNS1IP");
                        useDNS2 = true;
                    }

                    useDNS1 = true;
                    useDNS2 = true;

                    IPAddress dns1IP;
                    IPAddress dns2IP;

                    if(useDNS1)
                    {
                        boolean dns1Converted = dns1IP.fromString(dns1);

                        if(!dns1Converted)
                        {
                            #ifdef compileLoggingClassWarn
                                this->logger->logIt(F("startWifiStation"), F("Can't convert DNS1 Address to IPAddress format! - disable DNS1 useage"), 4);
                            #endif
                            useDNS1 = false;
                        }
                    }

                    if(useDNS2)
                    {
                        boolean dns2Converted = dns2IP.fromString(dns2);

                        if(!dns2Converted)
                        {
                            #ifdef compileLoggingClassWarn
                                this->logger->logIt(F("startWifiStation"), F("Can't convert DNS2 Address to IPAddress format! - disable DNS2 useage"), 4);
                            #endif
                            useDNS2 = false;
                        }
                    }


                    //configure WiFi

                    boolean wifiSuccessfullyConfigured = false;

                    if(useDNS1 || useDNS2)
                    {
                        if(useDNS1 && useDNS2)
                        {
                            wifiSuccessfullyConfigured =  WiFi.config(staIP, staGWIP, staSubnet, dns1IP, dns2IP);
                        }
                        else if(useDNS1)
                        {
                            wifiSuccessfullyConfigured = WiFi.config(staIP, staGWIP, staSubnet, dns1IP);
                        }
                        else if(useDNS2)
                        {
                            wifiSuccessfullyConfigured = WiFi.config(staIP, staGWIP, staSubnet, dns2IP);
                        }
                    }
                    else
                    {
                        wifiSuccessfullyConfigured =  WiFi.config(staIP, staGWIP, staSubnet);
                    }
                }
           }  
           else
           {
               //Network config does not contain all required properties - use DHCP
               useNetworkConfig = false;
           }
       }
    }
    

    /*
        Used if there is no NetworkConfig avail. in NetworkConfig file or no keys in it

    */

    if(!useNetworkConfig)
    {
        //use DHCP config for wifi
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("No manual network config found! - use DHCP config"));
        #endif

        boolean wifiSuccessfullyConfigured = WiFi.config(0U, 0U, 0U);

        if(wifiSuccessfullyConfigured)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiStation"), F("WiFi DHCP Config successfully set!"));
            #endif
        }
        else
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("startWifiStation"), F("Error while setup DHCP Mode! - return!"), 5);
            #endif
            return false;
        }
    }


    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("startWifiStation"), F("SSID: ") + homeSSID + F(", PSK: ") + homePSK);
    #endif

    wl_status_t state = WiFi.begin(this->homeSSID, this->homePSK, this->homeChannel);
    if(state == WL_IDLE_STATUS || state == WL_DISCONNECTED || state == WL_CONNECTED)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("WiFi enabled... - Connecting to ") + homeSSID);
        #endif
        this->wifiSTAIsConnecting = true;

        if(this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "true"))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiStation"), F("WiFi STA successfully enabled..."));
            #endif

            return true;
        }
        else
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("startWifiStation"), F("Error while writing changes to FS! - things mitght get weird in the next time :/"), 4);
            #endif
            return false;
        }
    }
    else
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiStation"), F("Error while initialize conenction to SSID ") + homeSSID + ". Current State: " + String(WiFi.status()));
        #endif
        return false;
    }
}

boolean WiFiManager::stopWifiStation(boolean disableWifiShield)
{
    //Wifi is not connected to network or should disabled (class)
    if(!this->wifiSTAEnabled && !WiFi.isConnected())
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("stopWifiStation"), F("WiFi already disabled!"));
        #endif

        if(disableWifiShield)
        {
            WiFiMode_t currentWiFiMode =  WiFi.getMode();

            if(currentWiFiMode == WIFI_OFF)
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("stopWifiStation"), F("WiFi-Shield is already disabled"));
                #endif

                //Write information to FS
                boolean result = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "false");

                if(result)
                {
                    return true;
                }
                else
                {
                    #ifdef compileLoggingClassError
                        this->logger->logIt(F("stopWifiStation"), F("Error while disabling WiFi Station and write Info to FS (Shield is disabled)"), 5);
                    #endif
                    return false;
                }
            }
            else
            {
                boolean wifiNowDisabled = WiFi.mode(WIFI_OFF);

                if(wifiNowDisabled)
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("stopWifiStation"), F("WiFi-Shield is now disabled!"));
                    #endif

                    //Write information to FS
                    boolean result = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "false");
                    
                    if(result)
                    {
                        #ifdef compileLoggingClassInfo
                            this->logger->logIt(F("stopWifiStation"), F("Wifi Station is now disabled!"));
                        #endif
                        return true;
                    }
                    else
                    {
                        #ifdef compileLoggingClassInfo
                            this->logger->logIt(F("stopWifiStation"), F("Error while writing information to FS!"));
                        #endif
                        return false;
                    }
                }
                else
                {
                    #ifdef compileLoggingClassWarn
                        this->logger->logIt(F("stopWifiStation"), F("Error while disabling WiFi Shield"), 4);
                    #endif
                    return false;
                }
            }
        }
        else
        {
            boolean result = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "false");
                    
            if(result)
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("stopWifiStation"), F("Wifi Station is now disabled!"));
                #endif
                return true;
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("stopWifiStation"), F("Error while writing information to FS!"));
                #endif
                return false;
            }
        }
    }
    //WiFi is connected to network - disconnect and disable
    else
    {
        boolean disconnectFromWiFi = WiFi.disconnect(disableWifiShield);

        if(disconnectFromWiFi)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("stopWifiStation"), F("WiFi successfully disconnected"));
            #endif

            if(disableWifiShield)
            {
             //WiFi.disconnect already disable WiFi Shield
             #ifdef compileLoggingClassInfo
                this->logger->logIt(F("stopWifiStation"), F("WiFi Shield successfully disabled"));
            #endif
            }

            //Write information to FS
            boolean successfullyWritten = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "false");

            if(successfullyWritten)
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("stopWifiStation"), F("WiFI Station disabled and inforamtion saved!"), 3);
                #endif
                return true;
            }
            else
            {
                #ifdef compileLoggingClassWarn
                    this->logger->logIt(F("stopWifiStation"), F("Error while writing to FS!"), 4);
                #endif
                return false;
            }
        }
        else
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("stopWifiStation"), F("Error while disconnecting from WiFi - Disconnect return false!"), 5);
            #endif
            return false;
        }
    }

    //Error?
    return false;
}

boolean WiFiManager::changeWifiSTACreds(String ssid, String psk, boolean reloadConfigAfterSave, int32_t channel, uint8_t bssid)
{
    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("changeWifiSTACreds"), F("There is no Config File on your FS :/ - create it"), 3);
        #endif
        
        boolean newConfig = this->FM->createFile(wifiConfig);

        if(!newConfig)
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("changeWifiSTACreds"), F("Error while creating new Configfile!"), 5);
            #endif
            return false;
        }
        
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("changeWifiSTACreds"), F("New configfile successfully created!"));
        #endif

        return changeWifiSTACreds(ssid, psk, channel, bssid);
    }
    else
    {
        boolean ssidChanged = this->FM->changeJsonValueFile(wifiConfig, "SSID", ssid.c_str());
        boolean pskChanged = this->FM->changeJsonValueFile(wifiConfig, "PSK", psk.c_str());
        short channelChanged = -1; //Not set
        short bssidChanged = -1; // Not set


        if(channel != -1)
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("changeWifiSTACreds"), F("change 'WiFi Channel value!"), 2);
            #endif
            channelChanged = this->FM->changeJsonValueFile(wifiConfig, "StaChannel", String(channel).c_str());
        }

        if(bssid != -1)
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("changeWifiSTACreds"), F("Change BSSID value!"), 2);
            #endif
            bssidChanged = this->FM->changeJsonValueFile(wifiConfig, "StaBSSID", String(bssid).c_str());
        }
        
        if(ssidChanged && pskChanged && channelChanged != 0 && bssidChanged != 0)
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("changeWifiSTACreds"), F("Successfully edited STA Params!"), 2);
            #endif

            //set wifiConfigured to true (if not already set)
            boolean valChanged = this->FM->changeJsonValueFile(wifiConfig, "configured", "true");

            if(!valChanged)
            {
                #ifdef compileLoggingClassError
                    this->logger->logIt(F("changeWifiSTACreds"), F("Error while set WiFI mark to configured!"), 5);
                #endif
                return false;
            }

            if(reloadConfigAfterSave)
            {
                boolean configReloaded = this->loadWifiConfig();

                if(configReloaded)
                {
                    #ifdef compileLoggingClassDebug
                        this->logger->logIt(F("changeWifiSTACreds"), F("Config reloaded..."), 2);
                    #endif
                    return true;
                }
                else
                {
                    #ifdef compileLoggingClassError
                        this->logger->logIt(F("channgeWifiSTACreds"), F("Error while reloading Config!"), 5);
                    #endif
                    return false;
                }
            }
            else
            {
                return true;
            }
        }
    }
    #ifdef compileLoggingClassCritical
        this->logger->logIt(F("changeWifiSTACreds"), F("Unexcepted error!"), 6);
    #endif
    return false;
} 

boolean WiFiManager::wifiStationInstantConnect(String SSID, String psk)
{
    //enable wifiSTA (if not enabled)

    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("wifiStationInstantConnect"), F("There is no WiFI Config! - create config file and try again"));
        #endif
        createWifiConfig();
        return wifiStationInstantConnect(SSID, psk);
    }

    //enable STA in config
    boolean valueChanged = this->FM->changeJsonValueFile(wifiConfig, "staEnabled", "true");

    if(!valueChanged)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("wifiStationInstantConnect"), F("Error while modifying config!"));
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("wifiStationInstantConnect"), F("STA enabled in Config!"));
    #endif

    //config wifi STA with DHCP
    if(!WiFi.config(IPAddress(0,0,0,0), IPAddress(0,0,0,0), IPAddress(0,0,0,0)))
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("wifiStationInstantConnect"), F("Error while modifying STA config for DHCP use!"), 5);
        #endif
        return false;
    }

    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("wifiStationInstantConnect"), F("DHCP on STA enabled!"));
    #endif

    if(!WiFi.mode(WIFI_AP_STA))
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("wifiStationInstantConnect"), F("Error while modifying WIFI Mode config for AP_STA use!"), 5);
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("wifiStationInstantConnect"), F("WiFi mode set to AP_STA"));
    #endif
    if(WiFi.begin(SSID, psk))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("wifiStationInstantConnect"), F("Successfully started STA (backupMode)"));
        #endif
        if(WiFi.waitForConnectResult(DEFAULTWIFICONNECTIONTIMEOUT) != WL_CONNECTED)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("wifiStationInstantConnect"), F("timeout reaced - cant conneto to AP!"));
            #endif
            return false;
        }
        else
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("wifiStationInstantConnect"), F("WiFi successfully connected!"));
            #endif
            return true;
        }
    }
    else
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("wifiStationInstantConnect"), F("Error while starting STA! - Status: ") + String(WiFi.status()));
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("wifiStationInstantConnect"), F("Unexcepted error!"));
    #endif
    return false;
}

/*
    WiFi AP control
*/
boolean WiFiManager::startWifiAP(boolean persistant, boolean registerWiFiMode, String useAPConfig)
{
    //this->logger->logIt(F("startWifiAP"), F("Startup WiFi AP... - reason: " + reason, 3);
    if(!this->FM->fExist(wifiConfig))
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiAP"), F("Seems like there is no Wifi Config on your OS :/"));
        #endif
        return false;
    }

    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("startWifiAP"), F("Enable AP..."));
    #endif
    
    if(persistant)
    {
        boolean fileChanged = this->FM->changeJsonValueFile(wifiConfig, "apEnabled", "true");

        if(!fileChanged)
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("startWifiAP"), F("Can't write changed to FS! - Error"), 5);
            #endif
            return false;
        }
    }
    

    this->wifiAPEnabled = true;
    boolean wifiModeSet = this->setWiFiMode();

    if(!wifiModeSet)
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("startWifiAP"), F("Failed to set new WiFi Mode!"), 5);
        #endif
        return false;
    }
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("startWifiAP"), F("WiFi Mode successfully set..."));
    #endif

    WiFiMode_t currentWiFiStatus = WiFi.getMode();

    if(currentWiFiStatus != WIFI_AP && currentWiFiStatus != WIFI_AP_STA)
    {
        #ifdef compileLoggingClassCritical
            this->logger->logIt(F("startWifiAP"), F("Seems like the WiFi Module doesn't change it's opMode! - Error"), 6);
        #endif
        return false;
    }

    if(currentWiFiStatus == WIFI_AP_STA)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiAP"), F("Restart WiFi Connection - reconnecting to Station"));
        #endif
        boolean reConnectActive = this->startWifiStation();

        if(!reConnectActive)
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("startWifiAP"), F("There was an error while init. reconnect to WiFi Station! "), 4);
            #endif
        }
        else
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("startWifiAP"), F("Successfully init. reconnect to WiFi Station"), 2);
            #endif
        }
    }

    //load AP Config - if no config detected use default (10.0.0.1/8 , AP IP: 10.0.0.1 - Subnet: 255.0.0.0 - )

    //load passed config if there is one
    if(useAPConfig != "")
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("startWifiAP"), F("Network AP Config passed - try to load it... Path: ") + useAPConfig, 2);
        #endif
        //check if file exist
        if(this->FM->fExist(useAPConfig.c_str()))
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("startWifiAP"), F("Config found... try to read keys..."), 2);
            #endif

            //overwrite apSSID and apPSK if exist in config File
            if(this->FM->checkForKeyInJSONFile(useAPConfig.c_str(), "apName"))
            {
                this->apSSID = this->FM->readJsonFileValueAsString(useAPConfig.c_str(), "apName");
                #ifdef compileLoggingClassDebug
                    this->logger->logIt(F("startWifiAP"), F("Set ap SSID from given config... New SSID: ") + apSSID, 2);
                #endif
            }
            if(this->FM->checkForKeyInJSONFile(useAPConfig.c_str(), "apPass"))
            {
                this->apPSK = this->FM->readJsonFileValueAsString(useAPConfig.c_str(), "apPass");
                if(apPSK == "")
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("startWifiAP"), F("Set ap PSK from given Config... New PSK: No Password - this is an open AP"));
                    #endif
                }
                else
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("startWifiAP"), F("Set ap PSK from given Config... New PSK: ") + apPSK);
                    #endif
                }
            }

            //try to load other config (alternatively use default config from FS if there is also no config use default)
            
            //check if all infos avail
            boolean APIPAvail = this->FM->checkForKeyInJSONFile(useAPConfig.c_str(), "apIP");
            boolean APSubnetAvail = this->FM->checkForKeyInJSONFile(useAPConfig.c_str(), "apSubnet");
            //If config does not contain apGateway use same as APIP
            boolean APGWAvail = this->FM->checkForKeyInJSONFile(useAPConfig.c_str(), "apGateway");


            if(!APIPAvail || !APSubnetAvail)
            {
                #ifdef compileLoggingClassWarn
                    this->logger->logIt(F("startWifiAP"), F("Can't use passed IP Config - missing APIP or APSubnet - use internal Config"), 4);
                #endif
                boolean configLoaded = loadInternalAPConfig();

                if(configLoaded)
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                    #endif
                }
                else
                {
                    #ifdef compileLoggingClassError
                        this->logger->logIt(F("startWifiAP"), F("Error while configuring wifi AP! - Can't start WiFI AP"), 5);
                    #endif
                    return false;
                }
            }
            else
            {
                //Load IP Address and Subnet and convert them to IP Addresses
                //Read AP IP and convert to IP Address
                String APIP = this->FM->readJsonFileValueAsString(useAPConfig.c_str(), "apIP");
                IPAddress APIPAddress;
                boolean APIPConverted = APIPAddress.fromString(APIP);
                
                //Read AP Subnet and convert to IP Address
                String APSubnet = this->FM->readJsonFileValueAsString(useAPConfig.c_str(), "apSubnet");
                IPAddress APIPSubnet;
                boolean APSubnetConverted = APIPSubnet.fromString(APSubnet);


                //Check if all Addresses are successfully converted - if not use internal Config
                if(APIPConverted && APSubnetConverted)
                {
                    //If APGW is availiable in config use >GW if not use the APIP as GW
                    if(!APGWAvail)
                    {
                        //All Addresses converted config WiFi AP
                        #ifdef compileLoggingClassInfo
                            this->logger->logIt(F("startWifiAP"), F("Config Wifi AP - use APIP as GW"));
                        #endif
                        boolean softAPConfigured = WiFi.softAPConfig(APIPAddress, APIPAddress, APIPSubnet);
                        this->currentAPGateway = APIPAddress;
                        this->currentAPIP = APIPAddress;
                        this->currentAPSubnet = APIPSubnet;

                        if(softAPConfigured)
                        {
                            #ifdef compileLoggingClassInfo
                                this->logger->logIt(F("startWifiAP"), F("SoftAP successfully configured - open AP"));
                            #endif
                        }
                        else
                        {
                            #ifdef compileLoggingClassWarn
                                this->logger->logIt(F("startWifiAP"), F("Error while configuring AP with given Configuration file - try to load internal Config"), 4);
                            #endif
                            boolean loadedInternalAPConfig = loadInternalAPConfig();
                            
                            if(loadedInternalAPConfig)
                            {
                                #ifdef compileLoggingClassInfo
                                    this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                                #endif
                            }
                            else
                            {
                                #ifdef compileLoggingClassError
                                    this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
                                #endif
                                return false;
                            }
                        }
                    }
                    else
                    {
                        //read and Convert GW also 
                        String APGateway = this->FM->readJsonFileValueAsString(useAPConfig.c_str(), "apGateway");
                        IPAddress APIPGateway;
                        boolean APGatewayConverted = APIPGateway.fromString(APGateway);

                        if(APGatewayConverted)
                        {
                            //IP Address successfully converted config wifi ap
                            #ifdef compileLoggingClassInfo
                                this->logger->logIt(F("startWifiAP"), F("Config Wifi AP - use GW from config file"));
                            #endif
                            boolean softAPConfigured = WiFi.softAPConfig(APIPAddress, APIPGateway, APIPSubnet);
                            this->currentAPGateway = APIPGateway;
                            this->currentAPIP = APIPAddress;
                            this->currentAPSubnet = APIPSubnet;


                            if(softAPConfigured)
                            {
                                #ifdef compileLoggingClassInfo
                                    this->logger->logIt(F("startWifiAP"), F("SoftAP successfully configured - open AP"));
                                #endif
                            }
                            else
                            {
                                #ifdef compileLoggingClassWarn
                                    this->logger->logIt(F("startWifiAP"), F("Error while configuring AP with given Configuration file - try to load internal Config"), 4);
                                #endif
                                boolean loadedInternalAPConfig = loadInternalAPConfig();

                                if(loadedInternalAPConfig)
                                {
                                    #ifdef compileLoggingClassInfo
                                        this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                                    #endif
                                }
                                else
                                {
                                    #ifdef compileLoggingClassError
                                        this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
                                    #endif
                                    return false;
                                }
                            }
                        }
                        else
                        {
                            //try to use APIP as GW Address
                            //Fallback

                            //IP Address successfully converted config wifi ap
                            #ifdef compileLoggingClassWarn
                                this->logger->logIt(F("startWifiAP"), F("Config Wifi AP - Fallback - Can't convert IP from Gateway - use AP IP as GW"), 4);
                            #endif
                            boolean softAPConfigured = WiFi.softAPConfig(APIPAddress, APIPGateway, APIPSubnet);
                            this->currentAPGateway = APIPGateway;
                            this->currentAPIP = APIPAddress;
                            this->currentAPSubnet = APIPSubnet;

                            if(softAPConfigured)
                            {
                                #ifdef compileLoggingClassInfo
                                    this->logger->logIt(F("startWifiAP"), F("SoftAP successfully configured - open AP"));
                                #endif
                            }
                            else
                            {
                                #ifdef compileLoggingClassWarn
                                    this->logger->logIt(F("startWifiAP"), F("Error while configuring AP with given Configuration file - try to load internal Config"), 4);
                                #endif
                                boolean loadedInternalAPConfig = loadInternalAPConfig();

                                if(loadedInternalAPConfig)
                                {
                                    #ifdef compileLoggingClassInfo
                                        this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                                    #endif
                                }
                                else
                                {
                                    #ifdef compileLoggingClassError
                                        this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
                                    #endif
                                    return false;
                                }
                            }
                        }
                    }
                }
                else
                {
                    //error while converting addresses load internal config
                    #ifdef compileLoggingClassWarn
                        this->logger->logIt(F("startWifiAP"), F("Error while converting addresses! - try to load internal Config"), 4);
                    #endif
                    boolean loadedInternalAPConfig = loadInternalAPConfig();

                    if(loadedInternalAPConfig)
                    {
                        #ifdef compileLoggingClassInfo
                            this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                        #endif
                    }
                    else
                    {
                        #ifdef compileLoggingClassError
                            this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
                        #endif
                        return false;
                    }
                }                
            }
        }
        else
        {
            //Cant find passed config File! - try to load internal config
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("startWifiAP"), F("Can't find passed config file! - use internal config"), 4);
            #endif
            boolean loadedInternalAPConfig = loadInternalAPConfig();
            
            if(loadedInternalAPConfig)
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
                #endif
            }
            else
            {
                #ifdef compileLoggingClassError
                    this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
                #endif
                return false;
            }
        }
    }
    else
    {
        //no config passed! - use internal config
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("startWifiAP"), F("No Config passed - use internal config"), 2);
        #endif
        boolean loadedInternalAPConfig = loadInternalAPConfig();
            
        if(loadedInternalAPConfig)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiAP"), F("Internal Config successfully loaded!"));
            #endif
        }
        else
        {
            #ifdef compileLoggingClassError
                this->logger->logIt(F("startWifiAP"), F("Error while loading internal - cant start WiFi AP!"), 5);
            #endif
            return false;
        }
    }



    //if here you have in every case a valid wifi config - open WiFi AP

    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("startWifiAP"), F("Open WiFI AP..."));
    #endif
    
    boolean apStarted = WiFi.softAP(this->apSSID, this->apPSK);

    if(!apStarted)
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("startWifiAP"), F("Error while starting wifi AP - softAP return false!"), 5);
        #endif
        return false;
    }
    else
    {
        if(registerWiFiMode)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("startWifiAP"), F("Add DNS and MDNS Stuff to get Capture WiFI Portal"));
            #endif
            this->addWiFiManagementSite(true);
        }
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("startWifiAP"), F("AP successfully started! - SSID: ") + this->apSSID + ", PSK: " + this->apPSK);
        #endif
        return true;
    }
}

boolean WiFiManager::stopWifiAP(boolean disableWiFiShield)
{
    if(!this->wifiAPEnabled)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("stopWifiAP"), F("AP already disabled - SKIP"));
        #endif
        return true;
    }

    boolean softAPDisabled = WiFi.softAPdisconnect(disableWiFiShield);

    if(softAPDisabled)
    {
        this->wifiAPEnabled = false;
        bool valueChanged = this->FM->changeJsonValueFile(wifiConfig, "apEnabled", "false");

        if(valueChanged)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("stopWifiAP"), F("WiFiAP successfully disabled!"));
            #endif
            return true;
        }
        else
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("stopWifiAP"), F("Error while disabling WiFi AP - can't update configFIle"), 4);
            #endif
            return false;
        }
    }
    else
    {
        #ifdef compileLoggingClassError
            this->logger->logIt(F("stopWifiAP"), F("Error while disabling softAP -> retuns false"), 5);
        #endif
        return false;
    }
}

boolean WiFiManager::loadInternalAPConfig()
{
    //check for user config in wifi config if there is no config use default
    boolean configured = false;

    if(FM->fExist(wifiConfig))
    {
        //file exist - check for config
        boolean apIPExist = this->FM->checkForKeyInJSONFile(wifiConfig, "apIP");
        boolean apGWIPExist = this->FM->checkForKeyInJSONFile(wifiConfig, "apGWIP");
        boolean apSubnetExist = this->FM->checkForKeyInJSONFile(wifiConfig, "apSubnet");

        if(apIPExist && apSubnetExist)
        {
            //read wifiConfig and configure ap
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("loadInternalAPConfig"), F("try to load AP Config from wifi config"), 2);
            #endif

            String apIP = this->FM->readJsonFileValueAsString(wifiConfig, "apIP");
            IPAddress apIPAddress;
            boolean apIPConverted = apIPAddress.fromString(apIP);

            String apSubnetAddress = this->FM->readJsonFileValueAsString(wifiConfig, "apSubnet");
            IPAddress apSubnetIPAddress;
            boolean apSubnetConverted = apSubnetIPAddress.fromString(apSubnetAddress);


            if(apIPConverted && apSubnetConverted)
            {
                //both addresses are valid - try to load GWIP or use apIP as GW

                if(apGWIPExist)
                {
                    //try to load and use APGWIP
                    String apGWIP = this->FM->readJsonFileValueAsString(wifiConfig, "apGWIP");
                    IPAddress apGWIPAddress;
                    boolean apGWIPConverted = apGWIPAddress.fromString(apGWIP);

                    if(apGWIPConverted)
                    {
                        //useAPGWIP from Config
                        configured = WiFi.softAPConfig(apIPAddress, apGWIPAddress, apSubnetIPAddress);
                        this->currentAPGateway = apGWIPAddress;
                        this->currentAPIP = apIPAddress;
                        this->currentAPSubnet = apSubnetIPAddress;

                        if(configured)
                        {
                            #ifdef compileLoggingClassInfo
                                this->logger->logIt(F("loadInternalAPConfig"), F("Successfully configured AP with wifiConfig"));
                            #endif
                            return true;
                        }
                    }
                    else
                    {
                        //use APIPAsGW
                        configured = WiFi.softAPConfig(apIPAddress, apIPAddress, apSubnetIPAddress);
                        this->currentAPGateway = apIPAddress;
                        this->currentAPIP = apIPAddress;
                        this->currentAPSubnet = apSubnetIPAddress;

                        if(configured)
                        {
                            #ifdef compileLoggingClassInfo
                                this->logger->logIt(F("loadInternalAPConfig"), F("Successfully configured AP with wifiConfig (APGWIP = APIP)"));
                            #endif
                            return true;
                        }
                    }
                }
                else
                {
                    //use apGWIPASGW
                    configured = WiFi.softAPConfig(apIPAddress, apIPAddress, apSubnetIPAddress);
                    this->currentAPGateway = apIPAddress;
                    this->currentAPIP = apIPAddress;
                    this->currentAPSubnet = apSubnetIPAddress;
                    if(configured)
                    {
                        #ifdef compileLoggingClassInfo
                            this->logger->logIt(F("loadInternalAPConfig"), F("Successfully configured AP with wifiConfig (APGWIP = APIP - noGW in config)"));
                        #endif
                        return true;
                    }
                }
            }
        }
    }

    if(!configured)
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("loadInternalAPConfig"), F("Seems like there is no valid AP Config in WiFiConfig! - use default values"));
        #endif

        boolean softAPConfigured = WiFi.softAPConfig(IPAddress(defaultAPIP), IPAddress(defaultAPGateway), IPAddress(defaultAPSubnet));
        this->currentAPGateway = IPAddress(defaultAPGateway);
        this->currentAPIP = IPAddress(defaultAPIP);
        this->currentAPSubnet = IPAddress(defaultAPSubnet);

        if(softAPConfigured)
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("loadInternalAPConfig"), F("SoftAP successfully configured with default values!"));
            #endif
            return true;
        }
        else
        {
            #ifdef compileLoggingClassCritical
                this->logger->logIt(F("loadInternalAPConfig"), F("Error while configuring with default values!"), 6);
            #endif
            return false;
        }
    }
    #ifdef compileLoggingClassCritical
        this->logger->logIt(F("loadInternalAPConfig"), F("ERROR while loading - unexcepted error!"), 6);
    #endif
    return false;
}



void WiFiManager::addWiFiManagementSite(boolean forRegister)
{   
    this->webManager->stopWebserver();
    #ifdef compileLoggingClassDebug
        this->logger->logIt(F("addWiFiManagerSite"), F("Webserver stopped! - Add WiFi sites!"), 2);
    #endif
    
    this->webManager->registerNewService("/wifisettings", std::bind(&WiFiManager::sendWiFiSite, this));
    this->webManager->registerNewService("/saveWifi", std::bind(&WiFiManager::saveWiFiCredentials, this));
    
    #ifdef compileLoggingClassInfo
        this->logger->logIt(F("addWiFiManagementSite"), F("WiFiManagement Site added!"), 3);
    #endif
    if(forRegister)
    {
        //add MDNS use. to redirect device directly to site
        this->dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        //this->webserver->onNotFound(std::bind(&WiFiManager::sendWiFiSite, this));

        if(!this->dnsServer->start(53, "*", currentAPIP))
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("addWiFiManagementSite"), F("Can't start DNS Server!"), 4);
            #endif
        }
        else
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("addWiFiManagementSite"), F("DNS Started!"), 3);
            #endif
        }

        if(!MDNS.addService("http", "tcp", 80))
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("addWiFiManagementSite"), F("Can't add MDNS HTTP Service!"), 4);
            #endif
        }
        else
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("addWiFiManagementSite"), F("mDNS Service added!"), 3);
            #endif
        }

        if(!MDNS.begin(hostname))
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("addWiFiManagementSite"), F("Can't start mDNS Server!"), 4);
            #endif
        }
        else
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("addWiFiManagementSite"), F("mDNS Started!"), 3);
            #endif
        }
        if(!this->setWifiHostname("NodeworkNewDevice"))
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("initSetup"), F("Can't set WiFi Hostname!"), 4);
            #endif
        }
    }
    this->webManager->startWebserver();
}


void WiFiManager::wifiSiteProcessor(DynamicJsonDocument &keys)
{
    //Scan Networks and inject to page
    this->currentDiscoveredNetworks = WiFi.scanNetworks();

    keys["wifiActive"] = "active-subGroup";
    String ssid = ""; 
    if(this->FM->fExist(wifiConfig))
    {
        if(this->FM->checkForKeyInJSONFile(wifiConfig, "SSID"))
        {
            ssid = this->FM->readJsonFileValueAsString(wifiConfig, "SSID");
        }
        else
        {
            ssid = "Keine SSID vorhanden";
        }
    }
    else
    {
        ssid = "keine Config!";
    }

    if(ssid == "" || ssid.length() < 1)
    {
        ssid = "No Network configured!";
    }
    keys["ssid"] = ssid;

    //Hostname
    String hostname = "";
    if(this->FM->fExist(wifiConfig))
    {
        if(this->FM->checkForKeyInJSONFile(wifiConfig, "hostname"))
        {
            hostname = this->FM->readJsonFileValueAsString(wifiConfig, "hostname");
        }
        else
        {
            hostname = "Kein Hostname konfiguriert - Use MAC";
        }
    }
    else
    {
        hostname = "keine Config - Mac wird benutzt!";
    }

    if(hostname == "" || hostname.length() < 1)
    {
        hostname = "Kein gültiger hostname - MAC wird genutzt";
    }

    keys["hostname"] = hostname;

    if(WiFi.scanComplete() == -2)
    {
        #ifdef compileLoggingClassDebug
            this->logger->logIt(F("sendWiFiState"), F("Scan was not triggered! -start WiFi Scan!"), 2);
        #endif
    }
    while(WiFi.scanComplete() == -1)
    {
        yield();
    }

    if(WiFi.scanComplete() != -1)
    {
        if(currentDiscoveredNetworks == 0)
        {
          keys["foundedssids"] = "<option>Keine Netzwerke gefunden</option>";
        }
        else
        {
          String allSSIDoptions = "<option>SSID auswählen</option> ";
          for(int i = 0; i < currentDiscoveredNetworks; i++)
          {
            allSSIDoptions += "<option>" + String(WiFi.SSID(i)) + "</option> ";
          }
          keys["foundedssids"] = allSSIDoptions;
        }
    }
    else
    {
       keys["foundedssids"] = F("<option>Internal Server Error! - there was a Problem while scanning for WiFi Networks in your area! - Please restart your device or contact support!</option>");
    }
}

void WiFiManager::sendWiFiSite()
{
    DynamicJsonDocument keys(800);
    this->wifiSiteProcessor(keys);

    this->webManager->sendWebsiteWithProcessor("config/os/web/wifiSettings.html", keys);
}

void WiFiManager::saveWiFiCredentials()
{
    boolean restart = false;

    if(this->webserver->hasArg("ssid") && this->webserver->hasArg("passwd") &&this->webserver->arg("ssid") != "SSID auswählen")
    {
        #ifdef compileLoggingClassInfo
            this->logger->logIt(F("saveWiFiCredentials"), F("WiFi credentials send - test and save it"));
        #endif
        String ssid =  this->webserver->arg("ssid");
        String psk =  this->webserver->arg("passwd");
        this->webserver->keepAlive(true);
        this->webserver->client().setTimeout(20000);

        if(wifiStationInstantConnect(ssid, psk))
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("saveWiFiCredentials"), F("Successfully connected to WiFi AP! - Save to FS"));
            #endif

            if(changeWifiSTACreds(ssid, psk, false))
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("saveWiFiCredentials"), F("Credentials successfully saved!"));
                #endif
                String url = "http://" + currentAPIP.toString() + "/wifisettings?state=success";
                webserver->sendHeader("Location", url, true);
                webserver->send( 302, "text/plain", "");

                if(stopWifiAP())
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("saveWiFiCredentials"), F("WiFI AP disabled - reload config!"));
                    #endif
                    loadWifiConfig();
                }
                else
                {
                    #ifdef compileLoggingClassInfo
                        this->FM->changeJsonValueFile(wifiConfig, "apEnabled", "false");
                    #endif
                    restart = true;
                }
            }
            else
            {
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("saveWiFiCredentials"), F("Failed while saving Credentials!"));
                #endif
                String url = "http://" + currentAPIP.toString() + "?state=systemError";
                webserver->sendHeader("Location", url, true);
                webserver->send( 302, "text/plain", "");
            }
        }
        else
        {
            #ifdef compileLoggingClassInfo
                this->logger->logIt(F("saveWiFiCredentials"), F("Failed while connecting to AP - Maybe the password is wrong?!"));
            #endif
            String url = "http://" + currentAPIP.toString() + "?state=failed";
            webserver->sendHeader("Location", url, true);
            webserver->send( 302, "text/plain", "");
        }

        webserver->sendHeader("Location", "http://" + currentAPIP.toString(), true);
        webserver->send( 302, "text/plain", "");
    }

    if(this->webserver->hasArg("hostname"))
    {

        String newHostname = this->webserver->arg("hostname");

        if(newHostname.length() == 0)
        {
            logWarn("saveWiFiCredentials", "Cant change hostname - size is 0!")
            webserver->sendHeader("Location", "http://" + WiFi.localIP().toString() + "/", true);
            webserver->send( 302, "text/plain", "");
            return;
        }

        if(!this->FM->changeJsonValueFile(wifiConfig, "hostname", newHostname.c_str()))
        {
            logError("saveWiFiCredentials", "Error while saving new Hostname!")
            webserver->sendHeader("Location", "http://" + WiFi.localIP().toString() + "/", true);
            webserver->send( 302, "text/plain", "");
        }
        logInfo("saveWiFICredentials", "Hostname successfully changed to " + newHostname)
        webserver->sendHeader("Location", "http://" + WiFi.localIP().toString() + "/", true);
        webserver->send( 302, "text/plain", "");
        restart = true;
    }
    if(restart)
    {
        yield();
        delay(1000);
        ESP.restart();
    }
}


void WiFiManager::addWebInterface()
{
    static boolean initSuccessfull = false;
    if(this->isWiFiConfigured() && !initSuccessfull)
    {
        addWiFiManagementSite();
        initSuccessfull = true;
    }
    return;
}


void WiFiManager::checkForWiFiReopen()
{
    //Check for wifi connection and re-setup wifiAP and register portal if wifi is not connected for a certain time
    //Only check if wifi is already configured (if not AP is open in every case)
    if(this->wifiAlreadyConfigured && !this->wifiAPEnabled)
    {
        if(WiFi.status() == WL_CONNECTED)
        {
            this->lastConnectedToWiFi = millis();

            //if wifi AP already active for registration and wifi can connected - close AP
            if(APForRegisterActive)
            {
                if(this->stopWifiAP())
                {
                    this->APForRegisterActive = false;
                    #ifdef compileLoggingClassDebug
                        this->logger->logIt(F("checkForWiFiReopen"), F("AP successfully closed after WiFi reconnection!"), 2);
                    #endif
                }
                else
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("checkForWiFiReopen"), F("Error while closing WiFI AP!"));
                    #endif
                }
            }
        }
        else
        {
            //If not connected wait <<conection timeout>> then reopen AP
            unsigned long timeout = this->connectionTimeout * 1000;
            unsigned long reopentime = this->lastConnectedToWiFi  + timeout;

            if(reopentime < millis())
            {
                //reopen AP
                #ifdef compileLoggingClassInfo
                    this->logger->logIt(F("checkForWiFiReopen"), F("Connection timeout (") + String(reopentime) + ", " + String(timeout) + ") exceeded - reopen WiFi AP!");
                #endif
                if(this->startWifiAP(false, true))
                {
                    #ifdef compileLoggingClassInfo
                        this->logger->logIt(F("checkForWiFiReopen"), F("AP successfully restarted!"));
                    #endif
                    this->APForRegisterActive = true;
                    //this->addWiFiManagementSite(true);
                }
                else
                {
                    #ifdef compileLoggingClassWarn
                        this->logger->logIt(F("checkForWiFiReopen"), F("Can't restart WiFI AP - Error!"), 4);
                    #endif
                }
            }
        }
    }
    
}

void WiFiManager::setCorrectHostname()
{
    /*

    - 24 chars max
        - only a..z A..Z 0..9 '-'
        - no '-' as last char

    */

    //check hostname and modify it (if needed)

    String macAddr = hostname;
    macAddr.replace(":", "-");
    macAddr.replace(".", "-");
    macAddr.trim();
    if(macAddr.length() > 24)
    {
        macAddr.remove(23);
    }
    
    if(macAddr != hostname)
    {
        logDebug("setCorrectHostname", "Hostname is not valid - changed to valid version!")
        //change in config to prevent next change and to show on webinterface that hostname was changed
        this->FM->changeJsonValueFile(wifiConfig, "hostname", macAddr.c_str());
        loadWifiConfig();
    }

    if(WiFi.status() == WL_CONNECTED && !hostnameSetOnWiFi && this->wifiAlreadyConfigured)
    {
        if(WiFi.hostname(macAddr))
        {
            #ifdef compileLoggingClassDebug
                this->logger->logIt(F("setCorrectHostname"), F("Hostname successfully set (") + macAddr + ")", 2);
            #endif
            hostnameSetOnWiFi = true;
        }
        else
        {
            #ifdef compileLoggingClassWarn
                this->logger->logIt(F("setCorrectHostname"), F("Error while set hostname (") + macAddr + ") for WiFi network!", 4);
            #endif
        }
    }
}

void WiFiManager::run()
{
    if(millis() >= (lastCall + 50))
    {
        static int counter = 0;
        counter++;
        if(this->wifiLEDAvail && counter > 30)
        {
            counter = 0;
            setOpticalMessage();
            this->wifiLED.run();
        }
        checkForWiFiReopen();
        setCorrectHostname();
        addWebInterface();
        lastCall = millis();
    }
}
