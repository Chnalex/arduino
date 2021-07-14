
///////////////////////////////
////  Alexandre OGER 2019  ////
///////////////////////////////


// capteur exterieur lumiere et temperature
// base sur un ESP12E (nodemcu form factor)
// un capteur i2C BH1750 pour la lumiere et un DS18S20 pour la temperaure
// l'ESP12E fourni la connectivite wifi et permet de faire tourner un serveur http sommaire 
// le serveur retourne à la demande les valeurs de lumiere et de temprature lues
//                                               __________________
//                                           --AO--|    TI_IT_TI    |--D0--
//                                           --RSV-|    |_____      |--D1-- SCL --
//                                           --RSV-|    |           |--D2-- SDA __|--port I2C pour le capteur lux BH1750
//                                           --SD3-|   ----------   |--D3-- onewire --capteur temp DS18B20
//                                           --SD2-|   |        |   |--D4--
//                                           --SD1-|   |        |   |--3V3-
//                                           --CMD-|   |        |   |--GND-
//                                           --SDO-|   |        |   |--D5-- 
//                                           --CLK-|   ----------   |--D6--
//                                           --GND-|                |--D7--
//                                           --3V3-|                |--D8--
//                                           --EN--|  ESP8266       |--RX--
//                                           --RST-|  ESP-12E       |--TX--
//                                           --GND-|     _____      |--GND-  
//                                           --VIN-|    [ USB ]     |--3V3-
//                                                 ------|___|-------
// 
 
//
//pinout cable cat3:
//D1 (vert/blanc)/D2 (vert) I2C
//D3 (bleu) onwire (temp)

// 3.3V ou 5V -V orange/blanc et marron
// 0V -> orange et marron/blanc

 
 
 //I2C
 #include <Wire.h> 


// bibliotheque wifi
#include <ESP8266WiFi.h>

//lux
#include <BH1750.h>
BH1750 lightMeter;


//appel librairie onewire pour communiquer avec les sondes de temperature
#include <OneWire.h>
#include <DallasTemperature.h>
OneWire temp(D3);
DallasTemperature DS18B20(&temp);
float  temperatureCString;
int  lux;


// SSID et pass
const char* ssid     = "xxxxx";
const char* password = "xxxxx";

// serveur web sur le port 80
WiFiServer server(80);

// Variable pour stocker la requete http
String header;







void setup() {

  Serial.begin(115200);
  delay(200);



  Wire.begin();
  DS18B20.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  WiFi.mode(WIFI_STA);
  WiFi.hostname("capttemplux");
  WiFi.begin(ssid, password);
  delay(5000);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin("iPhone", "totototo");
    digitalWrite(LED_BUILTIN, LOW);
    delay(5000);
    if (WiFi.status() != WL_CONNECTED) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  Serial.println("");
  Serial.println(WiFi.localIP());


  server.begin();

 

}

void loop(){

 

      lux = lightMeter.readLightLevel();
      getTemperature();    
      Serial.print("Lumiere: ");     
      Serial.println(lux);
      Serial.print("Temp: ");     
      Serial.println(temperatureCString);
      delay(1000);
  
 


  WiFiClient client = server.available();   
  delay(10);
  if (client) {                             
    Serial.println("Nv client");          
    String currentLine = "";     
    //header="                                ";           
    while (client.connected()) { 
      
      if (client.available()) {             
        char c = client.read();            
        Serial.write(c);                    
        header += c;
        if (c == '\n') {                    

          if (currentLine.length() == 0) {

            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-type:text/html"));
            client.println(F("Connection: close"));
            client.println();          
            client.println(F("<!DOCTYPE html><html>"));
            client.println(F("<head></head>"));
            client.println(F("<html><body>"));
            
            client.print(lux);
            client.print("|" );
            client.println(temperatureCString);
 

            client.println(F("</body></html>"));
            client.println();
            break;
          } else { 
            currentLine = "";
          }
        } else if (c != '\r') {  
          currentLine += c;      
        }
      }
    }

    header = "";

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
 

  
}



void getTemperature() {
  float tempC;
  float tempF;
  do {
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0);
    temperatureCString=tempC;
    //tempF = DS18B20.getTempFByIndex(0);
    //dtostrf(tempF, 3, 2, temperatureFString);
    delay(100);
  } while (tempC == 85.0 || tempC == (-127.0));
}
