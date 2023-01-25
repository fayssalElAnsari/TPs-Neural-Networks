//#include "sensors.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "uptime_formatter.h"
#include "uptime.h"
#include "ota.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
//#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include "utils.h"

#define SaveDisconnectTime 1000 // Connection seems to need several tries (often two)
// Time in ms for save disconnection, => delay between try
// cf https://github.com/espressif/arduino-esp32/issues/2501  
#define WiFiMaxTry 10


/* ---- Set timer ---- */
unsigned long loop_period = 1L * 1000; /* =>  10000ms : 10 s */

/** We define a max and min temperatures 
 *  so that we can turn on the FAN when 
 *  the temperature exceeds MAX_TEMP
 *  and turn on the heater (non-existent)
 *  when the temperature gets below MIN_TEMP
 */
const int MAX_TEMP = 28;
const int MIN_TEMP = 25;

const int CoolerLED = 19;
const int HeaterLED = 21;
const int TempSensor = 23;
const int Fan = 27; // no fan
const int Tachometer = 26; // no tach
const int LightSensor = 33; // Read analog input on ADC1_CHANNEL_5 (GPIO 33)
const int FireLED = 2;
float t;// temp digit
int l;
int lastTime = 0;


/**
 * these are identifiers of the state
 * ofthe ESP32, when the ESP32 is on
 * The StateLED is also on
 */
String StateLED = "off";
String IDENTIFIER = "Fayssal_ESP32";
String LOCATION;
String UPTIME;
const char* hostname = "Fayssal_ESP32"; /* FOR OTA */

/* ---- Light ----*/
const int LightPin = A5; // Read analog input on ADC1_CHANNEL_5 (GPIO 33)

/* ---- TEMP ---- */
OneWire oneWire(23); // Pour utiliser une entite oneWire sur le port 23
DallasTemperature tempSensor(&oneWire) ; // Cette entite est utilisee
					 // par le capteur de
					 // temperature

/**
 * 
 */
bool RUNNING;
bool HEAT = false;
bool COOL = false;
bool FIRE = false;

const float lat = 36.36;
const float lgn = 25.25;

/**
 * general textual bool values to use later on ;D
 */
const bool ON = true;
const bool OFF = false;
const bool NO = false;
const bool YES = true;


/*===========================================*/

short int Light_threshold = 250; // Less => night, more => day

// Host for periodic data report
  /** DEFAULTS **/
String TARGET_IP = "192.168.101.5";
int TARGET_PORT = 1880;
int TARGET_REFRESH = 5000;
  /** VARS **/
String target_ip = TARGET_IP;
int target_port = TARGET_PORT;
int target_refresh = TARGET_REFRESH; // post broadcast intervall

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String get_t(){
  return String(t);
}

/*====== Process configuration ==============*/

String processor(const String& var){
  /* Replaces "placeholder" in  html file with sensors values */
  /* accessors functions get_... are in sensors.ino file   */ 
  if(var == "TEMPERATURE"){
    return get_t();
    /* On aimerait écrire : return get_temperature(TempSensor);
     * mais c'est un exemple de ce qu'il ne faut surtout pas écrire ! 
     * yield + async => core dump ! 
     */
    //return get_temperature(TempSensor);
  }
  else if(var == "LIGHT"){
    return get_light(LightPin);
  }

  else if(var == "UPTIME") {
    return getUptime();
  }
  else if(var == "WHERE") {
    return LOCATION;
  }
  else if(var == "SSID") {
    return getSSID();
  }
  else if(var == "MAC") {
    return getMAC();
  }
  else if(var == "IP") {
    return getIP();
  }
  else if(var == "COOLER") {
    return get_cooler_str(COOL);
  }
  else if(var == "HEATER") {
    return get_heater_str(HEAT);
  }
  return String();
}
 
void setup_http_server() {
  /* 
   * Sets up AsyncWebServer and routes 
   */
  // doc => Serve files in directory "/" when request url starts with "/"
  // Request to the root or none existing files will try to server the default
  // file name "index.htm" if exists 
  // => premier param la route et second param le repertoire servi (dans le SPIFFS) 
  // Sert donc les fichiers css !  
  server.serveStatic("/", SPIFFS, "/").setTemplateProcessor(processor);  
  
  // Declaring root handler, and action to be taken when root is requested
  auto root_handler = server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        /* This handler will download index.html (stored as SPIFFS file) and will send it back */
        request->send(SPIFFS, "/index.html", String(), false, processor); 
        // cf "Respond with content coming from a File containing templates" section in manual !
        // https://github.com/me-no-dev/ESPAsyncWebServer
        // request->send_P(200, "text/html", page_html, processor); // if page_html was a string .   
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    /* The most simple route => hope a response with temperature value */ 
    /* Exemple de ce qu'il ne faut surtout pas écrire car yield + async => core dump !*/
    //request->send_P(200, "text/plain", get_temperature(TempSensor).c_str());
    
    request->send_P(200, "text/plain", get_t().c_str());
  });

  server.on("/light", HTTP_GET, [](AsyncWebServerRequest *request){
    /* The most simple route => hope a response with light value */ 
    request->send_P(200, "text/plain", get_light(LightPin).c_str());
  });

  // This route allows users to change thresholds values through GET params
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    /* A route with a side effect : this get request has a param and should     
     *  set a new light_threshold ... used for regulation !
     */
      if (request->hasArg("light_threshold")) { // request may have arguments
  Light_threshold = atoi(request->arg("light_threshold").c_str());
  request->send_P(200, "text/plain", "Threshold Set !");
      }
    });

  /**
   * FOR THE updating NODERED DASHBOARD manually :D
   */
  server.on("/esp", HTTP_GET, [](AsyncWebServerRequest *request){      
    request->send(200, "application/json", getJSONString_fromstatus(float(t),int(l)).c_str() );
  });
  
  server.on("/target", HTTP_POST, [](AsyncWebServerRequest *request){
    /* A route receiving a POST request with Internet coordinates 
     *  of the reporting target host.
     */
     Serial.println("Receive Request for a ""target"" route !"); 
        if (request->hasArg("ip") &&
        request->hasArg("port") &&
        request->hasArg("sp")) {
          target_ip = request->arg("ip");
          target_port = atoi(request->arg("port").c_str());
          target_refresh = atoi(request->arg("sp").c_str());
        }
//        if(target_ip.trim().equals("")) {
//          target_ip == TARGET_IP;
//        }
//        if(target_port.trim().equals("")){
//          target_port = TARGET_PORT;
//        }
//        if(target_refresh.trim().equals("")) {
//          target_refresh = TARGET_REFRESH;
//        }
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    /**
     * build a json response depending on the passed in arguments 
     */
    server.on("/value", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Requesting build of status JSON..."); 
      AsyncJsonResponse * response = new AsyncJsonResponse();
      response->addHeader("Server","ESP Async Web Server");
      JsonObject& jsondoc = (JsonObject&) response->getRoot();
      
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(request->hasParam("temperature")){
          jsondoc["status"]["temperature"] = t; // Temp value
        } 
        if(request->hasParam("light")){
          jsondoc["status"]["light"] = l;
        } 
        if(request->hasParam("target_ip")){
          jsondoc["reporthost"]["target_ip"] = target_ip;
        } 
        if(request->hasParam("target_port")){
          jsondoc["reporthost"]["target_port"] = target_port;
        } 
        if(request->hasParam("target_sp")){
          jsondoc["reporthost"]["target_sp"] = target_sp;
        } 
        if(request->hasParam("uptime")){
          jsondoc["info"]["uptime"] = getUptime();
        } 
        if(request->hasParam("ssid")){
          jsondoc["info"]["ssid"] = getSSID();
        } 
        if(request->hasParam("mac")){
          jsondoc["info"]["mac"] = getMAC();
        } 
        if(request->hasParam("ip")){
          jsondoc["info"]["ip"] = getIP();
        } 
        if(request->hasParam("loc")){
          jsondoc["info"]["loc"] = LOCATION;
        } 
  
        response->setLength();
        request->send(response);
        
        request->send(200, "json/application", getJSONString_fromstatus(float(t),int(l)).c_str() );
    });
    
  // If request doesn't match any route, returns 404.
  server.onNotFound([](AsyncWebServerRequest *request){
      request->send(404);
  });


    
  

  // Start server
  server.begin();
}

String getUptime() {
  uptime::calculateUptime();
  UPTIME = String(uptime::getMinutes()) +"m :  " + String(uptime::getSeconds()) + "s ";
  return UPTIME;
}

String getJSONString_fromlocation(float lat, float lgn){
  StaticJsonDocument<1000> jsondoc;
  jsondoc["lat"] = lat;
  jsondoc["lgn"] = lgn;
  String data = "";
  serializeJson(jsondoc, data);
  return data;
}

int getFire(){
  return FIRE;
}

bool switch_cooler(bool cooler_on){
  if (cooler_on){
    digitalWrite(CoolerLED, HIGH);
    COOL = ON;
  } else {
    digitalWrite(CoolerLED, LOW);
    COOL = OFF;
  }
  return COOL;
}


bool switch_heater(bool heater_on) {
  if (heater_on){
    digitalWrite(HeaterLED, HIGH);
    HEAT = ON;
  } else {
    digitalWrite(HeaterLED, LOW);
    HEAT = OFF;
  }
  return HEAT;
}
  
/*---- Arduino IDE paradigm : setup+loop -----*/
void setup(){
  Serial.begin(9600);
  while (!Serial); // wait for a serial connection. Needed for native USB port only
  /* WiFi connection  -----------------------*/
//  String hostname = "Detecteur 9000!";
  
  // Default Credentials 
  String ssid = String("_FAYSSAL");
  String passwd = String("qwertyui");
  bool connection_established = 0;
  wifi_connect_basic(hostname, ssid, passwd); // debug connect WIFI
//  connection_established = wifi_connect_multi(hostname); // reboot bug :(
//  while(!connection_established); // wait to connect to go to next line (to avoid reboot bug)

  
  // Initialize the LED 
//  setup_led(LEDpin, OUTPUT, LOW);
  setup_led(CoolerLED, OUTPUT, LOW);
  setup_led(HeaterLED, OUTPUT, LOW);
  setup_led(FireLED, OUTPUT, LOW);


  LOCATION = "" + String(lat) + ", " + String(lgn) + "";
  // Init temperature sensor 
  tempSensor.begin();

  // status constants
  RUNNING = true;

  /* Install OTA --------------------------*/
//  setupOTA("Fayssal_ESP32"); // TODO: ... CONST CHAR

    // Initialize SPIFFS
  SPIFFS.begin(true);
  
  setup_http_server();
}


void loop(){  
  // update current location 
  // lat = getLat()
  // lgn = getLgn()
  // LOCATION = getJSONString_fromlocation(lat, lgn);
  
  t = get_temperature_float(tempSensor);
  l = get_light_int(LightSensor);

/**
 * regulate the temperature to stay between 
 * the min and max at all times
 */
  if (t > MIN_TEMP and t < MAX_TEMP){
    switch_heater(OFF);
    switch_cooler(OFF);
  }
  else if(t >= MAX_TEMP) {
    switch_heater(OFF);
    switch_cooler(ON);
  } else {
    switch_heater(ON);
    switch_cooler(OFF);
  }

  String serverPath = "http://" + target_ip + ":" + target_port + "/nodered";
  if ((millis() - lastTime) > target_refresh) {    
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverPath.c_str());
    http.addHeader("Content-Type", "application/json");
    String payload = getJSONString_fromstatus(float(t),int(l)).c_str();
    Serial.println("sending post to" + serverPath + " :" + payload);
    int httpResponseCode = http.POST(payload);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
    lastTime = millis();
  }
  
}