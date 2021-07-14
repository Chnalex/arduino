 
////////////////////////////////////
////  Alexandre OGER 2012-2016  ////
////////////////////////////////////

  // domotique maison 
  //gestion des volets et de la chaudiere 
  
  
  
  //Version courante: 2.5
  
  //**** Historique ************************************************************************************************************

  // Mai 2016  V2.5        -> integration d'un module utilisant 4 registres à decalage 74HC595 pour commander les 4 cartes relais
                           // 8 relais par carte. les 32 relais sont pilotés par 3 fils depuis l'arduino (data, clock,latch)
                           //integration d'un afficheur LCD graphique tft comme afficheur secondaire 
  // Decembre 2015 V2.0    -> refonte globle de la sortie video tjrs par port serie, mais avec un raspberry pi (raspi) servant de 
                           // de carte vidéo HDMI, idem pour le touchscreen qui est géré en usb par le raspi comme une souris. 
                           // l'arduino envoie de simples trames d'etat (menu courant, données) un prg en python sur le raspi 
                           // gere l'affichage et envoie en retour une trame sur le port seria pour signaler des actions 
                           // (commandes volets, chauffage, changement menu courant, modif programmation volet ou chauffage)
  // Mi-mars 2013 v1.3     -> retouche tableau lever/couher du soleil 
                           // modif de gestion des volets pour prise en compte du delta de temps entre
                           // les passage heure été/hiver hiver/été et les fin de quinzaine
                           // evite un decalage d'une heure pour les fins des mois d'octobre et de mars après le chgt d'heure
  // Mi-novembre 2012 V1.2 -> suppression menu alarme et deplacement menu progs volets à la place
                           // retouches esthetiques IHM
  // Novembre 2012 V1.1    -> correction bug heures pleines forcées sur le chauffage  
                           //amelioration de l'hysteresis de declenchement du chauffage
                           //chauffage enclenché= 2 relais activés <- mesure de sécu si un relais defaillant
                           //correction affichage des jours en bas à gauche: ajout d'espace dans les chaines a afficher 
  // Octobre 2012  V1.0    -> 1ere version de prod
  //*****************************************************************************************************************************
  

  
  
  //connexion à l'arduino: 
  
  // carte utilisant 4 registres à decalage 74HC595. Chaque registre est connecté à une carte de 8 relais
  // deux sondes de temperature dallas onewire chacune sur un bus dedié (1 pin par bus)
  // une horloge temps reel a base de DS1307 communiquant par le bus I2C
  // un afficheur LCD I2C 160x240 pixels couleur
  // une sortie HDMI via raspberry pi sur port serie n°=1 relié a un moniteur LCD 10 pouces
  // touchscreen en usb sur le raspi, retour via le port serie 1
  
  //utilisation l'eeprom interne de l'arduino pour stockage de toutes les données de programation volets et chauffage
  
  
  //mappage des pins:
  
  //connecteur haut gauche et milieu
  // 2      -> 1er bus onewire 
  // 3      -> 2eme bus onewire
  // 20-21  -> bus I2C pour l'horloge temps reel et l'afficheur LCD 
  // 8-9-10 -> carte registres à decalage 74HC595
 


/*

                                                                  ________ TX1____
                                                                 | _______ RX1____|_____ port serie pour carte video HDMI raspi
                          bus onewire 1 _________                | |  ____ SDA ___
                          bus onewire 2 _______  |               | | | --- SCL ---|----- bus I2C (lcd 4x20 et horloge temps reel)
  carte 4 registres 74HC595--- | | |           | |               | | | |  
           ____________________________________________________________________________
           |             o o o o o o   o o o o o o o o   o o o o o o o o               |     
           |             1 1 1 1 9 8   7 6 5 4 3 2 1 0   1 1 1 1 1 1 2 2    22-23  o o |         
           |             3 2 1 0                         4 5 6 7 8 9 0 1    24-25  o o |         
           |                                                                26-27  o o |         
           |---------                                                       28-29  o o |         
           |         |                                                      30-31  o o |         
           |  USB    |                                                      32-33  o o |         
           |         |                                                      34-35  o o |         
           |---------                ARDUINO   MEGA  2560                   36-37  o o |      
           |                                                                38-39  o o |         
           |                                                                40-41  o o |         
           |                                                                42-43  o o |         
           |                                                                44-45  o o |         
           |                                                                46-47  o o |         
           |                R     G G V                       A A A A A A   48-49  o o |         
           |---------       S 3 5 N N I  A A A A A A A A  A A 1 1 1 1 1 1   50-51  o o |             
           |  VCC    |      T V V D D N  0 1 2 3 4 5 6 7  8 9 0 1 2 3 4 5   52-53  o o |             
           |---------     o o o o o o o  o o o o o o o o  o o o o o o o o              |                         
           |---------------------------------------------------------------------------              
                                                                                             
                                                                                            
                                                                  
                                                                     
                                          
                                               
*/



  //mappage des adresses de l'eeprom
  //    prog des 9 volets: de 100 à 153 (voir void check_volets pour les détails)
  //    prog générale du chauffage (ON/OFF): 5
  //    prog journalière du chauffage (7 jours, 3 plages/jour): de  10 à 93
  //    heure ete/heure hiver : 200
  
  
  
  
  //
  // Librairies
  //
  
  #include <stdint.h>
  
  //appel de la librairie du watchdog processeur pour les resets auto
  #include <avr/wdt.h>
  
  //appel librairie onewire pour communiquer avec les sondes de temperature
  #include <OneWire.h>
  
  //appel librairie wire pour la communication I2C 
  #include <Wire.h>
  
  //appel des librairies de gestion du temps (horloge RTC, temps et alarmes)
  #include <DS1307RTC.h>
  #include <Time.h>
  #include <TimeAlarms.h>
  
  //appel librairie EEPROM stockage/lecture des données de prog volet et chauffage
  #include <EEPROM.h>
  
  //appel librairie permettant de connaitre la quantitié de RAM dispo
  #include <MemoryFree.h>

  //appel de la lib pout l'afficheur LCDsur bus i2c
  #define _Digole_Serial_I2C_  
  #include <DigoleSerial.h>
  
 
  #include <ShiftOutX.h>
  #include <ShiftPinNo.h>

 


  //
  //declarations variables et constantes
  //  
 
  //init de regitres à décalage
  shiftOutX regOne(9, 8, 10, MSBFIRST, 4);
 
  // init du lcd sur bus I2C
  DigoleSerialDisp mydisp(&Wire,'\x27');
  #define SC_W 176  //largeur aff lcd en pixels
  #define SC_H 220  //hauteur aff lcd en pixels


  //declaration des bus onewire
  //capteurs utilisés DS18S20 ->resolution 0.5 Deg Celsius
  OneWire temp1(2);  // bus onewire 1 sur pin 2 (1er pin apres les pins des relais)
  OneWire temp2(3);  //bus onewire 2 sur pin 3
  
  byte addr1[8]; //tbl adresse capteur temp1
  byte addr2[8]; //tbl adresse capteur temp2
  byte donnees[12]; //buffer de lecture onewire
  
  
  float temperature1=19; //definition et valeur par defaut de la temperature courante du capteur 1
  float temperature2=19; //definition et valeur par defaut de la temperature courante du capteur 2 
  int consigne_temperature=19; //definition et valeur par defaut de la temperature de consigne du chauffage
  int temperature_consigne_virtuelle=17; //definition  de la temperature de consigne virtuelle apres application des heures creuses
  int heure_pleine=0; //definition et valeur par defaut de la variable heure pleine/creuse
  int heure_pleine_forcee=0;//definition et valeur par defaut de la variable de forcage heure pleine/creuse
  int heure_pleine_forcee_heure_max=0;//definition et valeur par defaut de la variable contenant l'heure/min jusqu'a laquelle le forcage sera actif
  int relais_chauffage=0; //definition et valeur par defaut de la variable du relais d'activation du chauffage
  int volet_courant_prog=0; //definition et valeur par defaut de la variable servant à identifier le volets courant du menu de prog des volets
  int jour_courant_prog=0; //definition et valeur par defaut de la variable servant à identifier le jour courant dans le menu de prog des volets
  int heure_en_cours_prog; //defintion variable heure pour la maj de l'horloge
  int minute_en_cours_prog; //defintion variable minute pour la maj de l'horloge
  int seconde_en_cours_prog; //defintion variable seconde pour la maj de l'horloge
  char date_heure[17]="00/00/0000 00:00"; //stockage de la chaine pour afficher la date et l'heure
  char datez[12]="00/00/0000";//stockage de la chaine pour afficher la date 
  char heurez[7]="00:00";//stockage de la chaine pour afficher l'heure
  char ram_uptime[17]="00Jo 0000"; //stockage de la chaine pour afficher la ram dispo et l'uptime
  char uptimez[5]="000"; // l'uptime

  float haut[48] = {19, 19, 19, 19, 19, 19, 19, 19, 19, 19 ,19 ,
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19 };

  float bas[48] =  {19, 19, 19, 19, 19, 19, 19, 19, 19, 19 ,19 ,
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 
                    19, 19, 19, 19 };
   // max min moy des temp. bas et haut
  float mib=19;
  float mab=19;
  float moyb=19;
  float mih=19;
  float mah=19;
  float moyh=19;
   
  char buffer_serie[32];


  const unsigned char fonts[] = {6, 10, 18, 51, 120, 123};
  const char *fontdir[] = {"0\xb0", "90\xb0", "180\xb0", "270\xb0"};
  
  // variable de stockage du menu courant 
  //1=accueil
  //2=prog chauffage
  //3=prog volets
  int menu_courant=1;

  
  // declaration des relais de 1 à 32 pour shiftout
  uint32_t relais[] = {
    1, 
    2,
    4,
    8,
    16,
    32,
    64,
    128,
    256,
    512,
    1024,
    2048,
    4096,
    8192,
    16384,
    32768,
    65536,
    131072,
    262144,
    524288,
    1048576,
    2097152,
    4194304,
    8388608,
    16777216,
    33554432,
    67108864,
    134217728,
    268435456,
    536870912,
    1073741824,
    0X80000000 
  };
  
  //declaration des données levé/couché du soleil par quinzaine (24 plages horaire/an)
  int horaires_soleil[24][4] = { 
    //janvier
    {8,45,17,25},
    {8,30,17,55},
    //fevrier
    {8,10,18,15},
    {7,45,18,30},
    //mars
    {7,15,18,50},
    {7,0,19,20},
    //avril
    {7,15,20,40},
    {7,0,21,10},
    //mai
    {7,0,21,40},
    {7,0,22,00},
    //juin
    {7,0,22,35},
    {7,0,22,50},
    //juillet
    {7,0,22,40},  
    {7,0,22,10},
    //aout
    {7,0,21,40},
    {7,0,21,10},
    //septembre
    {7,0,20,40},
    {7,15,20,00},
    //octobre
    {7,30,19,30},
    {8,10,19,00},
    //novembre
    {7,45,17,45},
    {8,10,17,30},
    //decembre
    {8,30,17,10},
    {8,45,17,10}
  };
  

  //declaration des jours de basule heure ete/heure hiver de 2012 à 2037
  int heures_ete[26][5] = { 
    {2012,25,3,28,10},
    {2013,31,3,29,10},
    {2014,30,3,26,10},
    {2015,29,3,25,10},
    {2016,27,3,30,10},
    {2017,26,3,29,10},
    {2018,25,3,28,10},
    {2019,31,3,27,10},
    {2020,29,3,25,10},
    {2021,28,3,31,10},
    {2022,27,3,30,10},
    {2023,26,3,29,10},
    {2024,31,3,27,10},
    {2025,30,3,26,10},
    {2026,29,3,25,10},
    {2027,28,3,31,10},
    {2028,26,3,29,10},
    {2029,25,3,28,10},
    {2030,31,3,27,10},
    {2031,30,3,26,10},
    {2032,28,3,31,10},
    {2033,27,3,30,10},
    {2034,26,3,29,10},
    {2035,25,3,28,10},
    {2036,30,3,26,10},
    {2037,29,3,25,10}
  };  


  //declaration des compteurs pour les cadenceurs 
  int compteur;  
  int compteur_cligno=0;
  
  
  //variable de test de demarrage-> pour le lcd
  int debut=0;
    
  
  void setup()   { 
    
    //mise en marche du watchdog du processeur pour reset auto en cas de freeze de plus de 8s
    wdt_enable(WDTO_8S);
    
    //conf du port serie 0 pour l'affichage du deboggage
    Serial.begin(19200);
    
    //conf du port serie 1 pour sortie HDMI
    Serial1.begin(57600); 

    //tous les relais à off
    regOne.allOff();
    
    //demarrage du bus I2C
    delay(200);                                
    Wire.begin();   

    //demarrage LCD
    mydisp.begin();
    mydisp.clearScreen(); //CLear screen

   
    //initialisation des deux bus onewire
    initonewire(); 
 
    //utilisation de la led interne de l'arduino
    pinMode (13,OUTPUT);
 

    //definition du timer de synchro avec l'horloge RTC + synchro toute les 2 heures
    setSyncProvider(RTC.get);
    setSyncInterval(7200); 
    
    if(timeStatus()== timeSet) {       
       Serial.println("Horloge Synchonisee avec le module RTC");
    } else {
       Serial.println("Impossible de se synchroniser avec l'horloge");
    }

    //init du compteur du cadenceur à la seconde courante
    compteur=second(); 

    
    init_lcd();
        
    aff_heure();
    aff_uptime();
     
    
    //declaration d'un timer toutes les secondes pour le cadencement des taches
    Alarm.timerRepeat(1,Cadenceur); 

    //declaration d'un timer toutes les 30 minutes pour le rafraichissement du graph sur l'aff LCD
    Alarm.timerRepeat(1800,graph_temp); 
  
  } 
    
    
  
 void loop(){  

    //trigger de declenchement de verif des timers d'alarme toues les 10 millisecondes
    Alarm.delay(10);

    
    //reset du watchdog pour indiquer que le script est en vie
    wdt_reset();
    ecoute_serie();
    
 
  } 


 void ecoute_serie()  {
  
  int cpt=0;
  int ok=0;
  char inByte; 
  int a;
  int b;
  int adresse_debut_volet;
  raz_buffer();
  while (Serial1.available() > 0) {
    inByte = Serial1.read();
    buffer_serie[cpt]=inByte;
    cpt++;
    delay(5);
    
  }

 /*
  * 
  *    menu accueil:
      
          'avXh' ou 's' ou 'b', X=num volet, h haut, s stp, b bas   
          'ac-' chauffage -
          'ac+' chauffage +
          'aho' heure pleine oui
          'ahn' heure pleine non
          'aco' chauffage oui
          'acn' chauffage non
          'apc' menu prog chauffage
          'apv' menu prog volet
          'awb' ts les volets en bas
          'awh' ts les volets en haut
          'aws' ts les volets stop
          'awy' les volets au sud vers le bas
          'awz' les volets au sud stop

          
      menu progvolet
      
          'pvXm' ou 'h' ou 'a' X=num volet, m manu, h heure fixe, a auto  
          'pvr' retour accueil    
          'pvXx00:00.00:00' X=num volet,'x' séparateur ,00:00 Hdeb et 00:00 HFin
      
      menu progchauf:   
      
          'pcr' retour accueil    
          'pcXx00:00.00:00' X=num plage chauf <-3/J su 7j=21 plages,'x' séparateur ,00:00 Hdeb et 00:00 HFin

 */
 if(cpt>0){
      Serial.println(buffer_serie);
    
      //menu accueil
      if (menu_courant==1) {
    
          if (buffer_serie[0]=='a' && buffer_serie[1]=='w') {
               if (buffer_serie[2]=='b' ) {
                   volets_bas();
               }
               if (buffer_serie[2]=='h' ) {
                   volets_haut();
               }
               if (buffer_serie[2]=='s' ) {
                   volets_stop();
               } 
               if (buffer_serie[2]=='y' ) {
                   volets_bas_sud();
               }  
               if (buffer_serie[2]=='z' ) {
                   volets_stop_sud();
               }  
          } 
          
          if (buffer_serie[0]=='a' && buffer_serie[1]=='v') {
              int levolet = buffer_serie[2] - '0';
                action_volet(levolet,buffer_serie[3]);
          } 
          
          if (buffer_serie[0]=='a' && buffer_serie[1]=='c') {
               if (buffer_serie[2]=='-' ) {
                   consigne_temperature--; is_heure_creuse(0);
               } 
               if (buffer_serie[2]=='+' ) {
                  consigne_temperature++;  is_heure_creuse(0);
               }
               if (buffer_serie[2]=='o' ) {
                  EEPROM.write(5,1); 
               }
               if (buffer_serie[2]=='n' ) {
                  EEPROM.write(5,0); 
               }
               check_chauffage();
          } 
          if (buffer_serie[0]=='a' && buffer_serie[1]=='h') {
               if (buffer_serie[2]=='o' ) {
                  heure_pleine=1;is_heure_creuse(1); 
               }
               if (buffer_serie[2]=='n' ) {
                  heure_pleine=0;is_heure_creuse(2);
               }
               check_chauffage();
          }
          if (buffer_serie[0]=='a' && buffer_serie[1]=='p') {
               if (buffer_serie[2]=='c' ) {
                  menu_courant=3; 
               }
               if (buffer_serie[2]=='v' ) {
                  menu_courant=2;
               }
               check_chauffage();
          }
      } 
    
      //menu prog volet
      if (menu_courant==2) {
           if (buffer_serie[0]=='p' && buffer_serie[1]=='v') {
               if (buffer_serie[2]=='r' ) {
                   menu_courant=1;
               } 
               if (buffer_serie[2]=='m' ) {
                    a=buffer_serie[3]-'0';
                    adresse_debut_volet=100+(a-1)*6;
                    if (buffer_serie[4]=='m' ) {
                        EEPROM.write(adresse_debut_volet,0);EEPROM.write(adresse_debut_volet+1,1);
                    }
                    if (buffer_serie[4]=='h' ) {
                       EEPROM.write(adresse_debut_volet,1);EEPROM.write(adresse_debut_volet+1,0);
                    }             
                    if (buffer_serie[4]=='a' ) {
                       EEPROM.write(adresse_debut_volet,1);EEPROM.write(adresse_debut_volet+1,1);
                    }             
               } 
               if (buffer_serie[2]=='p' ) {
    
                  int fin=long_buffer();
                  int pox_x=chaine_trouve(1,0);
                  int pox_12p=chaine_trouve(2,0);
                  int pox_p=chaine_trouve(3,0);
                  int pox_22p=chaine_trouve(4,pox_12p+1);

                  int volet=trouve_int(2,pox_x);
                  int heure_monte=trouve_int(pox_x,pox_12p);
                  int minute_monte=trouve_int(pox_12p,pox_p);
                  int heure_descente=trouve_int(pox_p,pox_22p);
                  int minute_descente=trouve_int(pox_22p,fin);

                  int adresse_debut_volet=100+(volet-1)*6;
                  
                  EEPROM.write(adresse_debut_volet+2,heure_monte);
                  EEPROM.write(adresse_debut_volet+3,minute_monte); 
                  EEPROM.write(adresse_debut_volet+4,heure_descente);
                  EEPROM.write(adresse_debut_volet+5,minute_descente); 
      
                 
               } 
          }  
      } 
    
    
      // menu prog chauf
      if (menu_courant==3) {
            if (buffer_serie[0]=='p' && buffer_serie[1]=='c') {
               if (buffer_serie[2]=='r' ) {
                   menu_courant=1;
               } 
               if (buffer_serie[2]=='p' ) {

                  int fin=long_buffer();
                  int pox_x=chaine_trouve(1,0);
                  int pox_12p=chaine_trouve(2,0);
                  int pox_p=chaine_trouve(3,0);
                  int pox_22p=chaine_trouve(4,pox_12p+1);

                  int num_plage=trouve_int(2,pox_x);
                  int heure_debut=trouve_int(pox_x,pox_12p);
                  int minute_debut=trouve_int(pox_12p,pox_p);
                  int heure_fin=trouve_int(pox_p,pox_22p);
                  int minute_fin=trouve_int(pox_22p,fin);

                  int debut_plage_memoire=10+(num_plage-1)*4;

                  EEPROM.write(debut_plage_memoire,heure_debut);
                  EEPROM.write(debut_plage_memoire+1,minute_debut);
                  EEPROM.write(debut_plage_memoire+2,heure_fin);
                  EEPROM.write(debut_plage_memoire+3,minute_fin);
                 
               } 
          }  
      }    
     aff_menu_courant();  
   }
}


int trouve_int(int debut,int fin){
  int a=0;
  int b=0;
  int c=0;
  
  if(fin-debut==2){
    a=buffer_serie[debut+1]-'0';
    return a;
  }else{
    a=buffer_serie[debut+1]-'0';
    b=buffer_serie[debut+2]-'0';
    c=a*10+b;
    return c;
  }

  return 0; 
  
}

int chaine_trouve(int mode,int val){
    int debut=0;
    int fin=long_buffer();
    char recherche;
    int comptechar=0;

    switch (mode) {
    case 1:
      recherche='x';
      break;
    case 2:
      recherche=':';
      break;
    case 3:
      recherche='.';
      break;
    case 4:
      recherche=':';
      debut=val;
      comptechar=val;
      break;
    }
    for (int i = debut; i < fin; i++){
            if(buffer_serie[i]==recherche){
              break;
            }else{
              comptechar++;
            }
    }  
    
    return comptechar;
  
}

int long_buffer(){
      int comptechar=0;
      for (int i = 0; i < sizeof(buffer_serie) - 1; i++){
        if(buffer_serie[i]=='\0'){
          break;
        }else{
          comptechar++;
        }
      }
      comptechar--;
      return comptechar;
}


void raz_buffer()  {  
  for(int i=0;i<32;i++){
    buffer_serie[i]='\0';
  } 
}

  
  
void Cadenceur(){ 
    
    //RAZ du compteur
    if (compteur==60) {
      compteur=0;
    } 


    //reset de la ligne1 du LCD lors du tout premier demarrage
    if(debut==0){

       debut=1; 
    } 


    // affichage du menu courant toutes les 5 sec
    if (compteur % 5==0 ) {
       aff_menu_courant();
       update_lcd();
    } 
    
    // check chauffage toutes les 10 sec
    if (compteur % 10==0 ) {
       check_chauffage();
    } 

    //check volet à 0 et 30 sec
    if (compteur==5 || compteur==35) {
      check_volets();
      check_heure_ete();
    } 



    //check heure d'été et capteur temp 1 chaue minute
    if (compteur==20) {
      retourne_temp(1);
    } 

    //check capteur temp 2 chaque minute
    if (compteur==50) {
      retourne_temp(2);
    } 
 

    //increment du compteur
    compteur++;  
    //affichage de l'heure sur le LCD
    aff_heure();
    //affichage de l'uptime sur le LCD
    aff_uptime();

  }


void envoie_data_temp(){

  //bas
    for (int i = 0 ; i < 48 ; i++){
      Serial.print(bas[i]) ; 
    } 
    Serial.println(" ");

  //haut
    for (int i = 0 ; i < 48 ; i++){
      Serial.print(haut[i]) ; 
    } 
    Serial.println(" ");
    Serial.println(" ");
  
}

 void aff_menu_courant(){

    char aff[6];
    char aff2[6];
    char aff3[6];
    char aff4[6];
    char aff5[6];
    char aff6[6];
    char aff7[6];
    char aff8[6];
    char aff9[6];
    char aff10[6];
    char aff11[6];
    
    int prog_chauffage=EEPROM.read(5);
    char chaine[300];
    char chaine1[38];
    char chaine2[38];
    char chaine3[38];
    char chaine4[38];
    char chaine5[38];
    char chaine6[38];
    char chaine7[38];
    char chaine8[38];
    char chaine9[38];


    switch (menu_courant) {
      case 1:
        dtostrf(temperature1, 2, 1, aff);
        dtostrf(temperature2, 2, 1, aff2);

        dtostrf(mib, 2, 1, aff6);
        dtostrf(mab, 2, 1, aff7);
        dtostrf(moyb, 2, 1, aff8);
        dtostrf(mih, 2, 1, aff9);
        dtostrf(mah, 2, 1, aff10);
        dtostrf(moyh, 2, 1, aff11);

        if (prog_chauffage==1) {
          sprintf(aff3, "on");
        }else {
          sprintf(aff3, "off");
        }
        if (relais_chauffage==1) {
          sprintf(aff5, "On");
        }else {
          sprintf(aff5, "Off");
        }
        if (heure_pleine==1) {
          sprintf(aff4, "oui");
        }else {
          sprintf(aff4, "non");
        }
        sprintf(chaine, "accueil %s %s %d %s %s %s %s %s %s %s %s %s %s %s", aff,aff2,temperature_consigne_virtuelle,aff3,aff4,date_heure,ram_uptime,aff5,aff6,aff7,aff8,aff9,aff10,aff11);
        Serial1.println(chaine);
        //Serial.println(chaine);
        break;
      case 2:
        strcpy(chaine1, aff_volet(1));
        strcpy(chaine2, aff_volet(2));
        strcpy(chaine3, aff_volet(3));
        strcpy(chaine4, aff_volet(4));
        strcpy(chaine5, aff_volet(5));
        strcpy(chaine6, aff_volet(6));
        strcpy(chaine7, aff_volet(7));
        strcpy(chaine8, aff_volet(8));
        strcpy(chaine9, aff_volet(9));        
        sprintf(chaine, "prog_volet %s %s %s %s %s %s %s %s %s", chaine1,chaine2,chaine3,chaine4,chaine5,chaine6,chaine7,chaine8,chaine9 );
        Serial1.println(chaine);
        //Serial.println(chaine);
        break;
      case 3:
        strcpy(chaine1, aff_chauf(0));
        strcpy(chaine2, aff_chauf(1));
        strcpy(chaine3, aff_chauf(2));
        strcpy(chaine4, aff_chauf(3));
        strcpy(chaine5, aff_chauf(4));
        strcpy(chaine6, aff_chauf(5));
        strcpy(chaine7, aff_chauf(6));
        sprintf(chaine, "prog_chauf %s %s %s %s %s %s %s", chaine1,chaine2,chaine3,chaine4,chaine5,chaine6,chaine7 );
        Serial1.println(chaine);
        //Serial.println(chaine);
        break;
     }

     Serial1.flush();
     Serial.flush();
  
 }

char* aff_volet(int volet){  

  char buffer_volet[22];
  
  int adresse_debut_volet=100+(volet-1)*6;
  int prog_manu=EEPROM.read(adresse_debut_volet);
  int mode_auto=EEPROM.read(adresse_debut_volet+1);
  int heure_monte=EEPROM.read(adresse_debut_volet+2);
  int minute_monte=EEPROM.read(adresse_debut_volet+3);
  int heure_descente=EEPROM.read(adresse_debut_volet+4);
  int minute_descente=EEPROM.read(adresse_debut_volet+5);

  char aff[6];
  
  if(prog_manu==0 && mode_auto==1){
    //manu 
    sprintf(aff, "manu");
  }

  if(prog_manu==1 && mode_auto==1){
    //auto2
    sprintf(aff, "auto2");
  }

  if(prog_manu==1 && mode_auto==0){
    //auto1
    sprintf(aff, "auto1");
  }

  sprintf(buffer_volet, "%s %02d:%02d %02d:%02d",aff,heure_monte,minute_monte,heure_descente,minute_descente);

  return buffer_volet;
  
 }



 char *aff_chauf(int jour_crt){ 

    char buffer_chauf[37];
     
    int debut_plage_memoire=10+(jour_crt)*12;
    int heure_debut1=EEPROM.read(debut_plage_memoire);
    int minute_debut1=EEPROM.read(debut_plage_memoire+1);
    int heure_fin1=EEPROM.read(debut_plage_memoire+2);
    int minute_fin1=EEPROM.read(debut_plage_memoire+3);

    int heure_debut2=EEPROM.read(debut_plage_memoire+4);
    int minute_debut2=EEPROM.read(debut_plage_memoire+5);
    int heure_fin2=EEPROM.read(debut_plage_memoire+6);
    int minute_fin2=EEPROM.read(debut_plage_memoire+7);

    int heure_debut3=EEPROM.read(debut_plage_memoire+8);
    int minute_debut3=EEPROM.read(debut_plage_memoire+9);
    int heure_fin3=EEPROM.read(debut_plage_memoire+10);
    int minute_fin3=EEPROM.read(debut_plage_memoire+11);

    sprintf(buffer_chauf, "%02d:%02d %02d:%02d %02d:%02d %02d:%02d %02d:%02d %02d:%02d",heure_debut1,minute_debut1,heure_fin1,minute_fin1,heure_debut2,minute_debut2,heure_fin2,minute_fin2,heure_debut3,minute_debut3,heure_fin3,minute_fin3);
    
    //Serial.println(buffer_chauf);
    
    return(buffer_chauf);
 }

 

 void check_heure_ete(){
    
      //format stockage: {2012,25,3,28,10}
      //                    0  1  2  3  4
      int i;
      int heure_courante=hour();
      int minute_courante=minute();
      int mois_courant=month(); 
      int jour_courant=day();
      int annee_courante=year();
      int heure_ete=EEPROM.read(200);
      
      for( i= 0; i<26; i++){
        if(heures_ete[i][0]==annee_courante){
           //test debut heure ete
           if(heures_ete[i][2]==mois_courant && heures_ete[i][1]==jour_courant && heure_courante==3 && minute_courante==0 && heure_ete==0){
             setTime(4,0,0,jour_courant,mois_courant,annee_courante);
             RTC.set(now());
             // ecriture eeprom pour stocker un booleen heure d'été à 1
             EEPROM.write(200,1);
             break;
           } 
           //test fin heure ete
           if(heures_ete[i][4]==mois_courant && heures_ete[i][3]==jour_courant && heure_courante==3 && minute_courante==0 && heure_ete==1){
             setTime(2,0,0,jour_courant,mois_courant,annee_courante);
             RTC.set(now()); 
             // ecriture eeprom pour stocker un booleen heure d'été à 0 
             EEPROM.write(200,0);         
             break;
           }
           break;  
        }        
      }
    
  }
 
 
 int check_decalage_heure_ete(){
    
      //permet de compenser pour les quelques jours après un passage
      //heure d'hiver ou d'été, le decalage entre l'heure de programation levé/couché du soleil
      //et la nouvelle heure
      int i;
      int mois_courant=month(); 
      int jour_courant=day();
      int annee_courante=year();
      int heure_ete=EEPROM.read(200);
      
      for( i= 0; i<26; i++){
        if(heures_ete[i][0]==annee_courante){
           //test decalage entree heure d'ete
           if(heures_ete[i][2]==mois_courant && jour_courant>=heures_ete[i][1] ){
             return 1;
           } 
           //test decalage sortie heure d'ete
           if(heures_ete[i][4]==mois_courant && jour_courant>=heures_ete[i][3] ){        
             return -1;
           }
           return 0;  
        }        
      }
    
  } 
  
  void check_volets(){ 
   
    // on passe en revue chacun des 9 volets pour verifier la prog, puis le mode auto et enfin les heures de prog
    // mappage des données en EEPROM: 
    // la conf de chaque volet est stockée sur 6 octets 
    // 
    // prog (1 octet):  oui ou non <- le volet est configuré en manuel ou en prog, si manule on ne fait rien
    // auto (1 octet): oui ou non <- si oui on se refere au tableau horaires_soleil pour savoir si on monte ou baisse le volet
    //                     si non on regard les valeur stockées sur l'eeprom
    // heure et minutes de montée du volet (2 octets)
    // heure et minutes de descente du volet (2 octets)
    //
    // l'adresse de debut du stockage mémoire est 100, la répartition se fait comme suit:
    //  volet1 : de 100 à 105
    //  volet2 : de 106 à 111
    //  volet3 : de 112 à 117
    //  volet4 : de 118 à 123
    //  volet5 : de 124 à 129
    //  volet6 : de 130 à 135
    //  volet7 : de 136 à 141
    //  volet8 : de 142 à 147
    //  volet9 : de 148 à 153
    
    int volet=0;
    int prog_manu=0;
    int mode_auto=0;
    int adresse_debut_volet=0;
    int heure_courante=0;
    int minute_courante=0;
    int mois_courant=0;
    int jour_courant=0;
    int quinzaine_courante=0;
    int heure_monte=0;
    int minute_monte=0;
    int heure_descente=0;
    int minute_descente=0;

    
    //boucle volets
    for ( volet = 1; volet < 10; volet++) { 

        adresse_debut_volet=100+(volet-1)*6;
        prog_manu=EEPROM.read(adresse_debut_volet);
       
        //test prog oui/non
        if (prog_manu==1) {              
            //test mode auto oui/non
            mode_auto=EEPROM.read(adresse_debut_volet+1);
            if (mode_auto==1) { 
 
                
                heure_courante=hour();
                minute_courante=minute();
                mois_courant=month();
                jour_courant=day();
                
                if (jour_courant<15){
                    quinzaine_courante=2*mois_courant-2;
                } else {
                    quinzaine_courante=2*mois_courant-1;  
                }  
              
                heure_monte=horaires_soleil[quinzaine_courante][0]+check_decalage_heure_ete();
                minute_monte=horaires_soleil[quinzaine_courante][1];
                heure_descente=horaires_soleil[quinzaine_courante][2]+check_decalage_heure_ete();
                minute_descente=horaires_soleil[quinzaine_courante][3];   

           
                if(heure_monte==heure_courante && minute_monte==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                      action_volet(volet,'h');
                      //Serial.println("Ce volet monte");
                    }
                }  
             
                if(heure_descente==heure_courante && minute_descente==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                     action_volet(volet,'b'); 
                     //Serial.println("Ce volet descend"); 
                    }
                }  
              
  
            
            } else { 
                //Serial.println("**Prog PERSO**");             
                heure_monte=EEPROM.read(adresse_debut_volet+2);
                minute_monte=EEPROM.read(adresse_debut_volet+3);
                heure_descente=EEPROM.read(adresse_debut_volet+4);
                minute_descente=EEPROM.read(adresse_debut_volet+5);   
                
                heure_courante=hour();
                minute_courante=minute();
                mois_courant=month();
                jour_courant=day();

                 
           
                if(heure_monte==heure_courante && minute_monte==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                       action_volet(volet,'h');
                       //Serial.println("Ce volet monte");
                    }
                }  
             
                if(heure_descente==heure_courante && minute_descente==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                       action_volet(volet,'b');
                      //Serial.println("Ce volet descend");
                    }
                }  
  
              
                
            }
    
        } else {  

            //Serial.println("%%mode manuel%%");
            

        }
    }
    

  }
  
  
  
  
  
  
  void check_chauffage(){ 
    
    
    //verif si chauffage en mode prog ou non
    //appel de la fonction is_heure_creuse <- retour oui ou non <- modif de la consigne de température en conséquence
    //mesure de la temperature des deux capteurs
    //compararaison consigne et temp en tena tcompte d'une hysteresis + ou - 0.5 deg
    //activation du relais de chauffage (relais 32)
    // prog chauffage stockée à l'adresse 5 de l'EEPROM
  
    
    int prog_chauffage;
    char aff[10];
    char aff2[10];
    char buf[21];
    

    
    prog_chauffage=EEPROM.read(5);
    
    if (prog_chauffage==1) {
   
      //verif si heure pleine et adaptation de la consigne en consequence
      is_heure_creuse(0);
          
      dtostrf(temperature1, 2, 1, aff);
      dtostrf(temperature2, 2, 1, aff2);
     
      sprintf(buf, "C:%d T1:%s T2:%s",temperature_consigne_virtuelle,aff,aff2 );

      
      //hysteresis de declanchement du chauffage
      //differenciation du comportement entre heures pleines et creuses
      if(heure_pleine==1){
          if (temperature1>=(temperature_consigne_virtuelle+0.5)){
            regOne.pinOff(relais[31]);
            regOne.pinOff(relais[30]);
            relais_chauffage=0;
          }          
          if (temperature1<=(temperature_consigne_virtuelle-0.5)){
            regOne.pinOn(relais[31]);
            regOne.pinOn(relais[30]);
            relais_chauffage=1;
          }
       } else  {
          if (temperature1>(temperature_consigne_virtuelle+0.5)){
            regOne.pinOff(relais[31]);
            regOne.pinOff(relais[30]);
            relais_chauffage=0;
          }         
          if (temperature1<=(temperature_consigne_virtuelle)){
            regOne.pinOn(relais[31]);
            regOne.pinOn(relais[30]);
            relais_chauffage=1;
          }         
       }
      
    } else {
       regOne.pinOff(relais[31]);
       regOne.pinOff(relais[30]);
       relais_chauffage=2;
    }
    

  }
  




        // btn hp:Oui  heure_pleine=1;is_heure_creuse(1); 
        // btn hp:non  heure_pleine=0;is_heure_creuse(2); 
        //consigne_temperature--; is_heure_creuse(0);
        //consigne_temperature++; is_heure_creuse(0);
        
void is_heure_creuse(int type_forcage){ 
     
       //Serial.println("Verif heure creuse/pleine");
     
       //verif si en fonction de l'heure courante et des valeurs de prog stockées dans l'EEPROM on est en periode d'heure creuse ou non
       //accepte aussi des variables de forçage qui seront valide jusqu'au prochain changement de plage
       //mappage de l'EEPROM:
       //par defaut on est en heure creuse
       //3 plages heure pleine sont stockées pour chaque jour de la semaine
       //par plage, 2 octets pour l'heure de début, 2 octets pour l'heure de fin
       //soit 12 octets par jour, 84 octets au total pour la semaine entiere
       //debut de la plage mémoire à partir de l'adresse :  10
       // -Lundi 10 à 21
       // -Mardi 22 à 33
       // -Mercredi 34 à 45
       // -Jeudi 46 à 57
       // -vendredi 58 à 69
       // -Samedi 70 à 81
       // -Dimanche 82 à 93
       //heure_pleine_forcee
       //heure_pleine_forcee_heure_max=0;
       
       
       int heure_courante;
       int minute_courante;
       int jour_courant;
       int debut_plage_memoire;
       int plage_horaire;
       int heure_debut;
       int minute_debut;
       int heure_fin;
       int minute_fin;
       int horaire_debut;
       int horaire_fin;
       int horaire_courant;
       
       heure_courante=hour();
       minute_courante=minute();
       jour_courant=weekday()-1;
       if (jour_courant==0) { jour_courant=7; }
       debut_plage_memoire=10+(jour_courant-1)*12;


       if (heure_pleine_forcee==1 ) {
             //for ( plage_horaire = 0; plage_horaire < 3; plage_horaire++) {
             //      heure_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4);
             //      minute_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4+1);
             //      heure_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+2);
             //      minute_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+3);
             //      horaire_debut=heure_debut*100+minute_debut;
             //      //Serial.println(horaire_debut);
             //      horaire_fin=heure_fin*100+minute_fin;
             //      horaire_courant=heure_courante*100+minute_courante;
             //      //Serial.println(horaire_fin);
             //      if ( horaire_debut==heure_pleine_forcee_heure_max    horaire_debut==heure_pleine_forcee_heure_max ) {  
             //          heure_pleine_forcee==0;
             //          moniteur_dplcmnt(23,1); 
             //          Serial1.print(heure_pleine_forcee_heure_max);
             //          Serial1.print(" fin forcage HP");
             //          break;                 
             //      } 
             //}
             horaire_courant=heure_courante*100+minute_courante;
 
             //Serial.print(F("Hp/Hc: "));
             //Serial.print(heure_pleine_forcee_heure_max);
             //Serial.print(F(" <-> "));
             //Serial.print(horaire_courant);
             if ( horaire_courant==heure_pleine_forcee_heure_max ) {  
                       heure_pleine_forcee=0;
                       heure_pleine_forcee_heure_max=0;
                       //Serial.print(heure_pleine_forcee_heure_max);
                       //Serial.print(F(" fin forcage HP"));
                                        
            }  
         
       }


       
       if (type_forcage==1 || type_forcage==2) {
             for ( plage_horaire = 0; plage_horaire < 3; plage_horaire++) {
                   heure_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4);
                   minute_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4+1);
                   heure_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+2);
                   minute_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+3);
                   horaire_debut=heure_debut*100+minute_debut;
                   //Serial.println(horaire_debut);
                   horaire_fin=heure_fin*100+minute_fin;
                   //Serial.println(horaire_fin);
                   horaire_courant=heure_courante*100+minute_courante;
                   //Serial.println(horaire_courant);
                   if (horaire_courant<horaire_debut) {
                        heure_pleine_forcee_heure_max=horaire_debut;
                        heure_pleine_forcee=1;
                        break;
                   } 
                   if (horaire_courant>horaire_debut && horaire_courant<horaire_fin) {
                        heure_pleine_forcee_heure_max=horaire_fin;
                        heure_pleine_forcee=1;
                        break;
                   }
                    if (plage_horaire == 2 && horaire_courant>horaire_fin) {
                        heure_pleine_forcee_heure_max=2359;
                        heure_pleine_forcee=1;
                        break;
                   } 
             } 
       }

       
       if (type_forcage==1) {
          heure_pleine=1;        

          //Serial.print(F("HP oui jusqu'a "));         
          //Serial.print(int(heure_pleine_forcee_heure_max/100));
          //Serial.print(F(":"));
          //Serial.print(heure_pleine_forcee_heure_max-int(heure_pleine_forcee_heure_max/100)*100);
       }
       
       if (type_forcage==2) {
          heure_pleine=0;        
 
          //Serial.print(F("HP non jusqu'a "));
          //Serial.print(int(heure_pleine_forcee_heure_max/100));
          //Serial.print(F(":"));
          //Serial.print(heure_pleine_forcee_heure_max-int(heure_pleine_forcee_heure_max/100)*100);
       }
       
       if (type_forcage==0 && heure_pleine_forcee==0) {
             //on passe en revue chacune des 3 plages

             //Serial.print(F("                    "));
             for ( plage_horaire = 0; plage_horaire < 3; plage_horaire++) {
                   heure_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4);
                   minute_debut=EEPROM.read(debut_plage_memoire+plage_horaire*4+1);
                   heure_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+2);
                   minute_fin=EEPROM.read(debut_plage_memoire+plage_horaire*4+3);
                   horaire_debut=heure_debut*100+minute_debut;
                   //Serial.println(horaire_debut);
                   horaire_fin=heure_fin*100+minute_fin;
                   //Serial.println(horaire_fin);
                   horaire_courant=heure_courante*100+minute_courante;
                   //Serial.println(horaire_courant);
                   if ( horaire_debut != 0 && horaire_fin != 0 && horaire_debut<horaire_courant && horaire_courant<horaire_fin ) {
                       heure_pleine=1;
                       //Serial.println("heure pleine OK");
                       break;                 
                   } else {
                       heure_pleine=0;
                       //Serial.println("heure pleine POK");
                   }
             }           
       }
       
      if(heure_pleine==1){
          temperature_consigne_virtuelle=consigne_temperature;
      } else {
          temperature_consigne_virtuelle=consigne_temperature-2;
      }
  
   }
  

  
void volets_bas(){

  for(int i=1;i<10;i++){
    action_volet(i,'b');
  }
  
 }

void volets_haut(){
  for(int i=1;i<10;i++){
    action_volet(i,'h');
  }
 }

void volets_stop(){
  for(int i=1;i<10;i++){
    action_volet(i,'s');
  }
 }

  
void volets_bas_sud(){

  for(int i=2;i<7;i++){
    action_volet(i,'b');
  }
  
 }

 void volets_stop_sud(){
  for(int i=2;i<7;i++){
    action_volet(i,'s');
  }
 }
  
 void action_volet(int volet,char action){
    
    int delais_tempo_volet=300;
    int num_relais;
    num_relais=volet*3-2;
    
    switch (action) {
      case 'h':
        num_relais=num_relais;
        break;
      case 's':
        num_relais=num_relais+1;
        break;
      case 'b':
        num_relais=num_relais+2;
        break;
     }
    //Serial.print("compteur :");
    //Serial.println(compteur);
    //Serial.print("relais: ");
    Serial.println(num_relais);
    
    delay(delais_tempo_volet);
    regOne.pinOn(relais[num_relais-1]);
    delay(delais_tempo_volet);
    regOne.pinOff(relais[num_relais-1]);    
    
  }
  



 void initonewire() { // fonction d'init des bus onewire
  
       //bus 1
       temp1.reset();	
       temp1.reset_search();
       temp1.search(addr1);
       temp1.select(addr1);
       temp1.reset();
       temp1.select(addr1);    
       temp1.write(0x4E);     //indique que l'on va ecrire dans la ram     
       temp1.write(0x4B);    // valeur par defaut de l'alarme temp haute
       temp1.write(0x46);    // valeur par defaut de l'alarme temp basse
       // les differentes resolutions
       //temp1.write(0x7F);    // 12-bit  = 0.0625 deg
       temp1.write(0x5F);    // 11-bit = 0.125 deg
       //temp1.write(0x3F);    // 10-bit = 0.25 deg
       //temp1.write(0x1F);    // 9-bit = 0.5 deg
       temp1.reset();
       
       //bus 2
       temp2.reset();
       temp2.reset_search();
       temp2.search(addr2);
       temp2.select(addr2);
       temp2.reset();
       temp2.select(addr2);    
       temp2.write(0x4E);     //indique que l'on va ecrire dans la ram     
       temp2.write(0x4B);    // valeur par defaut de l'alarme temp haute
       temp2.write(0x46);    // valeur par defaut de l'alarme temp basse
       // les differentes resolutions
       //temp2.write(0x7F);    // 12-bit  = 0.0625 deg
       temp2.write(0x5F);    // 11-bit = 0.125 deg
       //temp2.write(0x3F);    // 10-bit = 0.25 deg
       //temp2.write(0x1F);    // 9-bit = 0.5 deg
       temp2.reset(); 
 
  
 	//verif au cas ou le crc d'id du capteur 1 est faux 
	if ( OneWire::crc8( addr1, 7) != addr1[7] ) { 
	    //Serial.println(F("CRC CAPTEUR BUS 1 INVALIDE"));
        return;
      }
	//verif de l'id du capteur 1 sur le bus, une id commencant par 0x28 correspond ‡ un DS18S20
      if ( addr1[0] != 0x28){
	    //Serial.println(F("CAPTEUR BUS 1 NON CONFORME"));
        return;
      }
      
	//verif au cas ou le crc d'id du capteur 2 est faux 
	if ( OneWire::crc8( addr2, 7) != addr2[7] ) { 
	    //Serial.println(F("CRC CAPTEUR BUS 2 INVALIDE"));
        return;
      }
	//verif de l'id du capteur 2 sur le bus, une id commencant par 0x28 correspond ‡ un DS18S20
      if ( addr2[0] != 0x28){
	    //Serial.println(F("CAPTEUR BUS 2 NON CONFORME"));
        return;
      }         
       
  }
  

  
  
 void retourne_temp(int num_capteur) { // recupere la temp du capteur selectionne       
    
    int HighByte, LowByte, TReading, SignBit, i;
    float temp_tempo;
    
    if (num_capteur==1) {
        //Serial.println("Reset du bus Onewire 1");
        temp1.reset();                        //reset du bus
        temp1.select(addr1); 	
	      temp1.write(0x44,1);                  //alim (mode parasite)+ demande de mesure de temperature
        //Serial.println("coupure alim capteur 1 et demande de mesure");
        //delay(100);                           //delais d'attente prise de mesure sur 9 bits
	      //delay(200);                           //delais d'attente prise de mesure sur 10 bits
        delay(380);                           //delais d'attente prise de mesure sur 11 bits
        //delay(900);                           //delais d'attente prise de mesure sur 12 bits
        //Serial.println("Reset du bus Onewire 1");
	      temp1.reset();                        //reset du bus
        temp1.select(addr1);                  //select du capteur   
        //Serial.println("preparation lecture RAM du capteur 1");
        temp1.write(0xBE,1);                  //preparation lecture RAM
	      for ( i = 0; i < 9; i++) {            //RAM stockee dans 9 octets
            donnees[i] = temp1.read();        //stockaage de chaque octet dans le buffer
            //Serial.println(donnees[i]);
 	      }
        //Serial.println("contenu RAM du capteur 1 OK");
    }
    
    if (num_capteur==2) {
        //Serial.println("Reset du bus Onewire 2");
	      temp2.reset();                        //reset du bus pour couper l'alim
        temp2.select(addr2); 
	      temp2.write(0x44,1);                  //alim (mode parasite)+ demande de mesure de temperature
        //Serial.println("coupure alim capteur 2 et demande de mesure");
        //delay(100);                           //delais d'attente prise de mesure sur 9 bits
	      //delay(200);                           //delais d'attente prise de mesure sur 10 bits
        delay(380);                           //delais d'attente prise de mesure sur 11 bits
        //delay(900);                           //delais d'attente prise de mesure sur 12 bits
        //Serial.println("Reset du bus Onewire 2");
	      temp2.reset();                        //reset du bus pour couper l'alim
        temp2.select(addr2);                  //select du capteur   
        //Serial.println("preparation lecture RAM du capteur 2");
        temp2.write(0xBE,1);                  //preparation lecture RAM
	      for ( i = 0; i < 9; i++) {            //RAM stockee dans 9 octets
  	       donnees[i] = temp2.read();      //stockaage de chaque octet dans le buffer
             //Serial.println(donnees[i]);
	      }
        //Serial.println("contenu RAM du capteur 2 OK");
    }
	
    //routine de conversion des valeurs HEX pour aff temp en °C. La temp se trouve sur les deux premiers octets donnees[0] et donnees[1] 
    LowByte = donnees[0];
    HighByte = donnees[1];
    TReading = (HighByte << 8) + LowByte;
    SignBit = TReading & 0x8000;  
    if (SignBit) { TReading = (TReading ^ 0xffff) + 1; }
    //a modifier en fonction de la resolution
    temp_tempo = TReading*0.0625; //pour resolution de 11bits			
    if (SignBit) {
      temp_tempo=temp_tempo*(-1);
    }

    if (temp_tempo<0) { 
      temp_tempo=1;
    }
        
    if (num_capteur==1) { 
      temperature1=temp_tempo;
    }
    if (num_capteur==2) { 
      temperature2=temp_tempo;
    }
  
}

  
  
void aff_heure() {
    char ligne[10]="         ";
    char buf[5]="    ";
    date_heure[13]=':';
    date_heure[2]='/';
    date_heure[5]='/';

    int heures=hour();
    int minutes=minute();
    int jour_mois=day();
    int mois=month();
    int annee=year();
    
    
    sprintf(buf, "%d", heures);
    if (heures<10){
      heurez[0]='0';heurez[1]=buf[0];
      date_heure[11]='0';date_heure[12]=buf[0];
    }else {
      heurez[0]=buf[0];heurez[1]=buf[1];
      date_heure[11]=buf[0];date_heure[12]=buf[1];      
    }
    
    sprintf(buf, "%d", minutes);
    if (minutes<10){
      heurez[3]='0';heurez[4]=buf[0];
      date_heure[14]='0';date_heure[15]=buf[0];
    }else {
      heurez[3]=buf[0];heurez[4]=buf[1]; 
      date_heure[14]=buf[0];date_heure[15]=buf[1];       
    }
    
  


    //date en numerique

    sprintf(buf, "%d", jour_mois);
    if (jour_mois<10){
      date_heure[0]='0';date_heure[1]=buf[0];
      datez[0]='0';datez[1]=buf[0];
    }else {
      date_heure[0]=buf[0];date_heure[1]=buf[1];  
      datez[0]=buf[0];datez[1]=buf[1];     
    }

    sprintf(buf, "%d", mois);
    if (mois<10){
      date_heure[3]='0';date_heure[4]=buf[0];
      datez[3]='0';datez[4]=buf[0];
    }else {
      date_heure[3]=buf[0];date_heure[4]=buf[1];  
      datez[3]=buf[0];datez[4]=buf[1];       
    }

    sprintf(buf, "%d", annee);
    for(int i=0;i<4;i++) {
      date_heure[i+6]=buf[i];
      datez[i+6]=buf[i];
    }


} 
  
  
void aff_uptime() { 
     
        unsigned long resultat=0;
        unsigned long uptime;
        char chaine1[5]="   ";
        char buf[3]="  ";
        

        //recupe du temps en ms de l'uptime;
        uptime=(unsigned long) millis()/1000;
        
        if (uptime < 60){
          resultat=uptime; 
          sprintf(buf, "%d", resultat);
          chaine1[2]='s';
        }
        
        if (uptime>=60 && uptime<3600){ 
          resultat=(unsigned long) uptime/60;
          sprintf(buf, "%d", resultat);
          chaine1[0]=buf[0];
          chaine1[1]=buf[1];
          chaine1[2]='m'; 
        }
    
        if (uptime>=3600 && uptime<86400){ 
          resultat=(unsigned long) uptime/3600;
          sprintf(buf, "%d", resultat);
          chaine1[2]='H';  
        }
        
        if (uptime>=86400){ 
          resultat=(unsigned long) uptime/86400;
          sprintf(buf, "%d", resultat);
          chaine1[2]='J'; 
        }
        
        
       if (resultat<10){
          chaine1[0]='0';
          chaine1[1]=buf[0];            
       }else {
          chaine1[0]=buf[0];
          chaine1[1]=buf[1];
       } 
        

       sprintf(uptimez, "%s",chaine1);        
       sprintf(ram_uptime, "%d%s %s", freeMemory(),"_octets",chaine1);

} 



void update_lcd()  { 

   

    mydisp.setFont(fonts[1]); 
    mydisp.setColor(0x78);

    mydisp.setPrintPos(10, 1, _TEXT_);
    mydisp.print(temperature1);
    mydisp.setPrintPos(10, 2, _TEXT_);
    mydisp.print(temperature2);

    mydisp.setColor(0xFF);
    if(heure_pleine==0){
      mydisp.drawStr(10, 4, "HC");
    }else{
      mydisp.drawStr(10, 4, "HP");
    }
    
    mydisp.setColor(0xB7); 
    mydisp.setPrintPos(10, 6, _TEXT_);
    mydisp.print(temperature_consigne_virtuelle);
    if(relais_chauffage==0){
       mydisp.drawStr(10, 7, "NON");
    }else{
       mydisp.drawStr(10, 7, "OUI");
    }
 
  
    mydisp.setColor(0xF4);
    mydisp.drawStr(10, 11,uptimez);
  
  
    mydisp.setColor(0xF8); 
    mydisp.setFont(fonts[2]);
    mydisp.drawStr(0, 10,heurez); 
    mydisp.drawStr(0, 11,datez); 
  
}


  
void init_lcd()  {  
  
    resetpos1();
    
    mydisp.clearScreen();
    mydisp.setRot270(); 
    mydisp.setFont(fonts[1]); 
    mydisp.setColor(0x78);
    
    mydisp.drawStr(0, 1, "TMP BAS : ");
    mydisp.drawStr(0, 2, "TMP HAUT: ");

    mydisp.setColor(0xFF);
    mydisp.drawStr(0, 4, "HC/HP   : ");
  
    mydisp.setColor(0xB7);  
    mydisp.drawStr(0, 6, "CONSIGNE: ");
    mydisp.drawStr(0, 7, "CHAUFF. : "); 
  
    mydisp.setColor(0xF4);
    mydisp.drawStr(0, 11,"UPTIME  : ");
  
    mydisp.setColor(0xE8);  
    mydisp.drawStr(20, 18,"(c) AO 2013-2016");
  
    
    mydisp.setColor(0xFF); 
    mydisp.drawLine(95,0 ,95 ,150 );
    mydisp.drawLine(95,150 ,220 ,150 );
    mydisp.drawLine(95,75 ,220 ,75 );
    mydisp.setFont(fonts[1]);
    mydisp.drawStr(17, 9,"H");
    mydisp.drawStr(17, 0,"B");
  
}  
 

void decalage_stockage_temp()  { 

  for (int i = 1 ; i < 48 ; i++){
      haut[i-1]=haut[i];
      bas[i-1]=bas[i];
  } 
  bas[47]=temperature1;
  haut[47]=temperature2;
  
}


void graph_temp()  {  

  init_lcd();

  float somme=0;
  float moyenne=0;
  float mini=100;
  float maxi=0;
  float hauteur=0;
  int   offset=0;
  

  decalage_stockage_temp();
  

  envoie_data_temp();

  //bas
  for (int i = 0 ; i < 48 ; i++){
    somme += bas[i] ; 
    if(bas[i]>maxi){
      maxi=bas[i];
    }
    if(bas[i]<mini){
      mini=bas[i];
    }
  } 
  moyenne=round((somme/48)*10)/10;

  mib=mini;
  mab=maxi;
  moyb=moyenne;
   
  hauteur=round((65/(maxi-mini))*10)/10;
  offset=5; 
  mydisp.setColor(0xEC); 
  for(int i=2; i<47;i++){
    mydisp.drawLine(103+2*i, round((maxi-bas[i])*hauteur)+offset , 103+2*i+2 , round((maxi-bas[i+1])*hauteur)+offset  );   
  }
  mydisp.setColor(0xFF);
  //mydisp.drawLine(105, round((maxi-moyenne)*hauteur)+offset , 190 ,  round((maxi-moyenne)*hauteur)+offset );
  mydisp.setFont(fonts[1]);
  mydisp.setPrintPos(32, 3, _TEXT_);
  mydisp.print(moyenne,1);
  mydisp.setPrintPos(32, 7, _TEXT_);
  mydisp.print(mini,1);
  mydisp.setPrintPos(32, 0, _TEXT_);
  mydisp.print(maxi,1);

  somme=0;
  mini=100;
  maxi=0;
  moyenne=0;
  hauteur=0;
  
  //haut
  for (int i = 0 ; i < 48 ; i++){
    somme += haut[i] ; 
    if(haut[i]>maxi){
      maxi=haut[i];
    }
    if(haut[i]<mini){
      mini=haut[i];
    }
  } 
  moyenne=round((somme/48)*10)/10;

  mih=mini;
  mah=maxi;
  moyh=moyenne;
  
  hauteur=round((65/(maxi-mini))*10)/10;
  offset=80;
  mydisp.setColor(0xEA); 
  for(int i=2; i<47;i++){
    mydisp.drawLine(103+2*i, round((maxi-haut[i])*hauteur)+offset , 103+2*i+2 , round((maxi-haut[i+1])*hauteur)+offset  );   
  }
  mydisp.setColor(0xFF);
  //mydisp.drawLine(105, round((maxi-moyenne)*hauteur)+offset , 190 ,  round((maxi-moyenne)*hauteur)+offset );
  mydisp.setFont(fonts[1]);
  mydisp.setPrintPos(32, 12, _TEXT_);
  mydisp.print(moyenne,1);
  mydisp.setPrintPos(32, 15, _TEXT_);
  mydisp.print(mini,1);
  mydisp.setPrintPos(32, 9, _TEXT_);
  mydisp.print(maxi,1);
  


}


  
void resetpos1(void) 
{
  mydisp.setPrintPos(0, 0, _TEXT_);
  delay(1500); 
  mydisp.println("                "); 
  mydisp.setPrintPos(0, 0, _TEXT_);
} 
  

  
  



