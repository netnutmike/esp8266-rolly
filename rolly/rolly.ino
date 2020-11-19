/******************  LIBRARY SECTION *************************************/

#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <SimpleTimer.h>          //https://github.com/thehookup/Simple-Timer-Library
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <ESP8266WiFi.h>          //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>          //if you get an error here you need to install the ESP8266 board manager 
#include <ArduinoOTA.h>           //ArduinoOTA is now included with the ArduinoIDE
#include <ArduinoJson.h>          //ArduinoJson

/*****************  START USER CONFIG SECTION *********************************/

#define USER_SSID                 "hahn-2g"
#define USER_PASSWORD             "11ab3f4ef3"
#define USER_MQTT_SERVER          "10.232.0.204"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "mqtt"
#define USER_MQTT_PASSWORD        "mqtt"
#define USER_MQTT_CLIENT_NAME     "rolly1"           //used to define MQTT topics, MQTT Client ID, and ArduinoOTA
#define PIN 6                                        //pin where the led matrix is connected


/*****************  END USER CONFIG SECTION *********************************/

/***********************  WIFI AND MQTT SETUP *****************************/
/***********************  DON'T CHANGE THIS INFO *****************************/

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME ; 

/*****************  DEFINE JSON SIZE *************************************/

const int capacity = JSON_OBJECT_SIZE(3) + 2 * JSON_OBJECT_SIZE(1);
StaticJsonDocument<capacity> doc;

// Define the Matrix details
#define mw 44
#define mh 11
#define NUMMATRIX (mw*mh)

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, 44, 11, 
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
    NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG + 
    NEO_TILE_TOP + NEO_TILE_RIGHT +  NEO_TILE_ZIGZAG);
    

const uint16_t colors[] = {
  matrix->Color(255, 0, 0), matrix->Color(0, 255, 0), matrix->Color(0, 0, 255) };

/*****************  DECLARATIONS  ****************************************/
WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;


/*****************  GENERAL VARIABLES  *************************************/

bool boot = true;
char charPayload[50];
int x    = mw;
int pass = 0;
String currentMessage;
String currentCommand;

void setup_wifi() 
{
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname(USER_MQTT_CLIENT_NAME);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  // Loop until we're reconnected
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
        Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        if(boot == true)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Rebooted");
          boot = false;
        }
        if(boot == false)
        {
          client.publish(USER_MQTT_CLIENT_NAME"/checkIn","Reconnected"); 
        }
        client.subscribe(USER_MQTT_CLIENT_NAME"/message");
        client.subscribe(USER_MQTT_CLIENT_NAME"/command");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    else
    {
      ESP.restart();
    }
  }
}

/************************** MQTT CALLBACK ***********************/



void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  String stateMessage;
  
  newPayload.toCharArray(charPayload, newPayload.length() + 1); 
  
  if (newTopic == USER_MQTT_CLIENT_NAME"/command") 
  {

    client.publish(USER_MQTT_CLIENT_NAME"/status", charPayload);
  }
  if (newTopic == USER_MQTT_CLIENT_NAME"/message")
  {
    DeserializationError err = deserializeJson(doc, payload);

    if (err)
    {
      stateMessage = "Deserialization Error: " + String((char *)err.c_str());
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/status", charPayload);
    } else {
      String newPayloadMessage = doc["Message"];
      currentMessage = newPayloadMessage; 
      matrix->setCursor(x, 0);
      matrix->print(newPayloadMessage);
    }
  }
}



void setup() 
{
  Serial.begin(115200);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.mode(WIFI_STA);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  timer.setInterval(10000);
  FastLED.addLeds<NEOPIXEL,PIN>(matrixleds, NUMMATRIX); 
  //FastLED.addLeds<WS2811_PORTA,6>(matrixleds, NUMMATRIX/3).setCorrection(TypicalLEDStrip);
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(40);
  matrix->setTextColor(colors[0]);
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  //timer.run();

  if(boot == false)
  {
    matrix->fillScreen(0);
    if (currentMessage != NULL)
    {
      matrix->setCursor(x, 0);
      matrix->print(currentMessage);
      if(--x < -36) {
        x = matrix->width();
        if(++pass >= 3) pass = 0;
        matrix->setTextColor(colors[pass]);
      }
    }
    matrix->show();

    if (timer.isReady())
    {
      String stateMessage = "Alive";
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/state", charPayload);

      
      stateMessage = WiFi.localIP().toString();
      stateMessage.toCharArray(charPayload, stateMessage.length() + 1);
      client.publish(USER_MQTT_CLIENT_NAME"/ipaddress", charPayload);

      timer.reset();
    }
    
    delay(100);
  }
}
