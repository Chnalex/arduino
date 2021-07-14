
///////////////////////////////
////  Alexandre OGER 2012  ////
///////////////////////////////


//  Horloge a galanometres (1 galva heure - 1 galva minutes/pression atmospherique - 1 galva secondes/temperature ambiante)
//
// - Synchro avec horloge temps reel basee sur un chip DS3231 (compensation du quartz interne en fx de la temp.) sur bus I2C
// - Module barometrique BMP085 sur bus I2C
// - Capteur de temperature DS18B20 (precision reglée sur 0.125deg) sur bus OneWire
// - 2 Leds permettant de connaitre le mode (horloge ou press/temp)
//   les deux leds de mode oscillent en luminosite sur une periode d'une à deux secondes <- interet purement esthetique
// - Bp pour changer de mode horloge <-> pression/temperature (lecture numerique 0v ou 5V)
// - 3 Bp pour maj heures minutes secondes en mode horloge (permet de compenser manuellement la dérive du module DS3231 (1 à 2 min/an)
//   ces Bp sont montes sur un reseau de resitances avec un pont diviseur (une resistance est associee pour chaque BP: 4.7kOhm ou 68kOhm ou 100kOhm)
//   la lecture analogique de la tension resultante permet de connaitre quel Bp est pressé (0V si rien n'est pressé).
// - pour les parties pression baro et temp affichage sur les galvas minutes et secondes avec une echelle specifique imprimee sur les fonds
//
//


// mappage des pins sur un arduino nano
//
//
// Bus I2C -> SDA - pin  A4 et SCL - pin  A5
// Bus OneWire -> pin  D6
// Leds -> horloge - pin D3, baro/temp - pin D5
// Bp mode -> pin A2
// Bp reglage heures,minutes,secondes -> A3 avec reseau de resistance heures=4.7kOhm , minutes=68kOhm, secondes=100kOhm
// Galvanometres -> heures - pin analogique D11, minutes - pin analogique D10 et secondes - pin analogique D9
//  
//
//                                                    ---------------
//                                                TX  |             |  Vin  --------- +9v
//                                                RX  |             |  GND  --------- 0v
//                                                RST |             |  RST
//                                                GND |             |  +5V
//                                                D2  |             |  A0
//                Led mode horloge **  ---------  D3  |   NANO      |  A1
//                                                D4  |             |  A2   --------- Bouton poussoir changement de mode
//             Led mode baro/temp. **  ---------  D5  |             |  A3   --------- Bouton poussoir reglage heures minutes secondes***
//           Capteur 1-wire de temp. * ---------  D6  |             |  A4   --------- Bus I2C broche SDA (data) ****
//                                                D7  |             |  A5   --------- Bus I2C broche SCL (clock) ****
//                                                D8  |             |  A6
//    sortie galva 1 (secondes/temp.)  ---------  D9  |             |  A7
//  sortie galva 2 (minutes/pression)  ---------  D10 |             |  AREF
//            sortie galva 3 (heures)  ---------  D11 |             |  3.3V
//                                                D12 |             |  D13
//                                                      ---------------
// 
// 
//   *    avec resistance de tirage de 4.7kOhm reliee au +5V pour alimentation parasite
//   **   avec resistance 390 Ohm en serie
//   ***  pont diviseur avec une resistance de tirage à la masse de 33kOhm et une resistance de valeur differente par Bp
//   **** avec resistance de tirage de 4.7kOhm relie au +5V



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////// 


// Appel des librairies

 
#include <stdint.h>
  

// gestion du temps (horloge RTC, temps, timer)
#include <DS1307RTC.h>
#include <Time.h>
#include <Timer.h>
 

//bus I2C
#include <Wire.h>

//bus OneWire
#include <OneWire.h>

//module BMP085
#include <Adafruit_BMP085.h>


//creation objet du bus onewire 
OneWire temp(6);  // bus onewire sur pin 6 
byte addr[8]; //tbl adresse capteur temp
byte donnees[12]; //buffer de lecture onewire

//creation d'un objet BMP085
Adafruit_BMP085 baro;

//creation d'un objet timer t et d'une variable pour recuperer le numero du timer defini et pouvoir le stopper 
Timer t;
int timer_perso; // <- instancie quand le timer est lancé

//timestamp de reference mis à jour à chaque seconde, utilisé dans la fx loop 
time_t temps_precedent = 0;  

// mode de fonctionnement (mode horloge par defaut) et valeur de l'etat precedent du bouton de changmement de mode
int mode=1;
int etat_precedent_bp_mode=0;

//variables utilisees pour le calcul analogique du temps, d ela pression et de la température
float valeur;
int sortie1=1;  //variable de stockage de la valeaur de sortie 8bits (0 à 255) du galva 1 (secondes/temp)
int sortie2=1;  //variable de stockage de la valeaur de sortie 8bits (0 à 255) du galva 2 (minutes/pression)
int sortie3=1;  //variable de stockage de la valeaur de sortie 8bits (0 à 255) du galva 3  (heures)
int secondes=0; 
int minutes=0;
int minute_precedente;
int heures=0; 
int heure_precedente;
int pression=0; 
float temperature=0; 
int acquisition=0;

//variable utilisee pour la lecture du reseau de resistance sur A3 pour reglage de l'horloge
int lecture_pin;


//variables utilisees pour le "fondu" led
int intensite=10;
int intensite_max_horloge=110; 
int intensite_max_press_temp=202; 
int sens=1; // 0 montant, 1 descendant
int pas_horloge=4; //pas d'increment pour faire varier la luministe de la led horloge
int pas_press_temp=6; //pas d'increment pour faire varier la luministe de la led baro/temp




//fin appel librairies, creation d'objets et definition des variables globales



/////////////////////////////////////////////////////////////////////////////////////////////////////////////



void setup() {
  
  //init port serie <- pour sortie debug
  Serial.begin(57600);
  
  
  //demarrage du bus I2C
  delay(200);                                
  Wire.begin();  
  
  
  if (!baro.begin()) {
     Serial.println(F("capteur pression introuvable"));
  }
 
  
  //definition de la synchro de l'horloge interne avec le module RTC toutes les 300 secondes
  setSyncProvider(RTC.get);
  setSyncInterval(300);
  Serial.println(F("Attente synchro horloge ... "));
  Serial.println(F("merci de patienter!!!!!"));
  while(timeStatus()== timeNotSet) {     
     Serial.print(F("."));
     delay(1000);
  }
  Serial.println(F("OK"));
  
  
  //initialisation du bus onewire
  initonewire(); 
  
  
  //definition des pins et de leur mode
  
  //output analogique en PWM sur pins 9,10 et 11 pour les galvas
  pinMode(9, OUTPUT); //secondes
  pinMode(10, OUTPUT); //minutes
  pinMode(11, OUTPUT); //heures
  
  //output 3 et 5 en PWM pour les leds de mode horloge et baro/temp
  pinMode(3, OUTPUT);//horloge
  pinMode(5, OUTPUT);//baro/temp
  
  //input en digital pour A2 pour lecture du BP de mode
  pinMode(A2, INPUT);
  //input en analogique pour A3 pour lecture du reseau de resistance et regalge heures minutes et secondes
  pinMode(A3, INPUT);
 
 
  //init variables  minute_precedente et heure_precedente
  minute_precedente=minute();
  if (minute_precedente!=0) {
    minute_precedente--;
  }else{
    minute_precedente=59;
  }  
  heure_precedente=hour();
  if (heure_precedente!=0) {
    heure_precedente--;
  }else{
    heure_precedente=23; 
  } 
 
 //fin void setup 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void loop()
{  
  //update du timer
  t.update();
    

  //check du bouton mode et de l'etat precedent du bouton 
  //pour eviter que le mode change en boucle tant que le BP est enfonce
  if ( digitalRead(A2)==HIGH && etat_precedent_bp_mode==0) {
    
    //set de l'etat precedent du bouton mode
    etat_precedent_bp_mode=1;
    
    //raz luminosite leds mode
    intensite=10;
    
    //bascule d'un mode à l'autre
    if( mode==1) {
       fondu_led(5);
       digitalWrite(3, LOW);
       mode=2;
       //Serial.println("mode2");
       //amortissement du RAZ des aiguilles des galvas
       raz_galva(3,sortie3);
       raz_galva(2,sortie2);
       raz_galva(1,sortie1);
       sortie1=1;
       sortie2=1;
       sortie3=1;
       //relance du timer de maj pression et temperature la fx  maj_pression_temp sera declenchee toute les 10 sec
       timer_perso = t.every(10000, maj_pression_temp);
       


       
    } else  {
       mode=1;
       fondu_led(3);
       digitalWrite(5, LOW);
       //Serial.println("mode1");
       //amortissement du RAZ des aiguilles des galvas (le galva 3 est deja a zero quand l'on sort du mode baro/temp)
       raz_galva(2,sortie2);
       raz_galva(1,sortie1);
       sortie1=1;
       sortie2=1;
       sortie3=1;
       //stop du timer de maj pression et temperature
       t.stop(timer_perso);

    }

  }
  
  
  //reset de l'etat precedent du bouton mode si bouton relache
  if ( digitalRead(A2)==LOW ) {    
    etat_precedent_bp_mode=0;    
  } 
  
  
  // verif du mode: horloge ou baro/temp et affichage en consequence
  
  //mode 1:horloge
  if( mode==1) {
    
      //aff de la led du mode horloge et extinction de la led baro/temp
      fondu_led(3);
      digitalWrite(5, LOW);
    
    
      //test reseau resistance <-debug
      lecture_pin=analogRead(A3); 
      if(lecture_pin!=0 ) {
         Serial.println(lecture_pin);
      }
    
      //lecture analogique des BP de reglage H,M,S <- de 0 a 1024
    
      //reglage secondes si BP correspondant enfonce
      if(lecture_pin>70 && lecture_pin<110) {
          //set horloge interne arduino
          setTime(hour(),minute(),0,day(),month(),year()); 
          //set du module RTC
          RTC.set(now());
          // delay pour eviter de boucler trop rapidement si le doigt reste enfonce sur le bouton
          delay(750);
      }
      
      //reglage minutes si BP correspondant enfonce
      if(lecture_pin>110 && lecture_pin<150) {
          int minute_courante=minute();
          minute_courante++;
          if(minute_courante==60) {
            minute_courante=0;
          }
          //set horloge interne arduino 
          setTime(hour(),minute_courante,second(),day(),month(),year()); 
          //set du module RTC
          RTC.set(now());
          // delay pour eviter de boucler trop rapidement si le doigt reste enfonce sur le bouton
          delay(500);
      }
      
      //reglage heures si BP correspondant enfonce
      if(lecture_pin>680 && lecture_pin<720) {
          int heure_courante=hour();
          heure_courante++;
          if(heure_courante==24) {
            heure_courante=0;
          }
          //set horloge interne arduino 
          setTime(heure_courante,minute(),second(),day(),month(),year()); 
          //set du module RTC
          RTC.set(now());
          // delay pour eviter de boucler trop rapidement si le doigt reste enfonce sur le bouton
          delay(500);
      } 
  
      
      
    
      //la maj de l'aff de l'horloge ne se fait que si le timestamp courant a change
      if(now()!=temps_precedent) {
 
        temps_precedent = now();
        secondes=second();
        minutes=minute();
        heures=hour();
        
        //sortie sur port serie pour debug
        Serial.print(heures);
        Serial.print(F(":"));
        Serial.print(minutes);
        Serial.print(F(":"));
        Serial.println(secondes);
        
        
        //gestion galva des secondes
        //calcul proportionnel ideal 
        valeur= secondes*255/60;
        sortie1=(int) valeur;
        //rattrapage pour "coller" a realite de la deviation des galvas
        if(secondes>0 && secondes<18) {
          sortie1=sortie1+2;
        }
        if(secondes>=18 && secondes<30) {
          sortie1=sortie1+6;
        }
        if(secondes>=30 && secondes<44) {
          sortie1=sortie1+7;
        }
        if(secondes>=44 && secondes<48) {
          sortie1=sortie1+4;
        }
        if(secondes>=48 && secondes<51) {
          sortie1=sortie1+2;
        }
        if(secondes>53 && secondes<60) {
          sortie1=sortie1-2;
        }
        //fin rattrapage
        //amortissement du RAZ de l'aiguille du galva si besoin
        if(secondes==0) {
            raz_galva(1,255);
        }        
        //sortie PWM sur le galva des secondes
        analogWrite(9,sortie1); 
     
     
     
        
        //gestion galva  minutes
        //increment pour "coller" a realite de la deviation des galvas
        if(minutes>17 && minutes<44) {
          minutes++;
        }
        //calcul proportionnel ideal
        valeur= minutes*255/60;        
        sortie2=(int) valeur;
        //rattrapage pour "coller" a realite de la deviation des galvas
        if(minutes>10 && minutes<18) {
          sortie2++;sortie2++;
        }
        if(minute()==43) {
          sortie2=sortie2-3;
        }
        if(minutes==44) {
          sortie2=sortie2+2;
        }
        if(minutes==45) {
          sortie2=sortie2+4;
        }
        if(minutes==46) {
          sortie2=sortie2+3;
        }
        if(minutes>=47 && minutes<54) {
          sortie2=sortie2+2;
        }
        //fin rattrapage
        //amortissement du RAZ de l'aiguille du galva si besoin
        if(minutes==0  && minute_precedente!=minutes) {
            raz_galva(2,255);
            minute_precedente=minutes;
        }
        //sortie PWM sur le galva des minutes 
        analogWrite(10,sortie2);




        //gestion galva heures
        //l echelle des heures etant plutot grande, pas de systeme rattrapage necessaire
        //calcul proportionnel ideal
        valeur= heures*255/23;
        sortie3=(int) valeur;
        //amortissement du RAZ de l'aiguille du galva si besoin
        if(heures==0 && heure_precedente!=heures) {
            raz_galva(3,255);
            heure_precedente=heures;
        }
        //sortie PWM sur le galva des minutes 
        analogWrite(11,sortie3); 
        
      }
   //fin mode 1    
  }
 
 
 //mode 2:baro/temp 
 if( mode==2) {
   
    //les variables pression et temperature sont mises a jour toutes les 10sec par un timer
    //comme les prises de mesures par les capteurs sont longues (environ 1 sec)
    //on ralenti le systeme outre mesure en les interrogeants en permanance.
    //ce qui, par exemple empeche la fonction fondu_led de tourner correctement


    //aff de la led baro/temp et extinction de la led horloge
    fondu_led(5);
    digitalWrite(3, LOW);
                


        
    //routine amusante pour faire bouger l'aiguille des galvas heures,minutes et secondes
    //comme une pulsation en attendant la 1ere mesure de pression et de temp.
    if (acquisition==0){
         if (sortie2==253){
            analogWrite(9,253);
            analogWrite(10,253);
            analogWrite(11,253);
            delay(200);
            analogWrite(9,127);
            analogWrite(10,127);
            analogWrite(11,127);
            delay(200);
            analogWrite(9,60);
            analogWrite(10,60);
            analogWrite(11,60);
            delay(150);
            analogWrite(9,30);
            analogWrite(10,30);
            analogWrite(11,30);
            delay(100);
            analogWrite(9,0);
            analogWrite(10,0);
            analogWrite(11,0);
            sortie1=1;
            sortie2=1; 
            sortie3=1;
         } 
         sortie1=sortie1+6; 
         sortie2=sortie2+6;
         sortie3=sortie3+6;
         analogWrite(9,sortie1);
         analogWrite(10,sortie2);
         analogWrite(11,sortie3);        
    //fin routine amusante et aff des donnees si elle sont acquises     
 
    } else  {  
           
        //reset du galva des heures
        if (sortie3!=0) {
          raz_galva(3,sortie3);
          sortie3=0; 
        }
 
        
        //pression atmo  
        if (pression<990){
            pression=990;   //offset pour ne pas descendre en dessous de la valeur mini d'affichage
        }
        if (pression>1035){
            pression=1035; //offset pour ne pas monter au dela de la valeur maxi d'affichage
        }
        valeur= (pression-990)*255/45; // <- mise à l'echelle pour 45 graduations sur la galva allant de 990 mB  à 1035 mB
        sortie2=(int) valeur;
        sortie2=sortie2+1;        
        analogWrite(10,sortie2);


        //temperature        
          if (temperature<10){
            temperature=10;   //offset pour ne pas descendre en dessous de la valeur mini d'affichage
          }
          if (temperature>40){
            temperature=40;//offset pour ne pas monter au dela de la valeur maxi d'affichage
          }
          //Serial.print("Temperature :");
          //Serial.println((int) (temperature+0.5));
          valeur= ((int) (temperature-8.5))*255/30; // <- mise à l'echelle pour 30 graduations sur la galva allant de 10 degres  à 40 degres
          sortie1=(int) valeur;
          //leger rattrapage pour coller a l'echelle
          sortie1=sortie1-3;
          analogWrite(9,sortie1);    
    }

  //fin mode 2
 }
   
   
//fin void loop   
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//defintion des fonctions annexes



void maj_pression_temp() {//fonction de prise de mesure de la pression atmo et de la temprature ambiente
  
    //pression atmo
    //13 mB <- compensation entre la  valeur mesuree et la valeur relevee à Amiens par météo france
    //la compensation est liee a l'altitude par rapport au niveau de la mer
    pression=baro.readPressure()/100+13;     
    Serial.print(F("Pression :"));
    Serial.println(pression);
   
   
   //temperature  
   temperature=retourne_temp();
   Serial.println("");
   Serial.print(F("Temperature :"));
   Serial.println(temperature);
   Serial.println("");
   //set de la variable d'acquisition
   acquisition=1;
 
 //fin void maj_pression_temp 
}



void raz_galva(int numero_galva, int valeur_courante_affichee) {//fonction d'amortissement du retour a zero des galvas
    
  //permet de faire revenir le galva à la position zero en 0.6s si la valeur courante affichee dépasse le milieu du cadran
  //ou 0.3 s s'il est avant le milieu
  //cela évite de faire taper le galva en butée basse par un abaissement brutal de la tension
  //cette fonction sert d'amortisseur programmable
  
  if( valeur_courante_affichee>127) {
      //retour en 0.5s
      analogWrite(8+numero_galva,130);// <- astuce pour obtenir les pins 9,10 et 11 en fx du numero de galva envoye
      delay(50);
  
      analogWrite(8+numero_galva,100); 
      delay(200);
      analogWrite(8+numero_galva,60); 
      delay(200); 
      analogWrite(8+numero_galva,35); 
      delay(200); 
      analogWrite(8+numero_galva,0);     
  } else {
      //retour en 0.3s
      analogWrite(8+numero_galva,60); 
      delay(150); 
      analogWrite(8+numero_galva,35); 
      delay(150); 
      analogWrite(8+numero_galva,0);       
      
  }
  
//fin void raz_galva 
}



void fondu_led(int led) { //fonction de variation de la luminosite des leds de mode
  
    int pas;
    int intensite_max;
    
    
    switch (led) {
    case 3:    
          pas=pas_horloge;
          intensite_max=intensite_max_horloge;
      break;
    case 5:
          pas=pas_press_temp;
          intensite_max=intensite_max_press_temp;
      break;
  }

  if( sens==0 && intensite==intensite_max){
    sens=1;
    delay(150);
  }
  if( sens==1 && intensite==10){
    sens=0;
    delay(150);
  } 
  switch (sens) {
    case 0:    
         intensite=intensite+pas; 
      break;
    case 1:
         intensite=intensite-pas;  
      break;
  }
  
  
  analogWrite(led, intensite); 
  if (led==3){
    delay(35);
  }else{
    delay(25);
  }
  
//fin void fondu_led  
}


  
void initonewire() { // fonction d'init du bus onewire

    //reset bus et reset recherche de matos
    temp.reset();
    temp.reset_search();
    //recherche capteur de temp 
    temp.search(addr);
    //select adresse du capteur sur les bus <- la premiere adresse stockee
    temp.select(addr);	
    //modif de la resolution des capteurs de temp en fx de la precision souhaitee
    temp.reset();
    temp.select(addr);   
    temp.write(0x4E);     //indique que l'on va ecrire dans la ram     
    temp.write(0x4B);    // valeur par defaut de l'alarme temp haute
    temp.write(0x46);    // valeur par defaut de l'alarme temp basse
    // les differentes resolutions
    //temp.write(0x7F);    // 12-bit  = 0.0625 deg
    temp.write(0x5F);    // 11-bit = 0.125 deg
    //temp.write(0x3F);    // 10-bit = 0.25 deg
    //temp.write(0x1F);    // 9-bit = 0.5 deg
    temp.reset();       
    //verif au cas ou le crc d'id du capteur 1 est faux 
    if ( OneWire::crc8( addr, 7) != addr[7] ) { 
         Serial.println(F("CRC BUS INVALIDE"));
         return;
    }
    //verif de l'id du capteur sur le bus, une id commencant par 0x28 correspond ‡ un DS18B20
    if ( addr[0] != 0x28){
	    Serial.println(F("CAPTEUR BUS  NON CONFORME"));
      return;
    }
     
// fin void initonewire     
}
  

  
float retourne_temp() { // recupere la temp du capteur selectionne       
    
    int HighByte, LowByte, TReading, SignBit, i;
    float temp_tempo;
    temp.reset();                           //reset du bus
    temp.select(addr); 	                    //selection du capteur sur le bus
    temp.write(0x44,1);                     //alim (mode parasite)+ demande de mesure de temperature
    //delay(100);                           //delais d'attente prise de mesure sur 9 bits
    //delay(200);                           //delais d'attente prise de mesure sur 10 bits
    delay(450);                             //delais d'attente prise de mesure sur 11 bits
    //delay(800);                           //delais d'attente prise de mesure sur 12 bits
    temp.reset();                           //reset du bus
    temp.select(addr);                      //select du capteur   
    temp.write(0xBE,1);                     //preparation lecture RAM
        Serial.print("Ram - DS18B20 :  ");
	for ( i = 0; i < 9; i++) {          //RAM stockee dans 9 octets
        donnees[i] = temp.read();           //stockaage de chaque octet dans le buffer
        Serial.print(donnees[i]);
        Serial.print("-");
    }
    	
    //routine de conversion des valeurs HEX pour aff temp en °C. La temp se trouve sur les deux premiers octets donnees[0] et donnees[1] 
    LowByte = donnees[0];
    HighByte = donnees[1];
    //concatenation des deux octets de temp
    TReading = (HighByte << 8) + LowByte;
    //gestion du bit de temp negative
    SignBit = TReading & 0x8000;  
    if (SignBit) { 
       TReading = (TReading ^ 0xffff) + 1; 
    }
    //calcul de temp en fx de la resolution du capteur ->stockage toujours sur 12bits meme si la resoltion est configuree plus basse
    temp_tempo = TReading*0.0625;		
    if (SignBit) {
      temp_tempo=temp_tempo*(-1);
    }
    Serial.print(temp_tempo);
    temp_tempo=temp_tempo-1.5;
    return temp_tempo;


//fin float retourne_temp
}
  
  
