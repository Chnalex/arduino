///////////////////////////////
////  Alexandre OGER 2011  ////
///////////////////////////////


//machine inutile
// un inter sur pin 2
// un servomoteur sur pin 3
// afficheur LCD sur pins 5,4,6,11,10,9 et 8


//
//                                                    ---------------
//                                                TX  |             |  Vin  --------- +6v 4-LR6
//                                                RX  |             |  GND  --------- 0v
//                                                RST |             |  RST
//                                                GND |  ARDUINO    |  +5V
//         inter sur dessus boite     +5v--/ ---  D2  |             |  A0
//               servomoteur           ---------  D3  |             |  A1
//                             LCD     ---------  D4  |             |  A2   
//                             LCD     ---------  D5  |    NANO     |  A3   
//                             LCD     ---------  D6  |             |  A4   
//                                                D7  |             |  A5   
//                             LCD     ---------  D8  |             |  A6
//                             LCD     ---------  D9  |             |  A7
//                             LCD     ---------  D10 |             |  AREF
//                             LCD     ---------  D11 |             |  3.3V
//                                                D12 |             |  D13
//                                                    ---------------
// 
// 

#include <LiquidCrystal.h>

#include <Servo.h>

/*
machine ridicule qui ne sert à rien
une led
un servo moteur
un interrupteur
un afficheur LCD 
 */
 
 
int interPin = 2; //connexion interrupteur
int servoPin = 3; //connexion servo moteur
int ledPin = 13; //sortie led
int val=0; //variable intermediaire lecture de l'inter
int val1=0; //variable tempo aff
int cpt=0; //variable compteur pour affichage
int temps; //variable pour affichage de l'uptime
int temps_precedent; //variable pour affichage de l'uptime
int angle=40; //variable de sortie pour l'agnle du servomoteur
int j=0; //aff cyclique des messages
char* aff; //variable de sortie sur l'ecran lcd
char* retour; //chaine tempo
char* chaines[]={"Nan !!!!!", 
                 "Pas d'accord !!",
                 "Et puis quoi encore!",
                 "Don't touch!!!!",
                 "Tu te crois ou ?", 
                 "#@8/?&@# !!!!!",
                 "Retire tes pattes !!", 
                 "Toi pas comprendre!?!",
                 "Achtung !!!",
                 "Pas glop !!!!",
                 "Laisse moi",
                 "Non merci ...",
                 "Pffff !!!!"};
Servo servo1; //constructeur servomoteur
LiquidCrystal lcd(5,4,6,11,10,9,8); //constructeur affichage LCD

void setup() { 

  Serial.begin(9600);  
  pinMode(ledPin, OUTPUT); 
  pinMode(interPin, INPUT);
  randomSeed(analogRead(0));
  servo1.attach(servoPin); 
  servo1.write(angle); 
  lcd.begin(20, 2);
  lcd.print("Machine");
  lcd.setCursor(0, 1);
  lcd.print("       inutile.....");
  delay(10000); 
  lcd.clear() ;
  lcd.setCursor(0, 0);
  lcd.print("Il est temps de ");
  lcd.setCursor(0, 1);
  lcd.print(" faire quelquechose");
  
}


void loop() {
  
    val = digitalRead(interPin);
    Serial.println(val);
    if (val==1) {
        val1=1;
        lcd.clear() ;
        digitalWrite(ledPin, HIGH);
        int i = random(3000);
        delay(i);
        angle=168;
        servo1.write(angle);
        if (cpt>2) {
          lcd.setCursor(0, 1);
          lcd.print("deja ");
          lcd.print(cpt);
          lcd.print(" essais");
        }
        lcd.setCursor(0, 0);
        aff=affichage();
        lcd.print(aff);
        delay(500);
        angle=40;
        servo1.write(angle);
        delay(2000);
        cpt++;     
    } else {
      temps=millis()/1000;
      if (val1==1 && temps_precedent!=temps) {
        lcd.clear() ;
        lcd.setCursor(0, 0);
        lcd.print("le temps passe....");
        lcd.setCursor(4, 1); 
        lcd.print(temps);
        lcd.print(" secondes !!");
        digitalWrite(ledPin, LOW);
        temps_precedent=temps;
      }
    } 
  
}


char * affichage() {
    retour=chaines[j];
    if (j>=12) {
      j=0;
    } else {
      j++;
    }
    return retour;
}
