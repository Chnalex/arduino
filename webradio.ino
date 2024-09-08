/*

/////////////////////////////////
////  Alexandre OGER 2024    ////
/////////////////////////////////

Webradio:
- Wifi
- sortie I2S vers DAC 
- affichieur VFD 256x50 pixels (GP1287AI)
- telecommande infrarouge et/ou encodeur rotatif pour changer preselection ou gerer le volume
- vumetre
- 10 preselections paramétrables par serveur web
- SSID et PASSWD paramétrables par serveur web
- Si SSID non-joignable le webradio boot en mode AP (ssid: webradio) avec les sefgveru web actif



                          ___________________
                         |           __ __  |
                         |  __    __|  |  | |
                         | |  |  |  |  |  | |
                         | |  |__|  |  |  | |
                      ___|  ______________  |___ 
                  3V3| [ ] |              | [ ] |GND 
                  3V3| [ ] |   ESP32S3    | [ ] |TX 
                  RST| [ ] |   DEV KIT    | [ ] |RX
                    4| [ ] |              | [ ] |1
                    5| [ ] |              | [ ] |2
                    6| [ ] |              | [ ] |42 -- pin A |
                    7| [ ] |______________| [ ] |41 -- pin B | Encodeur rotatif 
      |PIN_MOSI -- 15| [ ]                  [ ] |40 -- BP    |
Affich| *PIN_DC -- 16| [ ]                  [ ] |39 
VFD   | PIN_SCK -- 17| [ ]                  [ ] |38
Fluo  |  PIN_CS -- 18| [ ]                  [ ] |37 
      | PIN_RST --  8| [ ]    __            [ ] |36
                    3| [ ]   |OO|           [ ] |35
                   46| [ ]   |__|           [ ] |0  
                    9| [ ]   RGB(Pin48)     [ ] |45
   Recepteur IR -- 10| [ ]                  [ ] |48
                   11| [ ]                  [ ] |47
Sortie| I2S_LRC -- 12| [ ]                  [ ] |21
Audio |I2S_BCLK -- 13| [ ]                  [ ] |20
 I2S  |I2S_DOUT -- 14| [ ] [O]          [O] [ ] |19
                   5V| [ ] BOOT         RST [ ] |GND
                  GND| [ ]                  [ ] |GND  
                     |     _____      __X__     |
                     | O  | USB |    | USB |  O |
                     +--------------------------+


*/

#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "esp32-rmt-ir.h"
#include <Preferences.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "AiEsp32RotaryEncoder.h"



#define I2S_DOUT      14
#define I2S_BCLK      13
#define I2S_LRC       12
#define IR_RECEIVER_PIN 10

#define PIN_MOSI 15
//#define PIN_DC 16
#define PIN_SCK 17
#define PIN_CS 18
#define PIN_RST 8


//codes IR touches 
#define CODE_IR_VOLUME_HAUT    0x807f18e7
#define CODE_IR_VOLUME_BAS     0x807f08f7
#define CODE_IR_VOLUME_MUTE    0x807f827d
#define CODE_IR_OK             0x807fc837
#define CODE_IR_HAUT           0x807f6897
#define CODE_IR_BAS            0x807f58a7
#define CODE_IR_GAUCHE         0x807f8a75
#define CODE_IR_DROITE         0x807f0af5
//codes IR pavé numérique 0 -> 9
const uint32_t CODE_IR_[10] = {0x807f807f,0x807f728d,0x807fb04f,0x807f30cf,0x807f52ad,0x807f906f,0x807f10ef,0x807f629d,0x807fa05f,0x807f20df};

#define ROTARY_ENCODER_A_PIN 42
#define ROTARY_ENCODER_B_PIN 41
#define ROTARY_ENCODER_BUTTON_PIN 40
#define ROTARY_ENCODER_VCC_PIN -1 //-1 = vcc directement sur 3.3v
#define ROTARY_ENCODER_STEPS 4


AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
Audio audio;
Preferences preferences;

WebServer server(80);

U8G2_GP1287AI_256X50_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ PIN_SCK, /* data=*/  PIN_MOSI, /* cs=*/  PIN_CS, /* dc=*/ U8X8_PIN_NONE,  /* reset=*/ PIN_RST ); 



String chaine_ir="";
String ptr = "";
String webradio_info="";
String webradio_streamtitle="";
String webradio_bitrate="";
String url_eeprom="";
String urlcourt_eeprom="";
   
int change_info=1;
int chaine_ou_volume=0;
unsigned long int code_ir = 0;
int code_recu=0;
uint16_t vu_metre=0;
int peakG=0;
int peakD=0;
int volume_courant=15;
int nbr_url_courante=-1;
int nbr_url=0;
int etat_mute=0;
int trigger_reboot=0;
int position=0;
int position_anterieure=0;
unsigned long millis_aff=0;
unsigned long millis_vu=0;
unsigned long millis_peakG=0;
unsigned long millis_peakD=0;
unsigned long millis_wifi=0;
unsigned long millis_reboot=0;
uint16_t timeout_ms = 500;
uint16_t timeout_ms_ssl = 4000;

void IRAM_ATTR readEncoderISR(){
    rotaryEncoder.readEncoder_ISR();
}

void setup(){

    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n");

    u8g2.begin();
    u8g2.setContrast(70);
    u8g2.setDisplayRotation(U8G2_R0);


    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.disableAcceleration();
    //rotaryEncoder.setBoundaries(0, 9, true); 
    rotaryEncoder.setEncoderValue(100);

    chaine_ir.reserve(50);
    webradio_info.reserve(300);
    webradio_streamtitle.reserve(50);
    webradio_bitrate.reserve(10);
    ptr.reserve(3000);
    url_eeprom.reserve(6);
    urlcourt_eeprom.reserve(6);

    Serial.print("Memoire dispo: ");
    Serial.println(ESP.getFreeHeap());

    preferences.begin("urls", false);
    /*
    A decommenter pour le 1er boot puis recommenter ensuite
    preferences.putString("URL0", "http://icecast.radiofrance.fr/franceinter-hifi.aac");
    preferences.putString("URL1", "http://icecast.radiofrance.fr/fip-hifi.aac");
    preferences.putString("URL2", "http://tsfjazz.ice.infomaniak.ch/tsfjazz-high.mp3");
    preferences.putString("URL3", "http://icecast.radiofrance.fr/franceculture-hifi.aac");
    preferences.putString("URL4", "http://radionova.ice.infomaniak.ch/radionova-256.aac");
    preferences.putString("URL5", "http://start-latina.ice.infomaniak.ch/start-latina-high.mp3");
    preferences.putString("URL6", "http://icecast.radiofrance.fr/francemusique-hifi.aac");
    preferences.putString("URL7", "http://icecast.radiofrance.fr/mouv-hifi.aac");
    preferences.putString("URL8", "http://icecast.radiofrance.fr/franceinfo-hifi.aac");
    preferences.putString("URL9", "https://ice2.somafm.com/defcon-256-mp3" );
    
    preferences.putString("URL0_court", "France INTER");
    preferences.putString("URL1_court", "FIP");
    preferences.putString("URL2_court", "TSF Jazz");
    preferences.putString("URL3_court", "France Culture");
    preferences.putString("URL4_court", "Nova");
    preferences.putString("URL5_court", "Latina");
    preferences.putString("URL6_court", "France Musique");
    preferences.putString("URL7_court", "Le Mouv");
    preferences.putString("URL8_court", "France INFO");
    preferences.putString("URL9_court", "SomaFM" );

    preferences.putString("ssid", "******" );
    preferences.putString("password", "********" );
    */

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);  
    audio.setAudioTaskCore(0);
    audio.setVolume(volume_courant); // default 0...21
    audio.setConnectionTimeout(timeout_ms, timeout_ms_ssl);

    WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(preferences.getString("ssid", "").c_str(), preferences.getString("password", "").c_str());
    Serial.print("\nConnecting: ");
    Serial.print(preferences.getString("ssid", "").c_str());
    Serial.print(" -> ");
    Serial.println(preferences.getString("password", "").c_str());
    

    millis_wifi=millis();
    while(WiFi.status() != WL_CONNECTED  ){
        Serial.print(".");
        delay(300);
        if(millis()-millis_wifi> 10000){
          break;
        }
    }
    if (WiFi.status() != WL_CONNECTED ){
      WiFi.mode(WIFI_AP);
      WiFi.softAP("webradio", NULL);
      Serial.println("");
      Serial.println("impossible de se connecter, changement de mode");
      Serial.print("[+] AP crée, IP: ");
      Serial.println(WiFi.softAPIP());
    } else{
      Serial.println("\nConnecté au SSID");
      Serial.print("IP Locale: ");
      Serial.println(WiFi.localIP());
      audio.connecttohost(preferences.getString("URL0", "").c_str());   
      irRxPin = 10;
      xTaskCreatePinnedToCore(recvIR, "recvIR", 2048, NULL, 1, NULL, 0);
    }

    server.on("/", OnConnect);
    server.on("/config_wifi", handle_config_wifi);
    server.on("/config_url", handle_config_url);
    server.onNotFound(handle_NotFound);
    server.begin();

}

void loop(){
    
  if (millis()-millis_aff> 10000 ) {
      millis_aff=millis();
      change_info=1;
  }
  if (millis()-millis_vu> 100 ) {
     millis_vu=millis();
     affichage_vu();    
  }
  if (code_recu==1){
    Serial.println(chaine_ir);
    analyse_ir();
    code_recu=0;
  }
  if(trigger_reboot==1){  
     millis_reboot=millis();
     trigger_reboot=0;
  }

  audio.loop();

  server.handleClient();

  affichage_vfd();

  lecture_encodeur();

  test_reboot();
}

void affichage_vfd(){
  int longueur=0;
  /* pour mémoire:
  u8g2_font_spleen12x24_mr //15 px
  u8g2_font_VCR_OSD_mr //gras 15px
  u8g2_font_7x13_mr //9px 
  u8g2_font_spleen6x12_mr //8px
  u8g2_font_siji_t_6x10 //symboles
  u8g2_font_chroma48medium8_8u //6px
  u8g2_font_6x12_tr //7px
  u8g2_font_spleen5x8_mf //6PX
  u8g2_font_luRS18_tf //18px
  */
  if(change_info==1){
    u8g2.clearBuffer();				

    //drawbox autour de vol. ou chaine
    u8g2.setFont(u8g2_font_spleen5x8_mf); //6px
    if(chaine_ou_volume==0){
      u8g2.drawStr(3,10,"Chaine");
      u8g2.drawButtonUTF8(18, 22, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  1,  1, "Vol.");
      rotaryEncoder.setEncoderValue(volume_courant);
    }else{
      u8g2.drawButtonUTF8(18, 10, U8G2_BTN_HCENTER|U8G2_BTN_BW1, 30,  1,  1, "Chaine");
      u8g2.drawStr(8,22,"Vol.");
      rotaryEncoder.setEncoderValue(nbr_url);
    }

    //nom station
    u8g2.setFont(u8g2_font_fub17_tr); //17px
    url_eeprom="URL"+String(nbr_url)+"_court";      
    longueur=u8g2.getStrWidth(preferences.getString(url_eeprom.c_str(), "").c_str());
    u8g2.drawStr((int)((256-longueur)/2)+10,17,preferences.getString(url_eeprom.c_str(), "").c_str());
    Serial.print("Station :");Serial.println(preferences.getString(url_eeprom.c_str(), ""));
    nbr_url_courante=nbr_url;

    //titre morceau    
    if(webradio_streamtitle==""){webradio_streamtitle="Titre indispo.";}
    u8g2.setFont(u8g2_font_7x13_mr); //9px
    longueur=u8g2.getStrWidth(webradio_streamtitle.substring(0,15).c_str());
    u8g2.drawStr((int)((256-longueur)/2)+10,30,webradio_streamtitle.substring(0,15).c_str() );
    Serial.print("Titre :");Serial.println(webradio_streamtitle.substring(0,15));
    
    //ip  freq ech. sample    
    u8g2.setFont(u8g2_font_spleen5x8_mf); //6px
    //IP
    longueur=u8g2.getStrWidth(AddressIP_en_chaine(WiFi.localIP()).c_str());
    Serial.print("position : ");Serial.println(256-longueur);
    u8g2.drawStr(256-longueur,48,AddressIP_en_chaine(WiFi.localIP()).c_str());
    Serial.print("IP : ");Serial.println(AddressIP_en_chaine(WiFi.localIP()));

    //bitrate
    u8g2.setFont(u8g2_font_spleen5x8_mf); //6px
    //bitrate  
    longueur=u8g2.getStrWidth(String("000Kb/s").c_str());    
    u8g2.drawStr(256-longueur,35,(String(webradio_bitrate.toInt()/1000)+String("Kb/s")).c_str() );
    Serial.print("Bitrate : ");Serial.println(String(webradio_bitrate.toInt()/1000)+String("Kb/s"));

    //puissance signal wifi   
    u8g2.setFont(u8g2_font_siji_t_6x10); //8px
    if(WiFi.RSSI()>=-57){
      u8g2.drawGlyph(244, 11, 0xE0F0); //4 barres
    }
    if(WiFi.RSSI()>=-70 && WiFi.RSSI()< (-57)){
      u8g2.drawGlyph(244, 11, 0xE0EF); //3 barres
    }   
    if(WiFi.RSSI()< (-70)){
      u8g2.drawGlyph(244, 11, 0xE0EE); //2 barres
    }  
    Serial.print("RSSI: ");Serial.println(WiFi.RSSI());

    // struct vu-metre
    u8g2.setFont(u8g2_font_6x12_tr); 
    u8g2.drawStr(3,39,"L");
    u8g2.drawStr(3,49,"R");

    u8g2.sendBuffer();

    change_info=0;
  }
	
}


void affichage_vu(){
  uint8_t x=0;
  uint16_t vu=0;
  vu=audio.getVUlevel();

  u8g2.clearBuffer();	
 
  u8g2.setFont(u8g2_font_6x12_tr); 
  u8g2.setFontMode(0);
  u8g2.setDrawColor(1);

  //gauche   
  u8g2.setDrawColor(0);
  u8g2.drawBox(15,34,160,3);
  u8g2.setDrawColor(1);
  x=round(highByte(vu)/1.6); 
  u8g2.drawBox(15,34,x,3);
       
  //droite  
  u8g2.setDrawColor(0);
  u8g2.drawBox(15,44,160,3);
  u8g2.setDrawColor(1);
  x=round(lowByte(vu)/1.6); 
  u8g2.drawBox(15,44,x,3); 

  //maj partielle du display surla zoe du vu-metre
  u8g2.updateDisplayArea(1, 4, 22, 2);
}


void irReceived(irproto brand, uint32_t code, size_t len, rmt_symbol_word_t *item){
	if( code ){
    chaine_ir= "IR "+ String(proto[brand].name) + " code:"+String(code, HEX) +" (longueur:"+String(len)+" bits)";
    code_ir=code;
    code_recu=1;
	}  
}

void analyse_ir(){
  switch (code_ir) {
    case CODE_IR_VOLUME_HAUT: case CODE_IR_HAUT:
      if(etat_mute==1){audio.pauseResume();etat_mute=0;}else{volume_courant++; }  
      if (volume_courant>=21){volume_courant=21;}
      audio.setVolume(volume_courant);
      Serial.print("Vol. H: ");
      Serial.println(volume_courant);
      break;
    case CODE_IR_VOLUME_BAS: case CODE_IR_BAS:
      if(etat_mute==1){audio.pauseResume();etat_mute=0;}else{volume_courant--; } 
      if (volume_courant<=0){volume_courant=0;}
      audio.setVolume(volume_courant);
      Serial.print("Vol. B: ");
      Serial.println(volume_courant);
      break;
    case CODE_IR_VOLUME_MUTE:
      audio.pauseResume();
      if(etat_mute==0){etat_mute=1;Serial.println("Mute");break;}
      if(etat_mute==1){etat_mute=0;Serial.print("Unmut: ");break;}
      break;
    case CODE_IR_GAUCHE: 
      nbr_url--;
      if (nbr_url<=0){
          nbr_url=9;
      }
      url_eeprom="URL"+String(nbr_url);
      Serial.println(url_eeprom);
      audio.connecttohost(preferences.getString(url_eeprom.c_str(), "").c_str());
      webradio_streamtitle="";
      break;
    case CODE_IR_DROITE: 
      nbr_url++;
      if (nbr_url>=10){
          nbr_url=0;
      }
      url_eeprom="URL"+String(nbr_url);
      Serial.println(url_eeprom);
      audio.connecttohost(preferences.getString(url_eeprom.c_str(), "").c_str());
      webradio_streamtitle="";
      break;      
  }
  for(int i=0;i<10;i++){
    if(code_ir==CODE_IR_[i]){
        url_eeprom="URL"+String(i);
        audio.connecttohost(preferences.getString(url_eeprom.c_str(), "").c_str());
        webradio_streamtitle="";
        Serial.println(i);
        nbr_url=i;
        break;
    }
  }
}



void OnConnect() {
  server.send(200, "text/html", SendHTML(0)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void handle_config_wifi() {
  server.send(200, "text/html", SendHTML(1)); 
  if(server.hasArg("ssid") &&server.hasArg("password")){
    trigger_reboot=1;
  }
}

void handle_config_url() {
  server.send(200, "text/html", SendHTML(2)); 
}


String SendHTML(int mode){

  ptr = "<!DOCTYPE html> <html lang='fr'>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\" charset='UTF-8',>\n";
  ptr +="<title>Webradio Nalex</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".container {text-align: right; background-color: #F9F6F6; padding: 20px; margin: auto; border: 1px solid #ddd; border-radius: 5px; box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.1); }\n";
  ptr +=".button {background-color: #3498db;border: none;color: white;padding: 15px 15px;text-decoration: none;font-size: 20px;margin: 0px auto 50px;cursor: pointer;border-radius: 4px;}\n";
  ptr +=".button-off {background-color: #34495e;}\n";
  ptr +=".button-off:active {background-color: #2c3e50;}\n";
  ptr +="input[type='text'] {padding: 3px 6px;font-size: 20px;font-weight: 400;line-height: 2;color: #212529;background-color: #fff;background-clip: padding-box;border: 1px solid #ced4da;appearance: none;border-radius: 4px;transition: border-color .15s ease-in-out,box-shadow .15s ease-in-out;}\n";
  ptr +="input[type='text']:focus{color: #212529;background-color: #fff;border-color: #86b7fe;outline: 0;box-shadow: 0 0 0 0.25rem rgb(13 110 253 / 25%);}\n";
  ptr +="input[type='password'] {padding: 3px 6px;font-size: 20px;font-weight: 400;line-height: 2;color: #212529;background-color: #fff;background-clip: padding-box;border: 1px solid #ced4da;appearance: none;border-radius: 4px;transition: border-color .15s ease-in-out,box-shadow .15s ease-in-out;}\n";
  ptr +="input[type='password']:focus{color: #212529;background-color: #fff;border-color: #86b7fe;outline: 0;box-shadow: 0 0 0 0.25rem rgb(13 110 253 / 25%);}\n";

  ptr +="p {font-size: 22px;color: #888;margin-bottom: 10px;}\n";
  ptr +="</style>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h1>Webradio Nalex</h1>\n";
  Serial.print("mode wifi: ");
  Serial.println(WiFi.getMode());
  //WiFi.status() != WL_CONNECTED
  if(WiFi.getMode()==2){
    ptr +="<h3>Mode point d'acces (AP) IP:"; 
    ptr +=AddressIP_en_chaine(WiFi.softAPIP());
    ptr +="</h3>\n";
  } else if(WiFi.getMode()==3){
    ptr +="<h3>Mode Wifi STA - IP locale:";
    ptr +=AddressIP_en_chaine(WiFi.localIP());
    ptr +="</h3>\n";
  }
  
   

  if(mode==0){
        ptr +="<div class='container' style='width:300px;'>\n";
        ptr +="<p>Config Wifi &nbsp;&nbsp;&nbsp;<a class='button button-off' href='/config_wifi'>Valider</a></p><br>\n";
        ptr +="<p>Config URLs &nbsp;&nbsp;&nbsp;<a class='button button-off' href='/config_url'>Valider</a></p>\n";
        ptr +="<br><br><br><br><p style='text-align: center;'><a class='button button-off' href='/?reset=1'>Reboot Webradio</a></p>\n";
        if(server.hasArg("reset")){
            trigger_reboot=1;
            ptr +="<script>alert(\"La webradio va rebooter dans 3 sec. (attention au changement d'IP possible!!\");</script>";
        }
  }

  if(mode==1){
        int n=0;
        if(server.hasArg("scan")){
            n = WiFi.scanNetworks();
            ptr +="<script>alert(\"Scan Ok, Cliquez sur le champ SSID pour obtenir la liste\");</script>";
        }
        
        if(server.hasArg("ssid") &&server.hasArg("password")){
          Serial.print("nouveau SSID: ");
          Serial.println(server.arg("ssid"));
          Serial.print("nouveau password: ");
          Serial.println(server.arg("password"));          
          if(preferences.putString("ssid", server.arg("ssid").c_str()) && preferences.putString("password", server.arg("password").c_str())){
              ptr +="<script>alert(\"SSID et mot de passe ont été modifiés.\\nLa webradio va rebooter  dans 3 sec. (attention au changement d'IP possible!!\");</script>";
          }
        }
      
        ptr +="<div class='container' style='width:550px;'>\n";
        ptr +="<p ><table border='0' width='100%'><tr><td style='text-align: left;'><a class='button button-off' href='/' >Retour</a></td><td style='text-align: right;'><a class='button button-off' href='/config_wifi?scan=1'>Scan SSID</a></td></tr></table></p>\n";
        if (WiFi.status() != WL_CONNECTED ){
          ptr +="<p style='text-align: center;color: red;'>Non connect&eacute; pour le moment</p><br>\n";
        }else{
          ptr +="<p style='text-align: center;'>Actuellement connect&eacute; :-)</p><br>\n";
        }
        ptr +="<form action='/config_wifi' method='post'><p>";
        ptr +="SSID <input type='text' name='ssid' list='ssid' value='";
        if(!server.hasArg("scan")){
          ptr +=preferences.getString("ssid", "");
        }
        ptr +="'> <datalist id='ssid'>";
        ptr +="<option value='";
        ptr +=preferences.getString("ssid", "");
        ptr +="'>";
          for (int i = 0; i < n; ++i) {
          Serial.println(WiFi.SSID(i).c_str());
          ptr +="<option value='";
          ptr +=WiFi.SSID(i).c_str();
          ptr +="'>";   
        }
        ptr +="</datalist>";
        ptr +="</p>";
        ptr +="<p>";
        ptr +="Mot de passe <input type='password' name='password' size='20' maxlength='20' value=\"";
        ptr +=preferences.getString("password", "");
        ptr +="\" >\n";
        ptr +="</p><br><br><div style='text-align: center;'><input type='submit' value='Valider' class='button button-off'></div></form>";
  }

  if(mode==2){
        for(int i;i<10;i++){
            url_eeprom="URL"+String(i);
            urlcourt_eeprom="URL"+String(i)+"_court";
            if( server.hasArg(url_eeprom.c_str()) && server.hasArg(urlcourt_eeprom.c_str())){
              Serial.print("nouvelle URL: ");
              Serial.println(url_eeprom.c_str());
              Serial.print(" -> ");
              Serial.println(server.arg(url_eeprom.c_str()));
              Serial.print(" -> ");
              Serial.println(server.arg(urlcourt_eeprom.c_str()));
              
              if( preferences.putString(url_eeprom.c_str(), server.arg(url_eeprom.c_str()).c_str()) && preferences.putString(urlcourt_eeprom.c_str(), server.arg(urlcourt_eeprom.c_str()).c_str()) ){
                  ptr +="<script>alert(\"L'URL n°: ";
                  ptr +=url_eeprom;
                  ptr +=" (";
                  ptr +=server.arg(urlcourt_eeprom.c_str()).c_str();
                  ptr +=" - ";
                  ptr +=server.arg(url_eeprom.c_str()).c_str();
                  ptr +=") a été modfiée\");</script>";
              }

            }
        }

        ptr +="<div class='container' style='width:800px;'>\n";
        ptr +="<p style='text-align: left;'><a class='button button-off' href='/'>Retour</a></p><br>\n";
        for(int i;i<10;i++){
          ptr +="<form action='/config_url' method='post'><p>";

          ptr +="<br> URL";
          ptr +=String(i);
          ptr +=" <input type='text' name='URL";
          ptr +=String(i);
          ptr +="'  maxlength='100' size='50' value=\"";
          url_eeprom="URL"+String(i);
          ptr +=preferences.getString(url_eeprom.c_str(), "");
          ptr +="\">\n";

          ptr +="<br> Pseudo (max 15 car.) <input type='text' name='URL";
          ptr +=String(i);
          ptr +="_court'  maxlength='15' size='15' value=\"";
          url_eeprom="URL"+String(i)+"_court";
          ptr +=preferences.getString(url_eeprom.c_str(), "");
          ptr +="\">\n";

          ptr += " <input type='submit' value='Valider' class='button button-off'></p>";

          ptr +="</form>";
        }
        
  }

  ptr +="</div><br><br><h3 style='text-align: right;'>(c) Nalex 2024</h3</body>\n";
  ptr +="</html>\n";
  return ptr;
}

String AddressIP_en_chaine(const IPAddress& ipAddress){
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

void test_reboot(){
  if(millis_reboot!=0 && millis()-millis_reboot> 3000){ 
     ESP.restart();
  }
}

void audio_info(const char *info){
    //Serial.print("info        "); Serial.println(info);
    change_info=1;
    webradio_info=String(info);
}

void audio_showstreamtitle(const char *info){
    //Serial.print("streamtitle ");Serial.println(info);
    change_info=1;
    webradio_streamtitle=String(info);
}
void audio_bitrate(const char *info){
    //Serial.print("bitrate     ");Serial.println(info);
    change_info=1;
    webradio_bitrate=String(info);
}

//non utilisé mais existent ds la bibli I2s-audio.h
void audio_commercial(const char *info){  //duration in sec
    //Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    //Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    //Serial.print("lasthost    ");Serial.println(info);
}
void audio_showstation(const char *info){
    //Serial.print("station     ");Serial.println(info);
}

void click_bouton(){
    static unsigned long millis_click = 0; 
    if (millis() - millis_click < 500)
    {
      return;
    }
    millis_click = millis();
    
    if(chaine_ou_volume==1){
      Serial.println("mode reglage volume ");
      chaine_ou_volume=0;
    }else{
      Serial.println("mode reglage chaine ");
      chaine_ou_volume=1;
    }
    change_info=1;
    affichage_vfd();

}

void lecture_encodeur(){
    if (rotaryEncoder.isEncoderButtonClicked())
    {
            click_bouton();
    }  
    if (rotaryEncoder.encoderChanged())
    {
            Serial.print("Valeur: ");
            Serial.println(rotaryEncoder.readEncoder());
            position=rotaryEncoder.readEncoder();
            if(chaine_ou_volume==0){
                if(position<=0){
                  position=0;
                  rotaryEncoder.setEncoderValue(position);
                } 
                if(position>21){
                  position=21;
                  rotaryEncoder.setEncoderValue(position);
                }              
                audio.setVolume(position);
                volume_courant=position;
            }else{
               if(position<0){
                  position=9;
                  rotaryEncoder.setEncoderValue(position);
                } 
                if(position>9){
                  position=0;
                  rotaryEncoder.setEncoderValue(position);
                }                 
                url_eeprom="URL"+String(position);
                Serial.println(url_eeprom);
                audio.connecttohost(preferences.getString(url_eeprom.c_str(), "").c_str());
                webradio_streamtitle="";
                nbr_url=position;
            }            
    }

}






