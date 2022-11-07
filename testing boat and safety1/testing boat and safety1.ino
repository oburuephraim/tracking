#include <TinyGPS++.h>


#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 256


#include <TinyGsmClient.h> //https://github.com/vshymanskyy/TinyGSM
#include <ArduinoHttpClient.h> //https://github.com/arduino-libraries/ArduinoHttpClient

#include <SoftwareSerial.h>



#define rxPinSim800 7
#define txPinSim800 8

// Create a TinyGPS++ object
TinyGPSPlus gps;



//*************************************************************
SoftwareSerial sim800(txPinSim800, rxPinSim800);

const char FIREBASE_HOST[]  = "boat-safety-tracking-default-rtdb.firebaseio.com";//remove https://
const String FIREBASE_AUTH  = "v5namsSNNLJtewqLq9OM4HhSkflbbjKrgxdUUQrj";//from firebase project seetings>service accounts>database secrets
const String FIREBASE_PATH  = "/data";
const int SSL_PORT          = 443;

char apn[]  = "internet";//airtel ->"internet"
char user[] = "";
char pass[] = "";


TinyGsm modem(sim800);

TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);

unsigned long previousMillis = 0;


void setup()
{
  Serial.begin(9600);

  Serial.println(F("device serial initialize"));

  sim800.begin(9600);
  Serial.println(F("SIM800L serial initialize"));

  Serial.println(F("Initializing modem..."));
  modem.restart();

  String modemInfo = modem.getModemInfo();

  Serial.print(F("Modem: "));
  Serial.println(modemInfo);
  Serial.print("IMEI: ");
  
  sim800.println("AT+CGSN");
delay (500);
  
  
  
  //delay(500);
  while(sim800.available()) 
  {
    Serial.println(sim800.read());//Forward what Software Serial received to Serial Port
  }  
    

  http_client.setHttpResponseTimeout(5 * 1000); //^0 secs timeout
}


void loop()//new concept
{ //first ensure proper gps connenctivity and obtain latitude and longitude
  //obtain other sensor data values
  //if valid lata and long and other sensor values, check if connected to the internet
  //if connected to the internet post data to the database

  String Data = ""; //clear all sensor values
  
  while (Serial.available() > 0) {
    if (gps.encode(Serial.read())) {
      
      String Data1 = "";
      Data1 =  displayInfo();
      Serial.flush();

      if (!(Data1 == "")) {

        Data += Data1;
        break;

      }

    }
  }
  // If 5000 milliseconds pass and there are no characters coming in
  // over the software serial port, show a "No GPS detected" error
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected"));
    while (true);
  }

  //get another sensor value

  if (!(Data == "")) { //if all sensor values are obtained
    
    int thispost = 0;

    while (thispost == 0) {
      Serial.print(F("Connecting to "));
      Serial.print(apn);
      if (!modem.gprsConnect(apn, user, pass))
      {
        Serial.println(F(" fail"));

        //delay(1000);
        return;
      }
      Serial.println(F(" OK"));

      http_client.connect(FIREBASE_HOST, SSL_PORT);
      while (true) {
        if (!http_client.connected())
        {
          Serial.println();
          http_client.stop();// Shutdown
          Serial.println(F("HTTP  not connect"));
          break;
        }
        else
        {
          
          PostToFirebase("PATCH", FIREBASE_PATH, Data, &http_client);
          Data ="";
          thispost = 1;
          break;
        }

      }
    }
  }

}




String displayInfo()
{
  int loc = 1;
  String Data = "";
  Data = "{";  
  
  if (gps.location.isValid())
  {
    Serial.print(F("Latitude: "));
    Serial.println(gps.location.lat(), 6);
    Serial.print(F("Longitude: "));
    Serial.println(gps.location.lng(), 6);
    Serial.print(F("Altitude: "));
    Serial.println(gps.altitude.meters());
    Serial.println(F("Speed:"));
    Serial.println(gps.speed.kmph()); 
    Serial.println(F("satellites:"));
    Serial.println(gps.satellites.value());    
    
    Data += "\"lat\":\"";
    Data += String(gps.location.lat(), 6);
    Data += "\",";
    Data += "\"lng\":\"";
    Data += String(gps.location.lng(), 6);
    Data += "\",";
    Data += "\"alt\":\"";
    Data += gps.altitude.meters();
    Data += "\",";
    Data += "\"speed\":\"";
    Data += gps.speed.kmph();
    Data += "\",";        
    Data += "\"satelites\":\"";
    Data += gps.satellites.value();
    Data += "\","; 
    

  }
  else
  {
    Serial.println(F("Location: Not Available"));    
    Data += "\"lat\":\"";
    Data += String(gps.location.lat(), 6);
    Data += "\",";
    Data += "\"lng\":\"";
    Data += String(gps.location.lng(), 6);
    Data += "\",";
    Data += "\"alt\":\"";
    Data += gps.altitude.meters();
    Data += "\",";
    Data += "\"speed\":\"";
    Data += gps.speed.kmph();
    Data += "\",";        
    Data += "\"satelites\":\"";
    Data += gps.satellites.value();
    Data += "\","; 
    loc = 0;
  }

  Serial.print(F("Date: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.println(gps.date.year());
    Data += "\"date\":\"";
    Data += gps.date.month();
    Data += "/";
    Data += gps.date.day();
    Data += "/";
    Data += gps.date.year();
    Data += "\",";

  }
  else
  {
    Serial.println(F("Not Available"));
    Data += "\"date\":\"";
    Data += gps.date.month();
    Data += "/";
    Data += gps.date.day();
    Data += "/";
    Data += gps.date.year();
    Data += "\",";    
  }

  Serial.print(F("Time: "));
  if (gps.time.isValid())
  {
    
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour() );
    Serial.print(F(":"));
    
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
        
    
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
    Data += "\"time\":\"";    
    Data += gps.time.hour() + 3;
    Data += ":";
    Data += gps.time.minute();
    Data += ":";    Data += gps.time.second();
    Data += ".";    
    Data += gps.time.centisecond();
    Data += "\"}";





  }
  else
  {
    Serial.println(F("Not Available"));
    Data += "\"time\":\"";    
    Data += gps.time.hour() + 3;
    Data += ":";
    Data += gps.time.minute();
    Data += ":";    Data += gps.time.second();
    Data += ".";    
    Data += gps.time.centisecond();
    Data += "\"}";    
  }

  

  Serial.println();
  Serial.println();
  //delay(1000);
  //if (loc == 1 ) {
    
    return Data;
 // } else {
    //Data = "";
    //return Data;
  //}
}



void PostToFirebase(const char* method, const String & path , const String & data, HttpClient* http)
{
  String response;
  int statusCode = 0;
  http->connectionKeepAlive(); // Currently, this is needed for HTTPS

  String url;
  if (path[0] != '/')
  {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=" + FIREBASE_AUTH;
  Serial.print(F("POST:"));
  Serial.println(url);
  Serial.print(F("Data:"));
  Serial.println(data);

  String contentType = "application/json";
  http->put(url, contentType, data);//updates the record of data in the database
  //http->post(url, contentType, data);//creats a new instance of the data in the databse

  statusCode = http->responseStatusCode();
  Serial.print(F("Status code: "));
  Serial.println(statusCode);

  response = http->responseBody();
  Serial.print(F("Response: "));
  Serial.println(response);

  if (!http->connected())
  {
    Serial.println();
    http->stop();// Shutdown
    Serial.println(F("HTTP POST disconnected"));
  }

}


