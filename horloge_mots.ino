
///////////////////////////////
////  Alexandre OGER 2016  ////
///////////////////////////////


// horloge à mots


// horloge avec 23 mots permettant d'afficher l'heure (cf variable tableau leds[])
// utilisation de 3 registres a decalage pour illuminer les 23 blocs de leds
// un BP pour passage heure hiver/ete
// horloge RTC a faible derive I2C sur base DS3231 (protocol de communication idem que DS1307, mais bcp plus precis-> derive de moins de 2min/an)
// remise à l'heure via le port serie (envoie d'une chaine formatée : HHMMSSJJMMAAAA)
//
//
//                                                    _________| |__
//                                                TX  |  |()|  | |  |  Vin  --------- +12v
//                                                RX  |        | |  |  GND  --------- 0v
//                                                RST |       ICSP  |  RST
//                                                GND |             |  +5V  --------- alim horloge et registres a decalage
//                                                D2  |             |  A0   --------- registre a decalage: Data
//                                                D3  |   NANO      |  A1   --------- registre a decalage: Latch
//                                                D4  |             |  A2   --------- registre a decalage: Clock
//                                                D5  |             |  A3   
//                                                D6  |             |  A4   --------- Bus I2C SDA pour horloge RTC
//                                                D7  |             |  A5   --------- Bus I2C SCL pour horloge RTC 
//                                                D8  |             |  A6
//                                                D9  |             |  A7
//                                                D10 |             |  AREF
//                                                D11 |    _____    |  3.3V
//                  BP heure ete/hiver  --------- D12 |   [ USB ]   |  D13
//                                                    -----|___|-----
// 



//conf eprom (heurehive/ete)
#include <EEPROM.h>

//registre à décalage
#include <ShiftOutX.h>
#include <ShiftPinNo.h>

//shiftOutX(byte _latchPin, byte _dataPin, byte _clockPin, byte _bitOrder, byte _NofRegisters);
shiftOutX regOne(A1, A0, A2, MSBFIRST, 3); 


// gestion du temps (horloge RTC, temps, timer)
#include <DS1307RTC.h>
#include <Time.h>
 #include <TimeAlarms.h>
 
//bus I2C
#include <Wire.h>

char buffer_serie[32];

uint8_t sensorVal;

uint32_t leds[] = {
    
    2,      //0
    16,     //1
    8,      //2
    64,     //3
    1,      //4
    32,     //5  
    4,      //6
    256,    //7
    1024,   //8
    512,    //9
    2048,   //10
    8192,   //11
    128,    //12
    4096,   //13->heure
    16384,  //14->et
    65536,  //15->moins
    32768,  //16->le
    2097152,//17->5
    262144, //18->10
    1048576,//19->15
    524288, //20->20
    4194304,//21->30
    131072, //22->40


};


#define heure 13  
#define et 14 
#define moins 15  
#define le 16 
#define cinq 17 
#define dix 18 
#define quart 19  
#define vingt 20  
#define trente 21  
#define quarante 22  

#define BP 12 


int heure_courante=0;
int minute_courante=0;
uint32_t valeur_courante=0;

void setup() {

   Serial.begin(56700);

   pinMode(BP,INPUT_PULLUP); //bp pour passage heure d'ete/heure d'hiver
     
   //ttes les leds en OFF
   regOne.allOff(); 

   //test de toutes les leds
   for (int i=0;i<23;i++) {
    regOne.pinOn(leds[i]);
    delay(200); 
    regOne.pinOff(leds[i]);

   }
  //demarrage du bus I2C
  delay(400);                                
  Wire.begin();  

  //definition de la synchro de l'horloge interne avec le module RTC toutes les 300 secondes
  setSyncProvider(RTC.get);
  setSyncInterval(3600);
  delay(200);
  
  if(timeStatus()== timeSet) {       
       regOne.pinOn(leds[et]);
    } else {
       regOne.pinOn(leds[moins]);
   }

   delay(2000);
   
  //ttes les leds en OFF
  regOne.allOff();

  //declaration d'un timer toutes les secondes pour le cadencement des taches
   Alarm.timerRepeat(5,Cadenceur); 

}


void loop() {

  //trigger de declenchement de verif des timers d'alarme toues les 10 millisecondes
  Alarm.delay(100);

  ecoute_serie();

  sensorVal = digitalRead(BP);
  if (sensorVal == LOW) {
     heure_hiver_ete();
     delay(1000);
  }
 
}

void ecoute_serie()  {
  
  int cpt=0;
  int hr=0;
  int mi=0;
  int sec=0;
  int jr=0;
  int mo=0;
  int an=0;
  char inByte; 
  
  raz_buffer();
  while (Serial.available() > 0) {
    inByte = Serial.read();
    buffer_serie[cpt]=inByte;
    cpt++;
    delay(5); 
  }
  
 if(cpt>0){
      Serial.println(buffer_serie);

      //format HHMMSSJJMMAAAA
      hr=((buffer_serie[0] - '0')*10) + (buffer_serie[1]- '0');
      mi=((buffer_serie[2] - '0')*10) + (buffer_serie[3]- '0');
      sec=((buffer_serie[4] - '0')*10) + (buffer_serie[5]- '0');
      jr=((buffer_serie[6] - '0')*10) + (buffer_serie[7]- '0');
      mo=((buffer_serie[8] - '0')*10) + (buffer_serie[9]- '0');
      an=((buffer_serie[10] - '0')*1000) + ((buffer_serie[11] - '0')*100)+((buffer_serie[12] - '0')*10)+(buffer_serie[13]- '0');
      Serial.print(hr);
      Serial.print(" ");
      Serial.print(mi);
      Serial.print(" ");
      Serial.print(sec);
      Serial.print(" ");
      Serial.print(jr);
      Serial.print(" ");
      Serial.print(mo);
      Serial.print(" ");
      Serial.print(an);
      Serial.println(" ");                           
      setTime(hr,mi,sec,jr,mo,an);
      RTC.set(now());  
      Cadenceur(); 
 }
}

void heure_hiver_ete(){
    int heures=hour();
    int minutes=minute();
    int secondes=second();
    int jour_mois=day();
    int mois=month();
    int annee=year();
    int heure_ete=EEPROM.read(200);
    if(heure_ete==0){
          if(heures==23){
            heures=0;
          }else{
            heures++;
          }
          setTime(heures,minutes,secondes,jour_mois,mois,annee);
          RTC.set(now());
          EEPROM.write(200,1);   
     }else{
          if(heures==0){
            heures=23;
          }else{
            heures--;
          }
          setTime(heures,minutes,secondes,jour_mois,mois,annee);
          RTC.set(now());
          EEPROM.write(200,0);       
     }
     Cadenceur(); 
}



void raz_buffer()  {  
  for(int i=0;i<32;i++){
    buffer_serie[i]='\0';
  } 
}


void Cadenceur(){ 

  uint32_t valeur=0;
  int heure_valeur=0;
  int heure_tempo=0;
  uint32_t minute_valeur=0;

   if(minute()!=minute_courante || hour()!=heure_courante){    

        if(minute()>=0 && minute()<5){
          minute_valeur=0;
        }
        if(minute()>=5 && minute()<10){
          minute_valeur=leds[cinq]; 
        }
        if(minute()>=10 && minute()<15){
          minute_valeur=leds[dix]; 
        } 
        if(minute()>=10 && minute()<15){
          minute_valeur=leds[dix]; 
        }        
        if(minute()>=15 && minute()<20){
          minute_valeur=leds[et]+leds[quart]; 
        }      
        if(minute()>=20 && minute()<25){
          minute_valeur=leds[vingt]; 
        } 
        if(minute()>=25 && minute()<30){
          minute_valeur=leds[vingt]+leds[cinq]; 
        }        
        if(minute()>=30 && minute()<35){
          minute_valeur=leds[trente]; 
        }   
        if(minute()>=35 && minute()<40){
          minute_valeur=leds[trente]+leds[cinq];  
        }
        if(minute()>=40 && minute()<45){
          minute_valeur=leds[quarante];  
        }
        if(minute()>=45 && minute()<50){
          minute_valeur=leds[moins]+leds[le]+leds[quart];  
        }
        if(minute()>=50 && minute()<55){
          minute_valeur=leds[moins]+leds[dix];  
        }
        if(minute()>=55 && minute()<60){
          minute_valeur=leds[moins]+leds[cinq];  
        }

        
        if(hour()==12 ){
          heure_tempo= 12;
        }else{
          heure_tempo= hour() % 12;
        }

        if(minute()>=45 ){
          heure_tempo++;      
        }
        heure_valeur=leds[heure_tempo];  

                       
        valeur= heure_valeur + leds[heure] + minute_valeur;

        if(valeur!=valeur_courante){
          regOne.allOff();
          regOne.pinOn(valeur);
          valeur_courante=valeur;
        }
        minute_courante=minute(); 
        heure_courante=hour(); 
   }
  
   Serial.println("******************************");
   Serial.println("Reglage Horloge");
   Serial.println(""); 
   Serial.print(hour());
   Serial.print(F(":"));
   Serial.print(minute());
   Serial.print(F(":"));
   Serial.print(second());
   Serial.print("  ");
   Serial.print(day());   
   Serial.print(F("/"));
   Serial.print(month());   
   Serial.print(F("/"));
   Serial.println(year()); 
   Serial.println("");       
   Serial.println("pour mettre a jour l'heure");
   Serial.println("taper une chaine comme suit:");
   Serial.println("HHMMSSJJMMAAAA");
   Serial.print(hour());
   Serial.print(minute());
   Serial.print(second());
   Serial.print(day());   
   Serial.print(month());   
   Serial.println(year());   
   Serial.println("******************************");
   Serial.println(""); 
   Serial.println(""); 
   Serial.println(""); 

} 
