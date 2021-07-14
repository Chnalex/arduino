
///////////////////////////////
////  Alexandre OGER 2016  ////
///////////////////////////////


// afficheur tb de bord moto multifonctions


//afficheur lcd 12x4 sur bus i2c
// capteur 6 positions sur la boite de vitesse (N-1-2-3-4-5) relié à un pont diviseur de tension et lecture sur pin analogique
// GPS sur port serie pour infos horloge et vitesse
// capteur de temp exterieur sur bus onewire



/*|
                              
                                             ________| |__
                               MOSI          | |()|  | | | SCK    
                               RX_LED/SS     |       | | | MISO
                               1/TX          |      ICSP | VIN   -------- Alim moto
                               0/RX          |           | GND   ------- Masse lcd+gps+moto
                               RESET         |           | RESET
                               GND           |           | +5V   ------- VCC lcd + GPS
        ----SDA (DC) LCD        2/SDA        | ARDUINO   | NC
        ----SCL (CS) LCD        3(PWM)/SCL   |           | NC
                               4/A6          |           | A5    
                               5 (PWM)       |   MICRO   | A4      
 capteur BV (0-5V analogique)  6 (PWM)/A7    |           | A3       
                               7             |           | A2        
        ----RX soft (GPS)      8/A9          |           | A1  
        ----TX soft (GPS)      9 (PWM)/A10   |           | A0    
        ----one wire (temp)    10 (PWM) /A11 |           | AREF
                               11(PWM)       |   _____   | 3.3V
      ----BP (heure ete/hiver) 12 (PWM)/A12  |  [ USB ]  | 13 (PWM)
                                             ----|___|---

*/

//conf afficheur LCD 12x4
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,12,4);

//conf onewire
#include <OneWire.h>
OneWire  ds(10);
//a changer si autre capteur avec une autre id
byte addr[8]= {0x28, 0xFF, 0xEF, 0xA2, 0x61, 0x15, 0x02, 0xF2};


//conf gps
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
static const int RXPin = 8, TXPin = 9;
static const uint32_t GPSBaud = 4800;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


//conf eprom (heurehive/ete)
#include <EEPROM.h>



//variables diverses

uint8_t gpsok=0;
uint8_t sensorVal;
uint8_t rapport=9;
uint8_t rapport_courant=9;
uint8_t compte=0;

uint8_t BP=12;
uint8_t BV=A7;

int  celsius=20;


uint8_t heure_courante=99;
uint8_t minute_courante=99;
uint8_t heures;
uint8_t minutes;
int     vitesse_courante=999;
uint8_t temp_courante=99;
uint8_t offset_temps;

uint8_t cc0[8] = {     // caractere perso 0
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000,
    B10000
 };

uint8_t cc1[8] = {     // caractere perso 1
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
 };

uint8_t cc2[8] = {    // caractere perso 2
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000,
    B00000,
    B00000
  };

uint8_t cc3[8] = {    // caractere perso 3
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
  };

uint8_t cc4[8] = {   // caractere perso 4
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000
  };

uint8_t cc5[8] = {    // caractere perso 5
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000,
    B00000
  };

uint8_t cc6[8] = {    // caractere perso 6
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B00000,
    B00000
  };

uint8_t cc7[8] = {     // caractere perso 7
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
  };





// tblx des caracteres perso pour former les nums sur 3 colonnes + le 'N' sur 4 colonnes
//            0 (N)      1        2         3        4        5        6        7        8        9    
char bn1[]={7,4,4,7,   3,1,4,   2,2,1,    2,2,1,   1,4,1,   1,2,2,   1,2,2,   2,2,1,   1,2,1,   1,2,1};
char bn2[]={1,2,3,1,   4,1,4,   7,6,5,    6,6,1,   5,6,1,   5,6,7,   1,6,7,   4,4,1,   1,6,1,   5,6,1};
char bn3[]={1,4,4,1,   3,1,3,   1,3,3,    3,3,1,   4,4,1,   3,3,1,   1,3,1,   4,4,1,   1,3,1,   3,3,1};




void setup() {
  
  pinMode(BP,INPUT_PULLUP); //bp pour passage heure d'ete/heure d'hiver
  pinMode(BV,INPUT);//capteur BV lecture tension sur 

  //demarrage port serie soft
  ss.begin(GPSBaud);

  
  //bus onewire (capteur de temp)
  ds.reset();  
  ds.select(addr);
  ds.reset();
  ds.select(addr);  

  /*
   ds.write(0x4E);     //indique que l'on va ecrire dans la ram     
   ds.write(0x4B);    // valeur par defaut de l'alarme temp haute
   ds.write(0x46);    // valeur par defaut de l'alarme temp basse
   ds.write(0x1F);    // 9-bit = 0.5 deg
   ds.reset();
  */ 

  lcd.init();                       
  lcd.init(); 
  lcd.backlight();
  lcd.clear();

  //on pousse les caracteres perso dans l'afficheur
  lcd.createChar(0,cc0);
  lcd.createChar(1,cc1);
  lcd.createChar(2,cc2);
  lcd.createChar(3,cc3);
  lcd.createChar(4,cc4);
  lcd.createChar(5,cc5);
  lcd.createChar(6,cc6);
  lcd.createChar(7,cc7);
  
  anim_demarrage();
  
  check_temp();

  //lecture decalage heure ete/hiver
  offset_temps=EEPROM.read(1);



}



void loop() {

     verif_rapport();

     //delai pour check des trames du GPS
     smartDelay(500);
     
     sensorVal = digitalRead(BP);
     if (sensorVal == LOW) {
        heure_hiver_ete();
     }
     
     if (compte==20){
       check_temp();
       compte=0;
     }
    
    verif_rapport();
    
    if(gpsok==1){ 
        recup_gps();
    }else{
       check_gps();
    }

    compte++;
}

void affgrandnombre(int digit)
{
  if(digit==0){
      // cas specifique du 'N' sur 4 colonnes  
      // Ligne 1
      lcd.setCursor(0,0);
      lcd.write(bn1[0]);
      lcd.write(bn1[1]);
      lcd.write(bn1[2]);
      lcd.write(bn1[3]);
      // Ligne 2
      lcd.setCursor(0,1);
      lcd.write(bn2[0]);
      lcd.write(bn2[1]);
      lcd.write(bn2[2]);
      lcd.write(bn2[3]);
      // Ligne 3
      lcd.setCursor(0,2);
      lcd.write(bn3[0]);
      lcd.write(bn3[1]);
      lcd.write(bn3[2]); 
      lcd.write(bn3[3]);   
  }else{
      // chiffre de 1 à 9 sur 3 colonnes   
      // Ligne 1
      lcd.setCursor(1,0);
      lcd.write(bn1[digit*3+1]);
      lcd.write(bn1[digit*3+2]);
      lcd.write(bn1[digit*3+3]);
      // Ligne 2
      lcd.setCursor(1,1);
      lcd.write(bn2[digit*3+1]);
      lcd.write(bn2[digit*3+2]);
      lcd.write(bn2[digit*3+3]);
      // Ligne 3
      lcd.setCursor(1,2);
      lcd.write(bn3[digit*3+1]);
      lcd.write(bn3[digit*3+2]);
      lcd.write(bn3[digit*3+3]);    
  }
}


void smartDelay(unsigned long ms){
  
    lcd.setCursor(11,0);
    lcd.print(" ");
    
    unsigned long start = millis();
    do{
      while (ss.available()) {
        gps.encode(ss.read());
      }
    }while (millis() - start < ms);

    lcd.setCursor(11,0);
    lcd.write(2);
   
}



void check_gps(){
  
   for(uint8_t j=0;j<11;j++){
      verif_rapport();
      if(gps.location.isValid()){
        gpsok=1;
        break;
      }
      delay(500);   
   }
   
}



void heure_hiver_ete(){
  if(offset_temps==1){
    EEPROM.write(1, 2);
    offset_temps=2;
  }else{
    EEPROM.write(1, 1);
    offset_temps=1;
  } 
  recup_gps();
  delay(1000);
}


  
void recup_gps(){
  
  char buf [4]; 
    
  //horloge
  heures=gps.time.hour()+offset_temps;
  minutes=gps.time.minute();

  if(heures !=heure_courante){
    /*
    if(heure_courante!=99){
       lcd.setCursor(0, 3); 
       lcd.print("  ") ;
    }
    */
    sprintf(buf, "%02d", gps.time.hour()+offset_temps);
    lcd.setCursor(6, 0); 
    lcd.print(buf) ;
    heure_courante=heures;
  }
   if(minutes !=minute_courante){
    /*
    if(minute_courante!=99){
       lcd.setCursor(3, 3); 
       lcd.print("  ") ;
    }
    */
    sprintf(buf, "%02d", gps.time.minute());
    lcd.setCursor(9, 0); 
    lcd.print(buf) ;
    minute_courante=minutes;
  }

  //direction
  // lcd.setCursor(6, 1); 
  // lcd.print(gps.cardinal(gps.course.deg()));
  //lcd.print(gps.course.deg());
  
  //vitesse
  int vi = abs((int)gps.speed.kmph());

  if(vi !=vitesse_courante){
    /*
    if(vitesse_courante!=999){
       lcd.setCursor(8, 1); 
       lcd.print("   ") ;
    }
    */


    lcd.setCursor(8, 2);
    lcd.print("    ") ;
    
    lcd.setCursor(9, 2);
    
    if (vi>100){
       lcd.setCursor(8, 2); 
    }

    
    lcd.print(vi) ;
    vitesse_courante=vi;
  } 
}




void check_temp(){
  //Serial.print("temperature: "); 
  byte i;
  byte data[12];


  /*
  if ( !ds.search(addr)) {
    ds.reset_search();
    delay(200);
    Serial.println("pas cool"); 
    return;
  }
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("pas cool2"); 
      return;
  }
  for ( i = 0; i < 8; i++) {          
    Serial.println(addr[i]);  
  }
  */

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);     
  delay(150);     
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);        
  for ( i = 0; i < 9; i++) {          
    data[i] = ds.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
  //// default is 12 bit resolution, 750 ms conversion time
  celsius = round((float)raw / 16.0)-3;
  aff_temp(celsius);
  //Serial.println(celsius);
}



void aff_temp(int valeur){

  char buf [4];
  if(valeur !=temp_courante){
    /*
    if(temp_courante!=99){
       lcd.setCursor(8, 3); 
       lcd.print("  ") ;
    }
    */
    sprintf (buf, "%02i", valeur);
    lcd.setCursor(0, 3);
    lcd.print(buf) ;
    temp_courante=valeur;
  }
}




void verif_rapport(){
  
     int capteur_boite = analogRead(BV);
     //lcd.setCursor(7, 1);
     //lcd.print("    ") ;
     //lcd.setCursor(7, 1);
     //lcd.print(capteur_boite) ;
     
     if (capteur_boite > 460 && capteur_boite < 539) {
        rapport=5;
        if (rapport!=rapport_courant) {
          aff_rapport(5);
          rapport_courant=rapport;
        }
        return;
     }
     if (capteur_boite > 540 && capteur_boite < 624) {
        rapport=4;
        if (rapport!=rapport_courant) {
          aff_rapport(4);
          rapport_courant=rapport;
        }
        return;
     }
     if (capteur_boite > 625 ) {
        rapport=3;
        if (rapport!=rapport_courant) {
          aff_rapport(3);
          rapport_courant=rapport;
        }
        return;
     }
     if (capteur_boite > 376 && capteur_boite < 459) {
        rapport=2;
        if (rapport!=rapport_courant) {
          aff_rapport(2); 
          rapport_courant=rapport;
        }
        return;
     }
     if ( capteur_boite < 240) {
        rapport=1;
        if (rapport!=rapport_courant) {
          aff_rapport(1);
          rapport_courant=rapport;
        }
        return;
     }
     if (capteur_boite > 241 && capteur_boite < 375) {
        rapport=0;
        if (rapport!=rapport_courant) {
          aff_rapport(0);
          rapport_courant=rapport;
        }
        return;
     }
 
}

  
void aff_rapport(uint8_t valeur)
{
   vide_rapport();
   affgrandnombre(valeur);
}


void vide_rapport(){
  for(int i=0;i<4;i++){
    for(int j=0;j<3;j++){
      lcd.setCursor(i, j);
      lcd.print(" "); 
    }
  }
}


void anim_demarrage(){

 for(int j=0;j<3;j++){
      for(int i=2;i<10;i++){
        lcd.setCursor(i, 1);
        lcd.write(1);
        if (i>0) {
          lcd.setCursor(i-1, 1);
          lcd.write(4);
        }
        delay(120);
      }
      
      for(int i=8;i>2;i--){
        lcd.setCursor(i, 1);
        lcd.write(1);
        lcd.setCursor(i+1, 1);
        lcd.write(4);
        delay(120);
      }
 }

  lcd.clear();

  for(int i=0;i<12;i++){
    lcd.setCursor(i, 0);
    lcd.write(1);
    lcd.setCursor(i, 1);
    lcd.write(1);
    lcd.setCursor(i, 2);
    lcd.write(1);    
    delay(90);
  }
  
  lcd.setCursor(1, 1); 
  lcd.print("Hello ;-)") ;
  delay (3000);  
  lcd.clear();
  lcd.setCursor(0, 1); 
  lcd.print("  AffMeule  ") ;
  lcd.setCursor(0, 3); 
  lcd.print("(c) Alex '16") ;
  delay (2000); 
  //fin anim 
  
  lcd.clear();

  //affichege des elements de fond
  lcd.setCursor(8, 0); 
  //lcd.setCursor(2, 3); 
  lcd.print(":") ;

  lcd.setCursor(2, 3);
  //lcd.setCursor(10, 3); 
  lcd.print((char)223);
  lcd.print("c") ;
  
  lcd.setCursor(8, 3); 
  lcd.print("Kmh") ;

  lcd.setCursor(6,1);
    lcd.print("_") ;
    lcd.print("_") ;
    lcd.print("_") ;
    lcd.print("_") ;
    lcd.print("_") ;
    lcd.print("_") ;

  lcd.setCursor(6,2);
    lcd.write(0);    

  lcd.setCursor(6,3);
    lcd.write(0);
 
}
