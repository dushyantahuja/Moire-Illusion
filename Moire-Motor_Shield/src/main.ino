#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "FastLED.h"
#include "ESP8266TrueRandom.h"

#define NUM_LEDS 26
#define DATA_PIN 32
#define UPDATES_PER_SECOND 60
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#define MQTT_MAX_PACKET_SIZE 256

#define MAXSPEED 1023
#define MINSPEED 512

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

CRGB leds[NUM_LEDS],bg;
void callback(const MQTT::Publish& pub);

//WiFi variables............................................................
const char ssid[] = "DUSHYANT";        //your network SSID (name)
const char password[] = "ahuja987";       // your network password

WiFiClient espClient;
IPAddress server(192, 168, 1, 232);
PubSubClient client(espClient, server);

long lastMsg = 0;
char msg[50];
int value = 0;

boolean start = 1;

#define PWMA 5
#define DIRA 0
#define PWMB 4
#define DIRB 2


int previousspeed1 = 0, speed1 = MAXSPEED, previousspeed2 = 0, speed2 = MAXSPEED;


void setup() {
  // Serial.begin(115200);
  delay(3000);
  pinMode(PWMA, OUTPUT); //Speed Motor 1
  pinMode(DIRA, OUTPUT); //Direction Motor 1
  pinMode(PWMB, OUTPUT); //Speed Motor 2
  pinMode(DIRB, OUTPUT); //Direction Motor 2

  //pinMode(2, OUTPUT); //Built-in LED

  // Stop motors while starting the ESP

  digitalWrite(PWMA,LOW);
  digitalWrite(DIRA,LOW);
  digitalWrite(PWMB,LOW);
  digitalWrite(DIRB,HIGH);


  ArduinoOTA.setHostname("Moire");
  ArduinoOTA.setPassword((const char *)"avin");

  //FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  //FastLED.setDither( 0 );
  //fill_solid(leds, NUM_LEDS, bg);
  //LEDS.setBrightness(constrain(128,10,255));

  setup_wifi();
  client.set_callback(callback);
  analogWriteFreq(200);
  start_motor();
}

void loop() {
  ArduinoOTA.handle();
  //EVERY_N_SECONDS(180){
    if (WiFi.status() != WL_CONNECTED)
      setup_wifi();
      if (!client.connected()) {
        reconnect();
      }
  //  }
  client.loop();
  EVERY_N_SECONDS(60){
    speed1+= random(-5,5); //if(speed1 >MAXSPEED) speed1 = MAXSPEED;if(speed1<MINSPEED) speed1 = MINSPEED;
    speed2+= random(-5,5); //if(speed2 >MAXSPEED) speed2 = MAXSPEED;if(speed2<MINSPEED) speed2 = MINSPEED;
    start_motor();
  }
}

void callback(const MQTT::Publish& pub) {
 if(pub.topic() == "moire1/command"){
    String payload = pub.payload_string();
    if(payload == "start"){
      client.publish("moire1/status","Moire Starting");
      start_motor();
    }
    else if(payload == "stop"){
      stop_motor();
      client.publish("moire1/status","Moire Stopping");
    }
    else if(payload == "up"){
      speed1 += 5;
      speed2 += 5;
      start_motor();
      client.publish("moire1/status","Increasing Speed: " + String(speed1));
    }
    else if(payload == "down"){
      speed1 -= 5;
      speed2 -= 5;
      start_motor();
      client.publish("moire1/status","Decreasing Speed: " + String(speed1));
    }
    /*else if (message.equals("next")) {
      gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
      gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
      client.publish("moire1/status","Next Pattern");
    }*/
  }
}

/* Motor Start and Stop Code */

void start_motor(){
  speed1 = (speed1>MAXSPEED ? MAXSPEED: speed1);
  speed1 = (speed1<MINSPEED ? MINSPEED: speed1);
  speed2 = (speed2>MAXSPEED ? MAXSPEED: speed2);
  speed2 = (speed2<MINSPEED ? MINSPEED: speed2);

  if(previousspeed1 <=100) {
    digitalWrite(PWMA,HIGH);
    delay(10);
  }
  if(previousspeed2 <=100) {
    digitalWrite(PWMB,HIGH);
    delay(10);
  }
  analogWrite(PWMA,speed1);
  analogWrite(PWMB,speed2);

  //digitalWrite(PWMA, 1);
  //digitalWrite(PWMB, 1);

  /*if(speed1>=previousspeed1){
    digitalWrite(DIRA,HIGH);
    for(int i = previousspeed1; i<=speed1; i+=5){
      analogWrite(PWMA,i);
      delay(500);
      yield();
    }
  } else {
    digitalWrite(DIRA,LOW);
    for(int i = previousspeed1; i>=speed1; i-=5){
      analogWrite(PWMA,i);
      delay(500);
      yield();
    }
  }
  if(speed2>=previousspeed2){
    digitalWrite(DIRB,HIGH);
    for(int i = previousspeed2; i<=speed2; i+=5){
      analogWrite(PWMB,i);
      delay(500);
      yield();
    }
  } else {
    digitalWrite(DIRB,LOW);
    for(int i = previousspeed2; i>=speed2; i-=5){
      analogWrite(PWMB,i);
      delay(500);
      yield();
    }
  }
  //digitalWrite(PWMA, HIGH);*/
  previousspeed1 = speed1;
  previousspeed2 = speed2;
}

void stop_motor(){
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  previousspeed1 = 0;
  previousspeed2 = 0;
}




/* --------------- WiFi Code ------------------ */


void setup_wifi() {
  //delay(10);
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    //delay(5000);
    //ESP.restart();
  }
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}


void reconnect() {
  // Loop until we're reconnected
  // while (!client.connected()) {
    if (client.connect("Moire1")) {
      // Once connected, publish an announcement...
      client.publish("moire1/status", "Alive - subscribing to moire1/command & moire1/speed, publishing to moire1/status");
      // ... and resubscribe
      client.subscribe("moire1/command");
      client.subscribe("moire1/speed");
    } else {
      // Wait 5 seconds before retrying
      //delay(500);
    }
    //digitalWrite(2, HIGH);
  // }
}
