
///////////////////////////////
////  Alexandre OGER 2021  ////
////     Pedalboard      ////
///////////////////////////////

// pedalier d'effet midi
// base sur une ancienne pedalwahwah (seul sont utlises potard et switch d'origine)
// boitier additionnel acouple pour offir 4 footswitchs (boutons poussoirs) et 5 leds

  
/*
                                                    +-----+
              +----[PWR]-------------------| USB |--+
              |                            +-----+  |
              |         GND/RST2  [ ][ ]            |
              |       MOSI2/SCK2  [ ][ ]  A5/SCL[ ] |    
              |          5V/MISO2 [ ][ ]  A4/SDA[ ] |    
              |                             AREF[ ] |
              |                              GND[ ] |
              | [ ]N/C                    SCK/13[ ] |   
              | [ ]IOREF                 MISO/12[ ] |________ footswitch 3   -> Vers GND
              | [ ]RST                   MOSI/11[ ]~|________ footswitch 1   -> Vers GND  
              | [ ]3V3    +---+               10[ ]~|________ footswitch 2   -> Vers GND
              | [ ]5v    -| A |-               9[ ]~|________ footswitch 4   -> Vers GND
              | [ ]GND   -| R |-               8[ ] |________ Switch wahwah  -> Vers GND 
              | [ ]GND   -| D |-                    |
              | [ ]Vin   -| U |-               7[ ] |   
              |          -| I |-               6[ ]~|
              | [ ]A0    -| N |-               5[ ]~|
potard wah ___| [ ]A1    -| O |-               4[ ] |
GND-> LED3 ___| [ ]A2     +---+           INT1/3[ ]~| 
GND-> LED4 ___| [ ]A3                     INT0/2[ ] |________ LED5   -> Vers GND    
GND-> LED1 ___| [ ]A4/SDA  RST SCK MISO     TX>1[ ] |  
GND-> LED2 ___| [ ]A5/SCL  [ ] [ ] [ ]      RX<0[ ] |
              |            [ ] [ ] [ ]              |
              | UNO WIFI2  GND MOSI 5V  ____________/
               \_________________|_____/
    
 
 */


#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();


//definition BD, sw, potard et leds
#define bp3 12
#define bp2 10
#define bp1 11
#define bp4 9
#define sw1 8
#define pot1 A1
#define led1 A4
#define led2 A5
#define led3 A2
#define led4 A3
#define led5 2

//valeurs des control change MID qui sont envoyes
//source norme: https://fr.audiofanzine.com/mao/editorial/dossiers/le-midi-les-midi-control-change.html
#define pedale 92 //normalise comme niveau de tremolo
#define foot_bp1 64 //normalise comme pedale de maintien (on/off)
#define foot_bp2 65 //normalise comme Portamento (on/off)
#define foot_bp3 66 //normalise comme Pedale de soutien (on/off)
#define foot_bp4 67 //normalise comme Pedale d'étouffement (on/off)
#define foot_sw1 68 //normalise comme Pedale de légato (on/off)

//definition du canal midi
#define canalmidi 1


//initialisation des valeurs temps reel des bp,sw et potard
bool bp1_valeur=false;
bool bp2_valeur=false;
bool bp3_valeur=false;
bool bp4_valeur=false;
bool sw1_valeur=false;
int  pot1_valeur=0;

//initialisation des valeurs d'etat des bp,sw et potard 
bool bp1_etat=false;
bool bp2_etat=false;
bool bp3_etat=false;
bool bp4_etat=false;
bool sw1_etat=false;
int  pot1_etat=0;

//variables de debounce pour les bp 1 a 4 
unsigned long delay_now1 = 0;
unsigned long delay_now2 = 0;
unsigned long delay_now3 = 0;
unsigned long delay_now4 = 0;

void setup() {

  MIDI.begin (MIDI_CHANNEL_OFF); //start midi
  pinMode(bp1, INPUT_PULLUP);
  pinMode(bp2, INPUT_PULLUP);
  pinMode(bp3, INPUT_PULLUP);
  pinMode(bp4, INPUT_PULLUP);
  pinMode(sw1, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT); 
  pinMode(led5, OUTPUT); 


  //Serial.begin(115200);
 

}


void loop() {
  delay(10);
  bp1_valeur =  digitalRead(bp1);
  bp2_valeur =  digitalRead(bp2);
  bp3_valeur =  digitalRead(bp3);
  bp4_valeur =  digitalRead(bp4);
  sw1_valeur =  digitalRead(sw1);        
  pot1_valeur = analogRead(pot1);   

  if (bp1_valeur == false && (millis())>delay_now1){
    check_switch_etat(bp1); 
    delay_now1 = millis()+1500; //debouce a 1.5 sec
  }
  if (bp2_valeur == false && (millis())>delay_now2){
    check_switch_etat(bp2);
    delay_now2 = millis()+1500; //debouce a 1.5 sec
  }
  if (bp3_valeur == false && (millis())>delay_now3){
    check_switch_etat(bp3);
    delay_now3 = millis()+1500;  //debouce a 1.5 sec
  }
  if (bp4_valeur == false && (millis())>delay_now4){
    check_switch_etat(bp4);
    delay_now4 = millis()+1500;  //debouce a 1.5 sec
  }

  
  if (sw1_valeur != sw1_etat){
   //Serial.println(sw1_etat);
   check_switch_etat(sw1);
   sw1_etat=sw1_valeur;
   
  }

  check_pedal_etat();
 
}


void check_switch_etat(int switch_numero){

    bool on_off=false;
    int led=0;
    int touche_midi;


    switch(switch_numero){
        case bp1:
           if (bp1_etat==false){bp1_etat=true;on_off=true;}else{bp1_etat=false;on_off=false;}
           led=led1;
           touche_midi=foot_bp1;
           break;
        case bp2:
           if (bp2_etat==false){bp2_etat=true;on_off=true;}else{bp2_etat=false;on_off=false;}
           led=led2;
           touche_midi=foot_bp2;
           break;                
        case bp3:
          if (bp3_etat==false){bp3_etat=true;on_off=true;}else{bp3_etat=false;on_off=false;}
          led=led3;
          touche_midi=foot_bp3;
          break;  
        case bp4:
          if (bp4_etat==false){bp4_etat=true;on_off=true;}else{bp4_etat=false;on_off=false;}
          led=led4;
          touche_midi=foot_bp4;
          break; 
        case sw1:
          if (sw1_etat==false){on_off=true;}else{on_off=false;}
          touche_midi=foot_sw1;
          led=led5;
          break;   
    }
    
  // allumage extinction d ela led correspondante et transmission midi de l'etat du capteur
  if (on_off==true){
     digitalWrite(led,HIGH);
     //Serial.print(touche_midi);Serial.println(" 127");
    MIDI.sendControlChange(touche_midi, 127, canalmidi);   
   } else {
     digitalWrite(led,LOW);
     //Serial.print(touche_midi);Serial.println(" 0");
    MIDI.sendControlChange(touche_midi, 0 , canalmidi);       
   }

  
}


void check_pedal_etat(){

  int valeur=0;
    if (pot1_valeur<950){
      pot1_valeur=950;
    }
  
  valeur=(map (pot1_valeur, 950, 1023, 0, 127));
  valeur = constrain(valeur, 0, 127);
  valeur=(map (valeur, 0, 127, 127, 0));

  //pour eviter toute transmission en continue de la valeur de potard de la pedale, ont ne trasnmet que si la valeur change (avec stockage et comparaison etat precedent)
  if(pot1_etat!=valeur){
    MIDI.sendControlChange(pedale, valeur, canalmidi); 
    //Serial.print(pedale);Serial.print(" "); Serial.println(valeur);
    pot1_etat=valeur;
  }



  
}
