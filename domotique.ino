  
///////////////////////////////
////  Alexandre OGER 2012  ////
///////////////////////////////

  // domotique maison 
  //gestion des volets et de la chaudiere 
  
  
  
  //Version courante: 1.3
  
  //**** Historique ************************************************************************************************************
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
  
  // deux cartes de 16 relais
  // deux sondes de temperature dallas onewire chacune sur un bus dedié (1 pin par bus)
  // une horloge temps reel a base de DS1307 communiquant par le bus I2C
  // un afficheur LCD 4 lignes 20 caracteres sur bus I2C
  // une sortie TV composite sur port serie n°=1 relié a un moniteur LCD 10 pouces
  // touchscreen resistif à 4 fils relié à 4 entrees/sortie analogiques (partie integrante du moniteur LCD 10 pouces)
  
  //utilisation l'eeprom interne de l'arduino pour stockage de toutes les données de programation volets et chauffage
  
  
  //mappage des pins:
  
  //connecteur haut gauche et milieu
  // 2     -> 1er bus onewire 
  // 3     -> 2eme bus onewire
  // 20-21   -> bus I2C pour l'horloge temps reel et l'afficheur LCD 
  //tout le connecteur droit vertical + pin21 sur le connecteur haut droit
  // 22-37 -> 1ere carte 16 relais
  // 38-49 et 66-69 -> 2eme carte 16 relais
  // A0 à A3 (54 à 57) touchscreen resistif


/*

                                                         ________ TX1____
                                                        | _______ RX1____|_____ port serie pour carte video TTY
                 bus onewire 1 _________                | |  ____ SDA ___
                 bus onewire 2 _______  |               | | | --- SCL ---|----- bus I2C (lcd 4x20 et horloge temps reel)
                                      | |               | | | |  
   ___________________________________________________________________________
  |             o o o o o o   o o o o o o o o   o o o o o o o o               |      ___
  |             1 1 1 1 9 8   7 6 5 4 3 2 1 0   1 1 1 1 1 1 2 2    22-23  o o |         |
  |             3 2 1 0                         4 5 6 7 8 9 0 1    24-25  o o |         |
  |                                                                26-27  o o |         |   Carte 16 relais n°1
  |---------                                                       28-29  o o |         |
  |         |                                                      30-31  o o |         |
  |  USB    |                                                      32-33  o o |         |
  |         |                                                      34-35  o o |         |
  |---------                ARDUINO   MEGA  2560                   36-37  o o |      ---
  |                                                                38-39  o o |         |
  |                                                                40-41  o o |         |
  |                                                                42-43  o o |         |   Carte 16 relais n°2
  |                                                                44-45  o o |         |      + pin A12 à A15 (66 à 69)
  |                                                                46-47  o o |         |   ^
  |                R     G G V                       A A A A A A   48-49  o o |      ---    |
  |---------       S 3 5 N N I  A A A A A A A A  A A 1 1 1 1 1 1   50-51  o o |             |
  |  VCC    |      T V V D D N  0 1 2 3 4 5 6 7  8 9 0 1 2 3 4 5   52-53  o o |             |
  |---------     o o o o o o o  o o o o o o o o  o o o o o o o o              |             |             
  |---------------------------------------------------------------------------              |
                                | | | |                  | | | |                            |
                                | | | |                  | | | |                            |
                                | | | |                  -------   suite carte relais n° 2  |
                                ------                      |_______________________________|
                       touchscreen resitif 4 fils          
                                               
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
  #include <HTI2CLCD.h> 
  
  // apple de la librairie de gestion du touchscreen du moniteur (resistif 4 fils)
  #include <TouchScreen.h>



  //
  //declarations variables et constantes
  //  
 
 
 
  //declaration des pins utilisés par le touchscreen A0 à A3 (54 à 57)
  #define YP A0 
  #define XM A3 
  #define YM 56 
  #define XP 55
 
  //init du touchscreen
  TouchScreen ts = TouchScreen(XP, YP, XM, YM, 770); 
 
  // init du lcd
  HTI2CLCD lcd;
  
  // adresse de l'afficheur LCD sur le bus i2c
  const int  lcd_address=58;  

 
  //definition des caracteres spécifiques à la sortie video pour l'appel des commandes
  #define CHAR_ESC 0x1B
  #define CHAR_DLE 0x10 

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
  
  // variable de stockage du menu courant et precedent
  int menu_courant=1;
  int menu_precedent=1;
  
  // declaration des relais de 1 à 32, 
  // carte1
  const int RELAIS1=22; 
  const int RELAIS2=23;
  const int RELAIS3=24;
  const int RELAIS4=25;
  const int RELAIS5=26;
  const int RELAIS6=27;
  const int RELAIS7=28;
  const int RELAIS8=29;
  const int RELAIS9=30;
  const int RELAIS10=31;
  const int RELAIS11=32;
  const int RELAIS12=33;
  const int RELAIS13=34;
  const int RELAIS14=35;
  const int RELAIS15=36;
  const int RELAIS16=37;
  //carte2
  const int RELAIS17=38;
  const int RELAIS18=39;
  const int RELAIS19=40;
  const int RELAIS20=41;
  const int RELAIS21=42;
  const int RELAIS22=43;
  const int RELAIS23=44;
  const int RELAIS24=45;
  const int RELAIS25=46;
  const int RELAIS26=47;
  const int RELAIS27=48; 
  const int RELAIS28=49; 
  const int RELAIS29=66;
  const int RELAIS30=67;
  const int RELAIS31=68;
  const int RELAIS32=69;
  
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
  int compteur=0;  
  int compteur_cligno=0;
  
  
  //variable de test de demarrage-> pour le lcd
  int debut=0;
    
  
  void setup()   { 
    
    //mise en marche du watchdog du processeur pour reset auto en cas de freeze de plus de 8s
    wdt_enable(WDTO_8S);
    
    //conf du port serie 0 pour l'affichage du deboggage
    Serial.begin(115200); 
    
    //conf du port serie 1 pour sortie TV <- carte Tellymate
    Serial1.begin(57600); 
    
    //efface ecran et curseur eteind
    Serial1.write( CHAR_ESC ) ;
    Serial1.write( 'E' ) ; // CLS
    Serial1.write( CHAR_ESC ) ;
    Serial1.write( 'f' ) ; // curseur off

    //demarrage du bus I2C
    delay(200);                                
    Wire.begin();   
   
    //initialisation des deux bus onewire
    initonewire(); 
  
    //definition du type d'afficheur lcd, de la puissance de retro eclairage
    lcd.setType(lcd_address,4,20); //4 lignes de 20 caracteres
    lcd.backLight(lcd_address,90); //luminosite led de retro eclairage
    lcd.clear(lcd_address);    
    
    
    //utilisation de la led interne de l'arduino
    pinMode (13,OUTPUT);
 
    // config des pins relais en output
     pinMode (RELAIS1,OUTPUT); 
     pinMode (RELAIS2,OUTPUT); 
     pinMode (RELAIS3,OUTPUT); 
     pinMode (RELAIS4,OUTPUT);
     pinMode (RELAIS5,OUTPUT); 
     pinMode (RELAIS6,OUTPUT); 
     pinMode (RELAIS7,OUTPUT); 
     pinMode (RELAIS8,OUTPUT); 
     pinMode (RELAIS9,OUTPUT);
     pinMode (RELAIS10,OUTPUT); 
     pinMode (RELAIS11,OUTPUT); 
     pinMode (RELAIS12,OUTPUT); 
     pinMode (RELAIS13,OUTPUT); 
     pinMode (RELAIS14,OUTPUT);
     pinMode (RELAIS15,OUTPUT); 
     pinMode (RELAIS16,OUTPUT); 
     pinMode (RELAIS17,OUTPUT); 
     pinMode (RELAIS18,OUTPUT); 
     pinMode (RELAIS19,OUTPUT);
     pinMode (RELAIS20,OUTPUT); 
     pinMode (RELAIS21,OUTPUT); 
     pinMode (RELAIS22,OUTPUT); 
     pinMode (RELAIS23,OUTPUT); 
     pinMode (RELAIS24,OUTPUT);
     pinMode (RELAIS25,OUTPUT); 
     pinMode (RELAIS26,OUTPUT); 
     pinMode (RELAIS27,OUTPUT); 
     pinMode (RELAIS28,OUTPUT); 
     pinMode (RELAIS29,OUTPUT);
     pinMode (RELAIS30,OUTPUT); 
     pinMode (RELAIS31,OUTPUT); 
     pinMode (RELAIS32,OUTPUT); 
  
    //init de tous les relais à zero (HIGH) <-  cartes relais fonctionnant en logique inversée
    digitalWrite(RELAIS1,HIGH);
    digitalWrite(RELAIS2,HIGH);
    digitalWrite(RELAIS3,HIGH);
    digitalWrite(RELAIS4,HIGH);
    digitalWrite(RELAIS5,HIGH); 
    digitalWrite(RELAIS6,HIGH);
    digitalWrite(RELAIS7,HIGH);
    digitalWrite(RELAIS8,HIGH);
    digitalWrite(RELAIS9,HIGH);
    digitalWrite(RELAIS10,HIGH); 
    digitalWrite(RELAIS11,HIGH);
    digitalWrite(RELAIS12,HIGH);
    digitalWrite(RELAIS13,HIGH);
    digitalWrite(RELAIS14,HIGH);
    digitalWrite(RELAIS15,HIGH); 
    digitalWrite(RELAIS16,HIGH);
    digitalWrite(RELAIS17,HIGH);
    digitalWrite(RELAIS18,HIGH);
    digitalWrite(RELAIS19,HIGH);
    digitalWrite(RELAIS20,HIGH); 
    digitalWrite(RELAIS21,HIGH);
    digitalWrite(RELAIS22,HIGH);
    digitalWrite(RELAIS23,HIGH);
    digitalWrite(RELAIS24,HIGH);
    digitalWrite(RELAIS25,HIGH); 
    digitalWrite(RELAIS26,HIGH);
    digitalWrite(RELAIS27,HIGH);
    digitalWrite(RELAIS28,HIGH);
    digitalWrite(RELAIS29,HIGH);
    digitalWrite(RELAIS30,HIGH); 
    digitalWrite(RELAIS31,HIGH);
    digitalWrite(RELAIS32,HIGH);
  
    //definition du timer de synchro avec l'horloge RTC + synchro toute les 2 heures
    setSyncProvider(RTC.get);
    setSyncInterval(7200); 
        
    
    //Serial.println("");
    //Serial.println("");
    //Serial.println("");
    
    if(timeStatus()== timeSet) {       
       //Serial.println("Horloge Synchonisee avec le module RTC");
       lcd_aff("Horloge synchro OK",2,1);
    } else {
       //Serial.println("Impossible de se synchroniser avec l'horloge");
       lcd_aff("Horloge non-synchro",2,1);
    }
    
 
    //declaration d'un timer toutes les 20sec pour le cadencement des taches
    Alarm.timerRepeat(20,Cadenceur); 
    
    //declaration d'un timer toutes les sec pour afficher l'heure sur le LCD et le moniteur
    Alarm.timerRepeat(1,cligno);
        
    //aff du splach screen + heure sur le LCD 
    lcd_aff("Systeme domotique",3,1);
    lcd_aff("(c) A. OGER 2012",4,1);   
    aff_heure();
    aff_uptime();
    
    //aff du menu d'accueil sur la sortie composite
    update_menu(); 
    
    
  
  } 
    
    
  
  void loop(){  

    //trigger de declenchement de verif des timers d'alarme
    Alarm.delay(1000);
    
    //reset du watchdog pour indiquer que le script est en vie
    wdt_reset();
    
    //aquisition des coordonnees sur le touch screeen
    Point p = ts.getPoint(); 
   
   //detection d'un appui quelquonque sur l'ecran et traitement par verif_click  
   if (p.x > 50 && p.y> 50) {
     //Serial.print("X = "); Serial.print(p.x);
     //Serial.print("\tY = "); Serial.println(1023-p.y);
     verif_click(p.x,1023-p.y);
    
    }
  
  } 
  

//fx d'affichage du fix pour  
void print_float(float f, int num_digits)
{
    int f_int;
    int pows_of_ten[4] = {1, 10, 100, 1000};
    int multiplier, whole, fract, d, n;

    multiplier = pows_of_ten[num_digits];
    if (f < 0.0)
    {
        f = -f;
        Serial1.print(F("-"));
    }
    whole = (int) f;
    fract = (int) (multiplier * (f - (float)whole));

    Serial1.print(whole);
    Serial1.print(F("."));

    for (n=num_digits-1; n>=0; n--) 
    {
         d = fract / pows_of_ten[n];
         Serial1.print(d);
         fract = fract % pows_of_ten[n];
    }
}

  
void verif_click(int x,int y){ 
 
    //pour les menus volet et prog volet: tableau des coordonnes des boutons volets (ref x à gauche de chaque btn);
    int coordonnees[]={145,225,305,385,465,550,630,710,790};  

    //pour le menu  prog chauffage: tableau des coordonnes des boutons journaliers
    int coordonnees_jours[]={145,225,305,385,465,550,630}; 
    
    //pour le menu prog volet
    int adresse_debut_volet;
    int prog_manu;
    int mode_auto;
    
    int i;
    
    switch (menu_courant) {
     
      //menu accueil
      case 1: 
        // btn temp:-      
        if ( ( x>170 && x<230) && ( y>360 && y<450 ) ) { consigne_temperature--; is_heure_creuse(0);moniteur_dplcmnt(23,1); Serial1.print(F("Bouton temp - 1 deg ")); }
        // btn temp:+
        if ( ( x>360 && x<440) && ( y>360 && y<450 ) ) { consigne_temperature++;  is_heure_creuse(0);moniteur_dplcmnt(23,1); Serial1.print(F("Bouton temp + 1 deg "));  }
        // btn hp:O
        if ( ( x>170 && x<230) && ( y>280 && y<350 ) ) { heure_pleine=1;is_heure_creuse(1); }
        // btn hp:n
        if ( ( x>360 && x<440) && ( y>280 && y<350 ) ) { heure_pleine=0;is_heure_creuse(2); }
        // btn Volets
        if ( ( x>630 && x<880) && ( y>270 && y<360 ) ) { menu_courant=2; }
        // btn prog volets
        if ( ( x>630 && x<880) && ( y>410 && y<505 ) ) { menu_courant=3; }
        // btn prog chauffage
        if ( ( x>630 && x<880) && ( y>550 && y<650 ) ) { menu_courant=5; }
        menu_precedent=1;
        update_menu();
        aff_heure();
        aff_uptime();
        break;
    
      //menu volets
      case 2:
        // btn Retour
        if ( ( x>130 && x<330) && ( y>220 && y<310 ) ) { menu_courant=1; }
        
        //boutons volets detection des X  par boucle de 1 à 9 puis des y de 1 à 3 puis action sur le volet correspondant
        for ( i= 0; i<10; i++) {         
           if ( x>coordonnees[i] && x<(coordonnees[i]+65) ) {            
             if ( y>365 && y<445 ) {action_volet(i+1,'H');moniteur_dplcmnt(23,1); Serial1.print(F("Volet: "));Serial1.print(i+1);Serial1.print(F(" sens: Haut "));}
             if ( y>446 && y<519 ) {action_volet(i+1,'S');moniteur_dplcmnt(23,1); Serial1.print(F("Volet: "));Serial1.print(i+1);Serial1.print(F(" sens: Stp  "));}
             if ( y>520 && y<595 ) {action_volet(i+1,'B');moniteur_dplcmnt(23,1); Serial1.print(F("Volet: "));Serial1.print(i+1);Serial1.print(F(" sens: Bas  "));}  
           }          
        }
        menu_precedent=2;
        update_menu();
        aff_heure();
        aff_uptime();
        break;
       
      //menu prog volets
      case 3:
        // btn Retour
        if ( ( x>130 && x<330) && ( y>220 && y<310 ) ) { menu_courant=1; }
        // btn horloge
        if ( ( x>610 && x<910) && ( y>220 && y<310 ) ) { menu_courant=4; }
        //boutons volets detection des X  par boucle de 1 à 9 puis des y  -> volet correspondant
        for ( i= 0; i<10; i++) {         
           if ( x>coordonnees[i] && x<(coordonnees[i]+65) ) {            
             if ( y>365 && y<445 ) {volet_courant_prog=i+1;moniteur_dplcmnt(23,1); Serial1.print(F("Volet: "));Serial1.print(i+1);Serial1.print(F(" prog       "));}
           }          
        }
        
        if (volet_courant_prog!=0){
          adresse_debut_volet=100+(volet_courant_prog-1)*6;
          prog_manu=EEPROM.read(adresse_debut_volet);
          mode_auto=EEPROM.read(adresse_debut_volet+1);
         }  
         
        //si volet courant!=0 et l'un des trois boutons on modifie les valeurs de l'eeprom en consequence puis update menu
        if (volet_courant_prog!=0){
          //btn manu
          if ( ( x>150 && x<305) && ( y>490 && y<570 ) ) { EEPROM.write(adresse_debut_volet,0);EEPROM.write(adresse_debut_volet+1,1); }
          //btn auto
          if ( ( x>310 && x<465) && ( y>490 && y<570 ) ) { EEPROM.write(adresse_debut_volet,1);EEPROM.write(adresse_debut_volet+1,1); }          
          //btn perso
          if ( ( x>475 && x<650) && ( y>490 && y<570 ) ) { EEPROM.write(adresse_debut_volet,1);EEPROM.write(adresse_debut_volet+1,0); }
        }
        
        //si prog_manu=1 et mode_auto=0 gestion des btn + et - pour les heure de montee et descente
        if(volet_courant_prog!=0 && prog_manu==1 && mode_auto==0) {
          //btn - montee
          if ( ( x>655 && x<710) && ( y>490 && y<560 ) ) { modif_prog_perso_volet('M',volet_courant_prog,'-');}          
          //btn + montee
          if ( ( x>815 && x<880) && ( y>490 && y<560 ) ) { modif_prog_perso_volet('M',volet_courant_prog,'+'); }              
          //btn - descente
          if ( ( x>655 && x<710) && ( y>565 && y<640 ) ) { modif_prog_perso_volet('D',volet_courant_prog,'-'); }  
          //btn + descente
          if ( ( x>815 && x<880) && ( y>565 && y<640 ) ) { modif_prog_perso_volet('D',volet_courant_prog,'+'); }    
 
           //btn ok/valider
          //if ( ( x>0 && x<0) && ( y>0 && y<0 ) ) { }          
        }   
        menu_precedent=3;
        update_menu();
        aff_heure();
        aff_uptime();
        break;
       
      //menu horloge
      case 4:
        // btn Retour
        if ( ( x>130 && x<330) && ( y>220 && y<310 ) ) { menu_courant=3 ; }
        
        // - heures
        if ( ( x>245 && x<315) && ( y>400 && y<470 ) ) { if (heure_en_cours_prog!=0) {heure_en_cours_prog--;} else {heure_en_cours_prog=23;}  }
        // + heures
        if ( ( x>350 && x<415) && ( y>400 && y<470 ) ) { if (heure_en_cours_prog!=23) {heure_en_cours_prog++;} else {heure_en_cours_prog=0;}  }
        // - minutes
        if ( ( x>430 && x<500) && ( y>400 && y<470 ) ) { if (minute_en_cours_prog!=0) {minute_en_cours_prog--;} else {minute_en_cours_prog=59;}  }
        // + minutes
        if ( ( x>530 && x<600) && ( y>400 && y<470 ) )  { if (minute_en_cours_prog!=59) {minute_en_cours_prog++;} else {minute_en_cours_prog=0;}  }
        // - secondes
        if ( ( x>610 && x<680) && ( y>400 && y<470 ) ) { if (seconde_en_cours_prog!=0) {seconde_en_cours_prog--;} else {seconde_en_cours_prog=59;}  }
        // + secondes
        if ( ( x>675 && x<780) && ( y>400 && y<470 ) ) { if (seconde_en_cours_prog!=59) {seconde_en_cours_prog++;} else {seconde_en_cours_prog=00;}  }
        
        // btn valider
        if ( ( x>410 && x<620) && ( y>510 && y<585 ) ) { setTime(heure_en_cours_prog,minute_en_cours_prog,seconde_en_cours_prog,day(),month(),year()); RTC.set(now());  menu_courant=3 ;} 
        
        menu_precedent=4;
        update_menu();
        aff_heure();
        aff_uptime();
        break;
       
      //menu prog chauffage
      case 5:
        // btn Retour
        if ( ( x>130 && x<330) && ( y>220 && y<310 ) ) { menu_courant=1;jour_courant_prog=0; }
        //boutons volets detection des X  par boucle de 1 à 9 puis des y  -> volet correspondant
        for ( i= 0; i<7; i++) {         
           if ( x>coordonnees_jours[i] && x<(coordonnees_jours[i]+65) ) {            
             if ( y>345 && y<430 ) {jour_courant_prog=i+1;moniteur_dplcmnt(23,1); Serial1.print(F("Jour: "));Serial1.print(i+1);Serial1.print(F(" prog        "));}
           }          
        }
        
        if (jour_courant_prog!=0){
          for ( i= 0; i<3; i++) { 
              //btn - debut
              if ( ( x>390 && x<450) && ( y>445+i*70 && y<515+i*70 ) ) { modif_prog_chauff(jour_courant_prog,i+1,'D','-');}          
              //btn + debut
              if ( ( x>550 && x<615) && ( y>445+i*70 && y<515+i*70 ) ) { modif_prog_chauff(jour_courant_prog,i+1,'D','+');}            
              //btn - fin
              if ( ( x>630 && x<695) && ( y>445+i*70 && y<515+i*70 ) ) { modif_prog_chauff(jour_courant_prog,i+1,'F','-');}   
              //btn + fin
              if ( ( x>790 && x<860) && ( y>445+i*70 && y<515+i*70 ) ) { modif_prog_chauff(jour_courant_prog,i+1,'F','+');}     
          }
        }
         // btn prog global on
        if ( ( x>655 && x<755) && ( y>235 && y<305 ) ) { EEPROM.write(5,1); }
         // btn prog global off
        if ( ( x>775 && x<900) && ( y>235 && y<305 ) ) { EEPROM.write(5,0); }        
        
        menu_precedent=5;
        update_menu();
        aff_heure();
        aff_uptime();
        break;
    }
    
  }

  
  void Cadenceur(){ 
    
    //RAZ du compteur
    if (compteur==3) {
      compteur=0;
    } 

    //reset de la ligne1 du LCD lors du tout premier demarrage
    if(debut==0){
       lcd_aff("                    ",2,1);
       debut=1; 
    } 

    //Serial.print("check numero: ");
    //Serial.print("  ");
    //Serial.println(compteur);
      
    switch (compteur) {
      case 0:
        //Serial.println("check volets et chauff."); 
        lcd_aff("VC1",1,18);
        moniteur_dplcmnt(0,0);
        Serial1.print(F("Busy"));
        check_heure_ete();
        check_volets();
        check_chauffage();
        moniteur_dplcmnt(0,0);
        Serial1.print(F("    "));
        break;
      case 1:
        //Serial.println("check volets et chauffage");
        lcd_aff("VC2",1,18);
        moniteur_dplcmnt(0,0);
        Serial1.print(F("Busy"));
        check_volets();
        check_chauffage();
        moniteur_dplcmnt(0,0);
        Serial1.print(F("    "));
        break;
      case 2:
        //Serial.println("check chauffage");
        lcd_aff(" C3",1,18);
        moniteur_dplcmnt(0,0);
        Serial1.print(F("Busy"));
        check_chauffage();
        moniteur_dplcmnt(0,0);
        Serial1.print(F("    "));
        break;
 
    }
    
    update_menu();
    //increment du compteur
    compteur++;  
    //affichage de l'heure sur le LCD
    aff_heure();
    //affichage de l'uptime sur le LCD
    aff_uptime();

  }
  
  void modif_prog_chauff(int jour_courant,int plage,char moment,char increment){  
    moniteur_dplcmnt(23,1); Serial1.print(F("plage "));Serial1.print(jour_courant);Serial1.print(plage);Serial1.print(moment);Serial1.print(increment);Serial1.print("           ");
    int debut_plage_memoire=10+(jour_courant-1)*12;
    int heure_debut=EEPROM.read(debut_plage_memoire+(plage-1)*4);
    int minute_debut=EEPROM.read(debut_plage_memoire+(plage-1)*4+1);
    int heure_fin=EEPROM.read(debut_plage_memoire+(plage-1)*4+2);
    int minute_fin=EEPROM.read(debut_plage_memoire+(plage-1)*4+3);
  
    if(moment=='D'){
        if(increment=='-'){ 
              if(minute_debut==0){
                  EEPROM.write(debut_plage_memoire+(plage-1)*4+1,45);
                  if(heure_debut==0){
                      EEPROM.write(debut_plage_memoire+(plage-1)*4,23); 
                  } else  {
                      EEPROM.write(debut_plage_memoire+(plage-1)*4,heure_debut-1); 
                  }                 
              } else  {
                 EEPROM.write(debut_plage_memoire+(plage-1)*4+1,minute_debut-15); 
              }
        }
        if(increment=='+'){
              if(minute_debut>=45){
                  EEPROM.write(debut_plage_memoire+(plage-1)*4+1,0);
                  if(heure_debut==23){
                      EEPROM.write(debut_plage_memoire+(plage-1)*4,0); 
                  } else  {
                      EEPROM.write(debut_plage_memoire+(plage-1)*4,heure_debut+1); 
                  }                 
              } else  {
                 EEPROM.write(debut_plage_memoire+(plage-1)*4+1,minute_debut+15); 
              } 
        }
    }
    
    if(moment=='F'){
        if(increment=='-'){ 
              if(minute_fin==0){
                  EEPROM.write(debut_plage_memoire+(plage-1)*4+3,45);
                  if(heure_fin==0){
                      EEPROM.write(debut_plage_memoire+(plage-1)*4+2,23); 
                  } else  {
                      EEPROM.write(debut_plage_memoire+(plage-1)*4+2,heure_fin-1); 
                  }                 
              } else  {
                 EEPROM.write(debut_plage_memoire+(plage-1)*4+3,minute_fin-15); 
              }
        }
        if(increment=='+'){
              if(minute_fin>=45){
                  EEPROM.write(debut_plage_memoire+(plage-1)*4+3,0);
                  if(heure_fin==23){
                      EEPROM.write(debut_plage_memoire+(plage-1)*4+2,0); 
                  } else  {
                      EEPROM.write(debut_plage_memoire+(plage-1)*4+2,heure_fin+1); 
                  }                 
              } else  {
                 EEPROM.write(debut_plage_memoire+(plage-1)*4+3,minute_fin+15); 
              } 
        }
    }
 }
  
  
  void modif_prog_perso_volet(char sens,int volet,char increment){  
  
      moniteur_dplcmnt(23,1); Serial1.print("btn ");Serial1.write(sens);Serial1.write(volet);Serial1.write(increment);Serial1.print(F("             "));
      
      int adresse_debut_volet=100+(volet-1)*6;
      if (sens=='M'){
          int heure_monte=EEPROM.read(adresse_debut_volet+2);
          int minute_monte=EEPROM.read(adresse_debut_volet+3);
          if (increment=='-'){
              if (minute_monte!=0){
                  minute_monte=minute_monte-5;
              } else {
                  minute_monte=55;
                  if( heure_monte!=0){
                      heure_monte--;
                  } else {
                      heure_monte=23;
                  }
              }  
          } else {
              if (minute_monte!=55){
                  minute_monte=minute_monte+5;
              } else {
                  minute_monte=0;
                  if( heure_monte!=23){
                      heure_monte++;
                  } else {
                      heure_monte=0;
                  }
              }       
          }
          EEPROM.write(adresse_debut_volet+2,heure_monte);
          EEPROM.write(adresse_debut_volet+3,minute_monte);  
      } else {
          int heure_descente=EEPROM.read(adresse_debut_volet+4);
          int minute_descente=EEPROM.read(adresse_debut_volet+5);  
          if (increment=='-'){
              if (minute_descente!=0){
                  minute_descente=minute_descente-5;
              } else {
                  minute_descente=55;
                  if( heure_descente!=0){
                      heure_descente--;
                  } else {
                      heure_descente=23;
                  }
              }  
          } else {
              if (minute_descente!=55){
                  minute_descente=minute_descente+5;
              } else {
                  minute_descente=0;
                  if( heure_descente!=23){
                      heure_descente++;
                  } else {
                      heure_descente=0;
                  }
              }       
          }
          EEPROM.write(adresse_debut_volet+4,heure_descente);
          EEPROM.write(adresse_debut_volet+5,minute_descente); 
         
      }      
   
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
        
        //Serial.print("Volet:");
        //Serial.println(volet);
        adresse_debut_volet=100+(volet-1)*6;
        prog_manu=EEPROM.read(adresse_debut_volet);
       
        //test prog oui/non
        if (prog_manu==1) {              
            //test mode auto oui/non
            mode_auto=EEPROM.read(adresse_debut_volet+1);
            if (mode_auto==1) { 
                //Serial.println("--Prog Auto--");  
                
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

                 //Serial.print(" montee->");Serial.print(heure_monte);Serial.print(":");Serial.println(minute_monte);
                 //Serial.print(" descente->");Serial.print(heure_descente);Serial.print(":");Serial.println(minute_descente);
                

           
                if(heure_monte==heure_courante && minute_monte==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                      action_volet(volet,'H');
                      //Serial.println("Ce volet monte");
                    }
                }  
             
                if(heure_descente==heure_courante && minute_descente==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                     action_volet(volet,'B'); 
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

                 //Serial.print(" montee->");Serial.print(heure_monte);Serial.print(":");Serial.println(minute_monte);
                 //Serial.print(" descente->");Serial.print(heure_descente);Serial.print(":");Serial.println(minute_descente);
                 
           
                if(heure_monte==heure_courante && minute_monte==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                       action_volet(volet,'H');
                       //Serial.println("Ce volet monte");
                    }
                }  
             
                if(heure_descente==heure_courante && minute_descente==minute_courante ) {
                    if(heure_courante==0 && minute_courante==0){
                    } else {
                       action_volet(volet,'B');
                      //Serial.println("Ce volet descend");
                    }
                }  
  
              
                
            }
    
        } else {  

            //Serial.println("%%mode manuel%%");
            

        }
    }
    
   //Serial.println("");
   //Serial.println("");
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
    
    //Serial.println("");
    //Serial.println("");
    //Serial.println("Check chauffage");
    //Serial.print("Heure:");
    //Serial.print(hour());
    //Serial.print(":");
    //Serial.print(minute());
    //Serial.print(":");
    //Serial.println(second());  
    
    prog_chauffage=EEPROM.read(5);
    
    if (prog_chauffage==1) {
      //Serial.println("mode prog. du chauffage - actif");
      lcd_aff("Ch:Oui",2,8);
      
      //verif si heure pleine et adaptation de la consigne en consequence
      is_heure_creuse(0);
      
      //recup des données de temperature sur les capteurs
      retourne_temp(1);
      retourne_temp(2);
      //Serial.println("Interogation capteurs de temperature");
      //Serial.print("capteur 1: ");
      //Serial.println(temperature1);
      //Serial.print("capteur 2: ");
      //Serial.println(temperature2);

      
      dtostrf(temperature1, 2, 1, aff);
      dtostrf(temperature2, 2, 1, aff2);
      lcd_aff("                    ",3,1);
      
      sprintf(buf, "C:%d T1:%s T2:%s",temperature_consigne_virtuelle,aff,aff2 );
      lcd_aff(buf,3,1);
      
      //hysteresis de declanchement du chauffage
      //differenciation du comportement entre heures pleines et creuses
      if(heure_pleine==1){
          if (temperature1>=(temperature_consigne_virtuelle+0.5)){
            digitalWrite(RELAIS32,HIGH);
            digitalWrite(RELAIS31,HIGH);
            lcd_aff("Relais chauffage OFF ",4,1);
            relais_chauffage=0;
          }          
          if (temperature1<=(temperature_consigne_virtuelle-0.5)){
            digitalWrite(RELAIS32,LOW);
            digitalWrite(RELAIS31,LOW);
            lcd_aff("Relais chauffage ON ",4,1);
            relais_chauffage=1;
          }
       } else  {
          if (temperature1>(temperature_consigne_virtuelle+0.5)){
            digitalWrite(RELAIS32,HIGH);
            digitalWrite(RELAIS31,HIGH);
            lcd_aff("Relais chauffage OFF ",4,1);
            relais_chauffage=0;
          }         
          if (temperature1<=(temperature_consigne_virtuelle)){
            digitalWrite(RELAIS32,LOW);
            digitalWrite(RELAIS31,LOW);
            lcd_aff("Relais chauffage ON ",4,1);
            relais_chauffage=1;
          }         
       }
      
    } else {
      lcd_aff("Ch:Non",2,8);
      relais_chauffage=2; 
    }
    

  }
  

 
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
             moniteur_dplcmnt(23,1); 
             Serial1.print(F("Hp/Hc: "));
             Serial1.print(heure_pleine_forcee_heure_max);
             Serial1.print(F(" <-> "));
             Serial1.print(horaire_courant);
             if ( horaire_courant==heure_pleine_forcee_heure_max ) {  
                       heure_pleine_forcee=0;
                       heure_pleine_forcee_heure_max=0;
                       moniteur_dplcmnt(23,1); 
                       Serial1.print(heure_pleine_forcee_heure_max);
                       Serial1.print(F(" fin forcage HP"));
                                        
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
          lcd_aff("Hp:FoO",2,1);         
          moniteur_dplcmnt(23,1);
          Serial1.print(F("HP oui jusqu'a "));         
          Serial1.print(int(heure_pleine_forcee_heure_max/100));
          Serial1.print(F(":"));
          Serial1.print(heure_pleine_forcee_heure_max-int(heure_pleine_forcee_heure_max/100)*100);
       }
       
       if (type_forcage==2) {
          heure_pleine=0;
          lcd_aff("Hp:FoN",2,1);         
          moniteur_dplcmnt(23,1);
          Serial1.print(F("HP non jusqu'a "));
          Serial1.print(int(heure_pleine_forcee_heure_max/100));
          Serial1.print(F(":"));
          Serial1.print(heure_pleine_forcee_heure_max-int(heure_pleine_forcee_heure_max/100)*100);
       }
       
       if (type_forcage==0 && heure_pleine_forcee==0) {
             //on passe en revue chacune des 3 plages
             moniteur_dplcmnt(23,1); 
             Serial1.print(F("                    "));
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
                       lcd_aff("Hp:Oui",2,1);
                       break;                 
                   } else {
                       heure_pleine=0;
                       //Serial.println("heure pleine POK");
                       lcd_aff("Hp:Non",2,1);
                   }
             }           
       }
       
      if(heure_pleine==1){
          temperature_consigne_virtuelle=consigne_temperature;
      } else {
          temperature_consigne_virtuelle=consigne_temperature-2;
      }
  
   }
  

   
  
  void action_volet(int volet,char action){
    
    int delais_tempo_volet=300;
    int num_relais;
    num_relais=volet*3-2;
    
    switch (action) {
      case 'H':
        num_relais=num_relais;
        break;
      case 'S':
        num_relais=num_relais+1;
        break;
      case 'B':
        num_relais=num_relais+2;
        break;
     }
     
    lcd_aff("ok",4,19);
    
    switch (num_relais) {
      case 1:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS1,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS1,HIGH);
        break;
      case 2:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS2,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS2,HIGH);
        break;
      case 3:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS3,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS3,HIGH);
        break;
      case 4:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS4,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS4,HIGH);
        break;
      case 5:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS5,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS5,HIGH);
        break;
       case 6:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS6,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS6,HIGH);
        break;
       case 7:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS7,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS7,HIGH);
        break;
       case 8:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS8,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS8,HIGH);
        break;
      case 9:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS9,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS9,HIGH);
        break;
      case 10:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS10,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS10,HIGH);
        break;
      case 11:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS11,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS11,HIGH);
        break;
      case 12:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS12,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS12,HIGH);
        break;      
      case 13:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS13,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS13,HIGH);
        break;
      case 14:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS14,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS14,HIGH);
        break;
      case 15:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS15,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS15,HIGH);
        break;
      case 16:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS16,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS16,HIGH);
        break;      
      case 17:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS17,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS17,HIGH);
        break;      
      case 18:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS18,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS18,HIGH);
        break;      
      case 19:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS19,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS19,HIGH);
        break;
      case 20:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS20,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS20,HIGH);
        break;
      case 21:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS21,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS21,HIGH);
        break;
      case 22:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS22,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS22,HIGH);
        break;
      case 23:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS23,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS23,HIGH);
        break;
      case 24:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS24,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS24,HIGH);
        break;      
      case 25:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS25,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS25,HIGH);
        break;
      case 26:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS26,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS26,HIGH);
        break;
      case 27:
        delay(delais_tempo_volet);
        digitalWrite(RELAIS27,LOW);
        delay(delais_tempo_volet);
        digitalWrite(RELAIS27,HIGH);
        break;
        
     }  
    
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
	    Serial.println(F("CRC CAPTEUR BUS 1 INVALIDE"));
        return;
      }
	//verif de l'id du capteur 1 sur le bus, une id commencant par 0x28 correspond ‡ un DS18S20
      if ( addr1[0] != 0x28){
	    Serial.println(F("CAPTEUR BUS 1 NON CONFORME"));
        return;
      }
      
	//verif au cas ou le crc d'id du capteur 2 est faux 
	if ( OneWire::crc8( addr2, 7) != addr2[7] ) { 
	    Serial.println(F("CRC CAPTEUR BUS 2 INVALIDE"));
        return;
      }
	//verif de l'id du capteur 2 sur le bus, une id commencant par 0x28 correspond ‡ un DS18S20
      if ( addr2[0] != 0x28){
	    Serial.println(F("CAPTEUR BUS 2 NON CONFORME"));
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
        delay(450);                           //delais d'attente prise de mesure sur 11 bits
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
        delay(450);                           //delais d'attente prise de mesure sur 11 bits
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
    //Serial.print("Capteur: ");
    //Serial.print(num_capteur);
    //Serial.print(" -> temperature: ");
    //Serial.println(temp_tempo);
    if (num_capteur==1) { 
      temperature1=temp_tempo;
    }
    if (num_capteur==2) { 
      temperature2=temp_tempo;
    }
  
  }
  
  
  void aff_heure() {
    char ligne[10]="         ";
    char buf[3]="  ";
    char jour[9]="        ";

    int heures=hour();
    int minutes=minute();
    int jour_courant=weekday()-1;
    if (jour_courant==0) {jour_courant=7;}
    int jour_mois=day();
    int mois=month();
    int annee=year();
    
    ligne[0]='-';ligne[1]='-';ligne[4]=':';ligne[7]='-';ligne[8]='-';
    
    sprintf(buf, "%d", heures);
    if (heures<10){
      ligne[2]='0';ligne[3]=buf[0];
    }else {
      ligne[2]=buf[0];ligne[3]=buf[1];      
    }
    
    sprintf(buf, "%d", minutes);
    if (minutes<10){
      ligne[5]='0';ligne[6]=buf[0];
    }else {
      ligne[5]=buf[0];ligne[6]=buf[1];      
    }
    
    lcd_aff(ligne,1,6);       
    moniteur_dplcmnt(0,29) ;
    Serial1.print(ligne) ;
   
    //jour de la semaine
    moniteur_dplcmnt(21,1); //jour de la semaine
    switch (jour_courant) {
      case 1:
        Serial1.print(F("Lundi   ")) ;
        break; 
      case 2:
        Serial1.print(F("Mardi   ")) ;
        break; 
      case 3:
        Serial1.print(F("Mercredi")) ;
        break; 
      case 4:
        Serial1.print(F("Jeudi   ")) ;
        break; 
      case 5:
        Serial1.print(F("Vendredi")) ;
        break; 
      case 6:
        Serial1.print(F("Samedi  ")) ;
        break; 
      case 7:
        Serial1.print(F("Dimanche")) ;
        break; 
    } 
    //date en numerique
    moniteur_dplcmnt(21,11);
    if (jour_mois<10) {
     Serial1.print("0") ;
    } 
    Serial1.print(jour_mois) ;
    Serial1.print("/") ;
    if (mois<10) {
     Serial1.print("0") ;
    }
    Serial1.print(mois) ;
    Serial1.print("/") ;
    Serial1.print(annee) ;
   } 
  
  
  
  
   void aff_uptime() { 
     
        unsigned long resultat=0;
        unsigned long uptime;
        char chaine1[4]="   ";
        char buf[3]="  ";
        
       
        
        //recupe du temps en ms de l'uptime;
        uptime=(unsigned long) millis()/1000;
        
        if (uptime < 100){
          resultat=uptime; 
          sprintf(buf, "%d", resultat);
          chaine1[2]='s';
        }
        
        if (uptime>=100 && uptime<6000){ 
          resultat=(unsigned long) uptime/60;
          sprintf(buf, "%d", resultat);
          chaine1[0]=buf[0];
          chaine1[1]=buf[1];
          chaine1[2]='m'; 
        }
    
        if (uptime>=6000 && uptime<86400){ 
          resultat=(unsigned long) uptime/3600;
          sprintf(buf, "%d", resultat);
          chaine1[2]='h';  
        }
        
        if (uptime>=86400){ 
          resultat=(unsigned long) uptime/86400;
          sprintf(buf, "%d", resultat);
          chaine1[2]='j'; 
        }
        
        
       if (resultat<10){
          chaine1[0]='0';
          chaine1[1]=buf[0];            
       }else {
          chaine1[0]=buf[0];
          chaine1[1]=buf[1];
       } 
        
        lcd_aff("Up:",2,15);
        lcd_aff(chaine1,2,18);
        moniteur_dplcmnt(24,23) ;
        Serial1.print(F("Up:")) ;
        Serial1.print(chaine1);
        Serial1.print(F(" Ram:"));
        Serial1.print(freeMemory());
   } 
  
  
  
  
  
  void lcd_aff(char ligne[21],int x,int y)  {  
    int delay_lcd=50;    
    //Serial.println(ligne);
    lcd.setCursor(lcd_address,x,y);    
    lcd.print(lcd_address,ligne);

  
  }
  
  
  
  void cligno()  {    
    int delay_lcd=50;    
        
    //lcd.setCursor(0,0);
    delay(delay_lcd); 
   
    switch (compteur_cligno) {
      case 0:
        compteur_cligno=1;
        lcd_aff(">",1,1);
        moniteur_dplcmnt(0,33) ;
        Serial1.print(":");
        digitalWrite(13,LOW);
        break; 
      case 1:
        compteur_cligno=0;
        lcd_aff(" ",1,1);
        digitalWrite(13,HIGH);
        moniteur_dplcmnt(0,33) ;
        Serial1.print(" ");
        break; 
    } 
  
  }
  
  
 void moniteur_dplcmnt( uint8_t row , uint8_t col )
{ // <ESC>Yrc
  //max 25lignes
  //max de 38 caracteres
  Serial1.write( CHAR_ESC ) ;
  Serial1.write( 'Y' ) ;
  Serial1.write( 32 + row ) ;
  Serial1.write( 32 + col ) ;
} 


    
  void update_menu()  {   
   
    if (menu_courant != menu_precedent) {   
      Serial1.write( CHAR_ESC ) ;
      Serial1.write( 'E' ) ; // CLS
    }
    moniteur_dplcmnt(0,10) ;
    
    

    switch (menu_courant) {
      case 1:
        dessine_menu_accueil();
        break; 
      case 2:
        dessine_menu_volets();
        break; 
      case 3:
        dessine_menu_prog_volets();
        break; 
      case 4:
        dessine_menu_horloge();
        break; 
      case 5:
        dessine_menu_prog_chauff();
        break; 
    } 

   
  }
  
  
  void dessine_menu_accueil(){ 
     menu_precedent=1;
     int i;     
    //entete et pied de page domotique   
    moniteur_dplcmnt(0,12);
    Serial1.write(201);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(187);
    moniteur_dplcmnt(1,12);
    Serial1.write(186);
    Serial1.print(F("  DOMOTIQUE  "));
    Serial1.write(186);
    moniteur_dplcmnt(2,12);
    Serial1.write(200);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(188);
    moniteur_dplcmnt(22,0);
    Serial1.write(218);
    for ( i = 0; i < 20; i++) {
      Serial1.write(196);
    }
    Serial1.write(191); 
    moniteur_dplcmnt(23,0); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,0); 
    Serial1.write(179);
    moniteur_dplcmnt(23,21); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,21); 
    Serial1.write(179); 
    //fin entete et pied
    
    //heures pleines + -
    moniteur_dplcmnt(5,2); 
    Serial1.write(218); 
    Serial1.write(196);
    Serial1.write(191);
    moniteur_dplcmnt(5,12);
    Serial1.write(218); 
    Serial1.write(196);
    Serial1.write(191);
    moniteur_dplcmnt(6,2); 
    Serial1.write(179);
    Serial1.write('P');
    Serial1.write(179);
    if(heure_pleine==0){
      Serial1.print(F("Creuse-"));
    }else{
      Serial1.print(F("Pleine+"));
    }
    Serial1.write(179);
    Serial1.write('C');
    Serial1.write(179);
    moniteur_dplcmnt(7,2);
    Serial1.write(192);
    Serial1.write(196);
    Serial1.write(217);
    moniteur_dplcmnt(7,12);
    Serial1.write(192);
    Serial1.write(196);
    Serial1.write(217);
    
 
 
     //temp + -
    moniteur_dplcmnt(9,2); 
    Serial1.write(218); 
    Serial1.write(196);
    Serial1.write(191);
    moniteur_dplcmnt(9,12);
    Serial1.write(218); 
    Serial1.write(196);
    Serial1.write(191);
    moniteur_dplcmnt(10,2); 
    Serial1.write(179);
    Serial1.write('-');
    Serial1.write(179);
    Serial1.print(F("Temp:"));
    Serial1.print(temperature_consigne_virtuelle);
    moniteur_dplcmnt(10,12);
    Serial1.write(179);
    Serial1.write('+');
    Serial1.write(179);
    moniteur_dplcmnt(11,2);
    Serial1.write(192);
    Serial1.write(196);
    Serial1.write(217);
    moniteur_dplcmnt(11,12);
    Serial1.write(192);
    Serial1.write(196);
    Serial1.write(217);
 
 
    

    //bouton volets
    moniteur_dplcmnt(5,25); 
    Serial1.write(218);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191); 
    moniteur_dplcmnt(6,25); 
    Serial1.write(179);
    Serial1.print(F("  Volets  "));
    Serial1.write(179);
    moniteur_dplcmnt(7,25);
    Serial1.write(192);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);

    

    

    //bouton prog volet
    moniteur_dplcmnt(11,25); 
    Serial1.write(218);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(12,25);
    Serial1.write(179);
    Serial1.print(F("Prg Volets"));
    Serial1.write(179);
    moniteur_dplcmnt(13,25);   
    Serial1.write(192);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);
   
   
   // cadre infos temp et chauffage 
    moniteur_dplcmnt(13,2);
    Serial1.write(218);
    for ( i = 0; i < 11; i++) {
      Serial1.write(196);
    } 
    Serial1.write(191); 
    moniteur_dplcmnt(14,2);
    Serial1.write(179);
    Serial1.print(F("  Bas: "));
    Serial1.print(temperature1);
    moniteur_dplcmnt(14,14);
    Serial1.write(179);
    moniteur_dplcmnt(15,2);
    Serial1.write(179);
    Serial1.print(F(" Haut: "));
    Serial1.print(temperature2);
    moniteur_dplcmnt(15,14);
    Serial1.write(179);   
    moniteur_dplcmnt(16,2);
    Serial1.write(179);
    moniteur_dplcmnt(16,14);
    Serial1.write(179); 
    moniteur_dplcmnt(17,2);
    Serial1.write(179);
    Serial1.print(F("Chauf: "));
    if(relais_chauffage==0){
      Serial1.print(F("Non"));
    }
    if(relais_chauffage==1){
      Serial1.print(F("Oui"));
    } 
    if(relais_chauffage==2){
      Serial1.print(F(" HS"));
    } 
    moniteur_dplcmnt(17,14);
    Serial1.write(179);  
     moniteur_dplcmnt(18,2);
    Serial1.write(192);
    for ( i = 0; i < 11; i++) {
      Serial1.write(196);
    } 
    Serial1.write(217);



    //bouton prog chauffage
    moniteur_dplcmnt(17,25);
    Serial1.write(218);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191); 
    moniteur_dplcmnt(18,25);
    Serial1.write(179);
    Serial1.print(F("Prg Chauff"));
    Serial1.write(179);
    moniteur_dplcmnt(19,25);   
    Serial1.write(192);
    for ( i = 0; i < 10; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);

    
 } 


  void dessine_menu_volets(){ 
     menu_precedent=2;
     int i;     
    //entete et pied de page domotique   
    moniteur_dplcmnt(0,12);
    Serial1.write(201);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(187);
    moniteur_dplcmnt(1,12);
    Serial1.write(186);
    Serial1.print(F("   Volets    "));
    Serial1.write(186);
    moniteur_dplcmnt(2,12);
    Serial1.write(200);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(188);
    moniteur_dplcmnt(22,0);
    Serial1.write(218);
    for ( i = 0; i < 20; i++) {
      Serial1.write(196);
    }
    Serial1.write(191); 
    moniteur_dplcmnt(23,0); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,0); 
    Serial1.write(179);
    moniteur_dplcmnt(23,21); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,21); 
    Serial1.write(179); 
    //fin entete et pied
    
    //bouton retour
    moniteur_dplcmnt(3,0); 
    Serial1.write(218);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(4,0);
    Serial1.write(179);
    Serial1.print(F(" Retour "));
    Serial1.write(179);
    moniteur_dplcmnt(5,0);   
    Serial1.write(192);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);
    
    //ligne avec les denominatons de volets
    moniteur_dplcmnt(8,1); 
    Serial1.print(F("BuE Sa1 Sa2 Sa3 Sa4 Cu1 Cu2 Sdb BuA"));
    //dessin des bouton volets ->motif puis boucle neuf fois
        for ( i = 0; i < 9; i++) {
          moniteur_dplcmnt(9,1+4*i);Serial1.write(218);Serial1.write(196);Serial1.write(191);
          moniteur_dplcmnt(10,1+4*i);Serial1.write(179);Serial1.write('^');Serial1.write(179);
          moniteur_dplcmnt(11,1+4*i);Serial1.write(192);Serial1.write(196);Serial1.write(217);
 
          moniteur_dplcmnt(12,1+4*i);Serial1.write(218);Serial1.write(196);Serial1.write(191);
          moniteur_dplcmnt(13,1+4*i);Serial1.write(179);Serial1.write('=');Serial1.write(179);
          moniteur_dplcmnt(14,1+4*i);Serial1.write(192);Serial1.write(196);Serial1.write(217);
          
          moniteur_dplcmnt(15,1+4*i);Serial1.write(218);Serial1.write(196);Serial1.write(191);
          moniteur_dplcmnt(16,1+4*i);Serial1.write(179);Serial1.write('v');Serial1.write(179);
          moniteur_dplcmnt(17,1+4*i);Serial1.write(192);Serial1.write(196);Serial1.write(217);
    }
    

 
 } 


  void dessine_menu_prog_volets(){ 
    if(menu_precedent!=3) {
      volet_courant_prog=0;
    }
    menu_precedent=3;
      int i;     
    //entete et pied de page domotique   
    moniteur_dplcmnt(0,12);
    Serial1.write(201);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(187);
    moniteur_dplcmnt(1,12);
    Serial1.write(186);
    Serial1.print(F("Prog. Volets "));
    Serial1.write(186);
    moniteur_dplcmnt(2,12);
    Serial1.write(200);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(188);
    moniteur_dplcmnt(22,0);
    Serial1.write(218);
    for ( i = 0; i < 20; i++) {
      Serial1.write(196);
    }
    Serial1.write(191); 
    moniteur_dplcmnt(23,0); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,0); 
    Serial1.write(179);
    moniteur_dplcmnt(23,21); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,21); 
    Serial1.write(179); 
    //fin entete et pied
    
    //bouton retour
    moniteur_dplcmnt(3,0); 
    Serial1.write(218);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(4,0);
    Serial1.write(179);
    Serial1.print(F(" Retour "));
    Serial1.write(179);
    moniteur_dplcmnt(5,0);   
    Serial1.write(192);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);

    //bouton prog volet
    moniteur_dplcmnt(3,24); 
    Serial1.write(218);
    for ( i = 0; i < 12; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(4,24);
    Serial1.write(179);
    Serial1.print(F("Regl Horloge"));
    Serial1.write(179);
    moniteur_dplcmnt(5,24);   
    Serial1.write(192);
    for ( i = 0; i < 12; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);

    
    //ligne avec les denominatons de volets
    moniteur_dplcmnt(8,1); 
    Serial1.print(F("BuE Sa1 Sa2 Sa3 Sa4 Cu1 Cu2 Sdb BuA"));
    //dessin des bouton volets ->motif puis boucle neuf fois
        for ( i = 0; i < 9; i++) {
          moniteur_dplcmnt(9,1+4*i);Serial1.write(218);Serial1.write(196);Serial1.write(191);
          moniteur_dplcmnt(10,1+4*i);Serial1.write(179);
          if(volet_courant_prog==i+1) {
            Serial1.print(F("="));
          } else {
            Serial1.print(F("v"));
          } 
          Serial1.write(179);
          moniteur_dplcmnt(11,1+4*i);Serial1.write(192);Serial1.write(196);Serial1.write(217);
 

    }
     if(volet_courant_prog!=0) {
      
       //motif des 3 boutons de prog
       
       //manu
       moniteur_dplcmnt(14,1);
       Serial1.write(218);
       for ( i = 0; i < 6; i++) {
          Serial1.write(196);
       }   
       Serial1.write(191);
       moniteur_dplcmnt(15,1);
       Serial1.write(179);
       Serial1.print(F("Manu  "));
       Serial1.write(179);
       moniteur_dplcmnt(16,1);   
       Serial1.write(192);
       for ( i = 0; i < 6; i++) {
        Serial1.write(196);
       }    
       Serial1.write(217); 
       
       //auto
       moniteur_dplcmnt(14,9);
       Serial1.write(218);
       for ( i = 0; i < 6; i++) {
          Serial1.write(196);
       }   
       Serial1.write(191);
       moniteur_dplcmnt(15,9);
       Serial1.write(179);
       Serial1.print(F("Auto  "));
       Serial1.write(179);
       moniteur_dplcmnt(16,9);   
       Serial1.write(192);
       for ( i = 0; i < 6; i++) {
        Serial1.write(196);
       }    
       Serial1.write(217); 
  
        //perso
       moniteur_dplcmnt(14,17);
       Serial1.write(218);
       for ( i = 0; i < 7; i++) {
          Serial1.write(196);
       }   
       Serial1.write(191);
       moniteur_dplcmnt(15,17);
       Serial1.write(179);
       Serial1.print(F("Perso  "));
       Serial1.write(179);
       moniteur_dplcmnt(16,17);   
       Serial1.write(192);
       for ( i = 0; i < 7; i++) {
        Serial1.write(196);
       }    
       Serial1.write(217); 
       
       
       //requeteee prom pour placer le caractere * sur la prog correspondante
       if (volet_courant_prog!=0){
         int adresse_debut_volet=100+(volet_courant_prog-1)*6;
         int prog_manu=EEPROM.read(adresse_debut_volet);
         if (prog_manu==1) {
            int mode_auto=EEPROM.read(adresse_debut_volet+1);
            if (mode_auto==1) {
               //prog auto 
               moniteur_dplcmnt(15,6);
               Serial1.write(' ');              
               moniteur_dplcmnt(15,14);
               Serial1.write('*');
               moniteur_dplcmnt(15,24);
               Serial1.write(' ');
               moniteur_dplcmnt(14,26); 
               Serial1.print(F("           "));
               moniteur_dplcmnt(15,26); 
               Serial1.print(F("           "));  
               moniteur_dplcmnt(16,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(17,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(18,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(19,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(20,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(21,26); 
               Serial1.print(F("           ")); 
               moniteur_dplcmnt(22,26); 
               Serial1.print(F("           "));         
            } else {
               //prog manuelle
               moniteur_dplcmnt(15,6);
               Serial1.write(' ');
               moniteur_dplcmnt(15,14);
               Serial1.write(' ');
               moniteur_dplcmnt(15,24);
               Serial1.write('*');  
               //si prog sur prog manuelle aff de la zone avec les heures de montée et de descente + btn ok pour valider la prog 
                int heure_monte=EEPROM.read(adresse_debut_volet+2);
                int minute_monte=EEPROM.read(adresse_debut_volet+3);
                int heure_descente=EEPROM.read(adresse_debut_volet+4);
                int minute_descente=EEPROM.read(adresse_debut_volet+5);
                moniteur_dplcmnt(15,29); 
                if(heure_monte<10){ Serial1.print("0");}
                Serial1.print(heure_monte);
                Serial1.print(":");
                if(minute_monte<10){ Serial1.print("0");}
                Serial1.print(minute_monte);
                moniteur_dplcmnt(18,29); 
                if(heure_descente<10){ Serial1.print("0");}
                Serial1.print(heure_descente);
                Serial1.print(":");
                if(minute_descente<10){ Serial1.print("0");}
                Serial1.print(minute_descente);
                //- debut
                moniteur_dplcmnt(14,26); 
                Serial1.write(218); 
                Serial1.write(196);
                Serial1.write(191);
                moniteur_dplcmnt(15,26); 
                Serial1.write(179);
                Serial1.write('-');
                Serial1.write(179);
                moniteur_dplcmnt(16,26);
                Serial1.write(192);
                Serial1.write(196);
                Serial1.write(217);
                
                //+ debut
                moniteur_dplcmnt(14,34); 
                Serial1.write(218); 
                Serial1.write(196);
                Serial1.write(191);
                moniteur_dplcmnt(15,34); 
                Serial1.write(179);
                Serial1.write('+');
                Serial1.write(179);
                moniteur_dplcmnt(16,34);
                Serial1.write(192);
                Serial1.write(196);
                Serial1.write(217);
  
                //- fin
                moniteur_dplcmnt(17,26); 
                Serial1.write(218); 
                Serial1.write(196);
                Serial1.write(191);
                moniteur_dplcmnt(18,26); 
                Serial1.write(179);
                Serial1.write('-');
                Serial1.write(179);
                moniteur_dplcmnt(19,26);
                Serial1.write(192);
                Serial1.write(196);
                Serial1.write(217);
                
                //+ fin
                moniteur_dplcmnt(17,34); 
                Serial1.write(218); 
                Serial1.write(196);
                Serial1.write(191);
                moniteur_dplcmnt(18,34); 
                Serial1.write(179);
                Serial1.write('+');
                Serial1.write(179);
                moniteur_dplcmnt(19,34);
                Serial1.write(192);
                Serial1.write(196);
                Serial1.write(217);
                
               
            }
         } else {
           //pas de prog
           moniteur_dplcmnt(15,6);
           Serial1.write('*');
           moniteur_dplcmnt(15,14);
           Serial1.write(' ');
           moniteur_dplcmnt(15,24);
           Serial1.write(' '); 
           moniteur_dplcmnt(14,26); 
           Serial1.print(F("           "));
           moniteur_dplcmnt(15,26); 
           Serial1.print(F("           "));  
           moniteur_dplcmnt(16,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(17,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(18,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(19,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(20,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(21,26); 
           Serial1.print(F("           ")); 
           moniteur_dplcmnt(22,26); 
           Serial1.print(F("           ")); 
         }
         
         
       } 
       
    }
    
 
 } 


  
  void dessine_menu_horloge(){

      int i;     
    //entete et pied de page domotique   
    moniteur_dplcmnt(0,12);
    Serial1.write(201);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(187);
    moniteur_dplcmnt(1,12);
    Serial1.write(186);
    Serial1.print(F("   Horloge   "));
    Serial1.write(186);
    moniteur_dplcmnt(2,12);
    Serial1.write(200);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(188);
    moniteur_dplcmnt(22,0);
    Serial1.write(218);
    for ( i = 0; i < 20; i++) {
      Serial1.write(196);
    }
    Serial1.write(191); 
    moniteur_dplcmnt(23,0); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,0); 
    Serial1.write(179);
    moniteur_dplcmnt(23,21); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,21); 
    Serial1.write(179); 
    //fin entete et pied
    
    //bouton retour
    moniteur_dplcmnt(3,0); 
    Serial1.write(218);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(4,0);
    Serial1.write(179);
    Serial1.print(F(" Retour "));
    Serial1.write(179);
    moniteur_dplcmnt(5,0);   
    Serial1.write(192);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);
 
    //labels entete heures  minutes et secondes
    moniteur_dplcmnt(5,12);
    Serial1.print(F("Mise a jour Horloge"));
    moniteur_dplcmnt(7,15);
    Serial1.print(day());
    Serial1.print("-");
    Serial1.print(month());
    Serial1.print("-");
    Serial1.print(year());
    moniteur_dplcmnt(9,6);
    Serial1.print(F("Heures   Minutes  Secondes")); 
    
    //boutons + et -
    //boutons heures
    moniteur_dplcmnt(10,6); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,6); Serial1.write(179); Serial1.write('-'); Serial1.write(179);
    moniteur_dplcmnt(12,6); Serial1.write(192); Serial1.write(196); Serial1.write(217);
    moniteur_dplcmnt(10,11); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,11); Serial1.write(179); Serial1.write('+'); Serial1.write(179);
    moniteur_dplcmnt(12,11); Serial1.write(192); Serial1.write(196); Serial1.write(217);    

    //boutons minutes
    moniteur_dplcmnt(10,15); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,15); Serial1.write(179); Serial1.write('-'); Serial1.write(179);
    moniteur_dplcmnt(12,15); Serial1.write(192); Serial1.write(196); Serial1.write(217);
    moniteur_dplcmnt(10,20); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,20); Serial1.write(179); Serial1.write('+'); Serial1.write(179);
    moniteur_dplcmnt(12,20); Serial1.write(192); Serial1.write(196); Serial1.write(217); 

    //boutons secondes
    moniteur_dplcmnt(10,24); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,24); Serial1.write(179); Serial1.write('-'); Serial1.write(179);
    moniteur_dplcmnt(12,24); Serial1.write(192); Serial1.write(196); Serial1.write(217);
    moniteur_dplcmnt(10,29); Serial1.write(218); Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(11,29); Serial1.write(179); Serial1.write('+'); Serial1.write(179);
    moniteur_dplcmnt(12,29); Serial1.write(192); Serial1.write(196); Serial1.write(217);   
  
  
    //bouton valider 

    moniteur_dplcmnt(15,14); 
    Serial1.write(218);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(16,14);
    Serial1.write(179);
    Serial1.print(F("Valider "));
    Serial1.write(179);
    moniteur_dplcmnt(17,14);   
    Serial1.write(192);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217); 
    
    //verif si on vient d'un autre menu et definition des variables courantes heures minutes et secondes
    if(menu_precedent!=4) { 
      heure_en_cours_prog=hour(); 
      minute_en_cours_prog=minute(); 
      seconde_en_cours_prog=second(); 
    }
   
   //affichage heures minutes et secondes
   moniteur_dplcmnt(11,9);  
   if (heure_en_cours_prog<10) {Serial1.write('0');}
   Serial1.print(heure_en_cours_prog);
   moniteur_dplcmnt(11,18); 
  if (minute_en_cours_prog<10) {Serial1.write('0');} 
   Serial1.print(minute_en_cours_prog);
   moniteur_dplcmnt(11,27);
   if (seconde_en_cours_prog<10) {Serial1.write('0');}  
   Serial1.print(seconde_en_cours_prog);
       
   menu_precedent=4;
   
   
 } 


  void dessine_menu_prog_chauff(){
    menu_precedent=5;
    int i; 
    int debut_plage_memoire;
    int heure_debut;
    int minute_debut;
    int heure_fin;
    int minute_fin;
    
    //entete et pied de page domotique   
    moniteur_dplcmnt(0,12);
    Serial1.write(201);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(187);
    moniteur_dplcmnt(1,12);
    Serial1.write(186);
    Serial1.print(F("Prog. Chauff."));
    Serial1.write(186);
    moniteur_dplcmnt(2,12);
    Serial1.write(200);
    for ( i = 0; i < 13; i++) {
      Serial1.write(205);
    }
    Serial1.write(188);
    moniteur_dplcmnt(22,0);
    Serial1.write(218);
    for ( i = 0; i < 20; i++) {
      Serial1.write(196);
    }
    Serial1.write(191); 
    moniteur_dplcmnt(23,0); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,0); 
    Serial1.write(179);
    moniteur_dplcmnt(23,21); 
    Serial1.write(179); 
    moniteur_dplcmnt(24,21); 
    Serial1.write(179); 
    //fin entete et pied
    
    //bouton retour
    moniteur_dplcmnt(3,0); 
    Serial1.write(218);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(191);
    moniteur_dplcmnt(4,0);
    Serial1.write(179);
    Serial1.print(F(" Retour "));
    Serial1.write(179);
    moniteur_dplcmnt(5,0);   
    Serial1.write(192);
    for ( i = 0; i < 8; i++) {
      Serial1.write(196);
    }    
    Serial1.write(217);
    
    //bouton prog globale on/off
    moniteur_dplcmnt(4,19);
    Serial1.print(F("Globale"));
    //btn on
    moniteur_dplcmnt(3,26);Serial1.write(218);  Serial1.write(196);Serial1.write(196);Serial1.write(196); Serial1.write(191);
    moniteur_dplcmnt(4,26); Serial1.write(179);Serial1.print(F("On "));Serial1.write(179);
    moniteur_dplcmnt(5,26); Serial1.write(192);Serial1.write(196);Serial1.write(196);Serial1.write(196);Serial1.write(217);
    //btn off
    moniteur_dplcmnt(3,32);Serial1.write(218);  Serial1.write(196);Serial1.write(196);Serial1.write(196); Serial1.write(196);Serial1.write(191);
    moniteur_dplcmnt(4,32); Serial1.write(179);Serial1.print(F("Off "));Serial1.write(179);
    moniteur_dplcmnt(5,32); Serial1.write(192);Serial1.write(196);Serial1.write(196);Serial1.write(196);Serial1.write(196);Serial1.write(217);    
    
    //aff de l'etoile en face de la bonne prog (on ou off) apres lecture de l'eeprom
    int prog_chauffage=EEPROM.read(5);
    if(prog_chauffage==1) {
      moniteur_dplcmnt(4,29);
      Serial1.print("*");
      moniteur_dplcmnt(4,36);
      Serial1.print(" ");
    } else {
      moniteur_dplcmnt(4,29);
      Serial1.print(" ");
      moniteur_dplcmnt(4,36);
      Serial1.print("*");
    }
    
    //ligne avec les denominatons de jour de la semaine
    moniteur_dplcmnt(7,1); 
    Serial1.print(F("Lu. Ma. Me. Je. Ve. Sa. Di."));
    //dessin des bouton volets ->motif puis boucle neuf fois
        for ( i = 0; i < 7; i++) {
          moniteur_dplcmnt(8,1+4*i);Serial1.write(218);Serial1.write(196);Serial1.write(191);
          moniteur_dplcmnt(9,1+4*i);Serial1.write(179);
          if(jour_courant_prog==i+1) {
            Serial1.print("=");
          } else {
            Serial1.print("v");
          } 
          Serial1.write(179);
          moniteur_dplcmnt(10,1+4*i);Serial1.write(192);Serial1.write(196);Serial1.write(217);
    }
    
    
    
    if(jour_courant_prog!=0) {
          debut_plage_memoire=10+(jour_courant_prog-1)*12;
          for ( i = 0; i < 3; i++) {
              moniteur_dplcmnt(12+3*i+1,5);Serial1.print(F("Plage"));Serial1.print(i+1);
              //- debut
              moniteur_dplcmnt(12+3*i,13);  Serial1.write(218);  Serial1.write(196); Serial1.write(191);
              moniteur_dplcmnt(12+3*i+1,13);  Serial1.write(179);Serial1.write('-');Serial1.write(179);
              moniteur_dplcmnt(12+3*i+2,13);Serial1.write(192);Serial1.write(196);Serial1.write(217); 
              //heures minutes debut
              heure_debut=EEPROM.read(debut_plage_memoire+i*4);
              minute_debut=EEPROM.read(debut_plage_memoire+i*4+1);
              moniteur_dplcmnt(12+3*i+1,16);
              if (heure_debut<10) {Serial1.write('0');}
              Serial1.print(heure_debut);
              Serial1.write(':');
              if (minute_debut<10) {Serial1.write('0');}
              Serial1.print(minute_debut);
              //+ debut
              moniteur_dplcmnt(12+3*i,21);  Serial1.write(218);  Serial1.write(196); Serial1.write(191);
              moniteur_dplcmnt(12+3*i+1,21);  Serial1.write(179);Serial1.write('+');Serial1.write(179);
              moniteur_dplcmnt(12+3*i+2,21);Serial1.write(192);Serial1.write(196);Serial1.write(217); 
              
              //- fin
              moniteur_dplcmnt(12+3*i,25);  Serial1.write(218);  Serial1.write(196); Serial1.write(191);
              moniteur_dplcmnt(12+3*i+1,25);  Serial1.write(179);Serial1.write('-');Serial1.write(179);
              moniteur_dplcmnt(12+3*i+2,25);Serial1.write(192);Serial1.write(196);Serial1.write(217);  
              //heures minutes fin
              heure_fin=EEPROM.read(debut_plage_memoire+i*4+2);
              minute_fin=EEPROM.read(debut_plage_memoire+i*4+3);
              moniteur_dplcmnt(12+3*i+1,28);
              if (heure_fin<10) {Serial1.write('0');}
              Serial1.print(heure_fin);
              Serial1.write(':');
              if (minute_fin<10) {Serial1.write('0');}
              Serial1.print(minute_fin);
              //+ fin
              moniteur_dplcmnt(12+3*i,33);  Serial1.write(218);  Serial1.write(196); Serial1.write(191);
              moniteur_dplcmnt(12+3*i+1,33);  Serial1.write(179);Serial1.write('+');Serial1.write(179);
              moniteur_dplcmnt(12+3*i+2,33);Serial1.write(192);Serial1.write(196);Serial1.write(217);             
            
          }  
    }
 
 } 
