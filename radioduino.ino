
///////////////////////////////
////  Alexandre OGER 2013  ////
///////////////////////////////

  
  //radio arduino alex oger été 2013
  

//un afficheur LCD I2c 4 lignes de 20 caracteres
//un capteur bmp-085 (pression atmo) sur bus I2c
//un tuner FM sparkfun à base de chip Si4703 sur bus I2c
//une horloge RTC sur bus I2c (DS3231 plus precis qu'un DS1307)
// un joystick analogique sparkfun avec "click" intégré


/*|
                              
                                            ________| |__
                              MOSI          | |()|  | | | SCK
                              RX_LED/SS     |       | | | MISO
                              1/TX          |      ICSP | VIN
                              0/RX          |           | GND
                              RESET         |           | RESET
                              GND           |           | +5V
I2C-SDA (BARO-LCD-TEMPS-TUNER) 2/SDA        | ARDUINO   | NC
I2C-SCL (BARO-LCD-TEMPS-TUNER) 3(PWM)/SCL   |           | NC
                              4/A6          |           | A5  
                              5 (PWM)       |   MICRO   | A4   
                              6 (PWM)/A7    |           | A3    ---------- buzzer
                              7             |           | A2    ---------- joystick (click)
                              8/A9          |           | A1    ---------- joystick (horizontal)
                              9 (PWM)/A10   |           | A0    ---------- joystick (vertical)
                              10 (PWM) /A11 |           | AREF
                              11(PWM)       |   _____   | 3.3V
                              12 (PWM)/A12  |  [ USB ]  | 13 (PWM)
                                            ----|___|---

*/

  //appel librairie wire pour la communication I2C avec l'horloge RTC
  #include <Wire.h>
  
  //module BMP085 (capteur pression atmo et temperature)
  #include <Adafruit_BMP085.h>
  
  //appel des librairies de gestion du temps (horloge RTC, temps et alarmes/timers)
  #include <DS1307RTC.h>
  #include <Time.h>
  #include <TimeAlarms.h>
  
  
  //appel librairie module radio sur bus I2C
  #include <Si4703_Breakout.h>
  
  //appel librairie EEPROM pour le stockage/lecture des stations memorisées
  //les adresses de 1 à 5 sont utilisees pour stocker 5 stations FM en memoire
  #include <EEPROM.h>
  
  //appel librairie permettant de connaitre la quantitié de RAM dispo
  //utilisé uniquemeent dans loop() pour le debug
  #include <MemoryFree.h>
  
  //ecran lcd
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27,20,4);
    
  //creation d'un objet BMP085 pour le capteur de pression
  Adafruit_BMP085 baro;
  
   
  byte addr1[8]; //tbl adresse capteur temp onewire
  byte donnees[12]; //buffer de lecture onewire
  
  //joystick analogiue cf setup()
  const int VERT = A0; // analogigue
  const int HORIZ = A1; // analogique
  const int SEL = A2; // digital
  int vertical=0;
  int horizontal=0;
  int select=0;
  
  //buzzer
  const int BUZZER = A3;
  
  //init des variable pression atmo 
  int pression=0; 
  char tendance[9];
  int tend=0;

  
  //init des variables de gestion l'horloge
  int heure_courante;
  int minute_courante;
  int seconde_courante;
  int mois_courant; 
  int jour_courant;
  int annee_courante;
  int heure_ete;

  //init des variables de positionnement du pseudo curseur
  int position_courante_curseur=5;
  int ligne_position_courante_curseur=3;
  int col_position_courante_curseur=4;
  int caractere_position_courante_curseur=43;
  int blink_cur=0;
  
  //init des variables de gestion de la minuteurie
  int minuterie=0;
  int minuterie_en_cours=0;
  long minuterie_temp_fin=0;
  int minuterie_temps_restant=0;
  int minuterie_id_timer=0;
  
  
  //tableau des données pour le pseudo curseur et la matrice de deplacement du joystick
  int pseudo_blink[13][7] = { 
    //{colonne,ligne,caractere en dec,nvl_index_deplcmt_vert_haut,nvl_index_deplcmt_vert_bas,nvl_index_deplcmt_horiz_gauche,nvl_index_deplcmt_horiz_droite}
    // tellymate 38x25 carectere à l'écran
    //      cf sortie vidéo:
    //          1<rechercher>2  <- 1 et 2 pour seek-up et seek-down
    //            2 3 4 5 6 7   <- 234567  les 5 memoires FM affichees
    //           <8emoriser>    <- 8 mise memoire d'un station FM
    //       minuterie 9 00  10   11epart  12rret <- 9 10 11 12 gestion de la minuterie - + depart arret
    {0,0,0,0,0,0,0}, //0-index null on n'y passe jamais.. 
    {9,1,42,1,4,1,2}, //1-mise en memoire station Fm courante
    {10,1,45,2,4,1,3}, //2-recherche fm -> 88mhz
    {19,1,43,3,4,2,3}, //3-recherche fm -> 108mhz
    {1,3,45,1,4,4,5}, //4- minuterie -
    {4,3,43,1,5,4,6}, //5- minuterie +
    {8,3,68,6,6,5,7}, //6- minuterie depart
    {15,3,65,7,7,6,7}, //7- minuterie arret

    
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

  //init des variables de gestion de la FM et du RDS
  float  frequence;
  char ps[8]; //stockage des donnees RDS-PS (Program String -> nom de la station)
  char rt[64];//stockage des donnees RDS-RT (Radio Text -> infos sur le programen en ecoute)
  int memoire_FM_OK=0;
  int id_alarm_rt; //id du timer de recherche du radio text (en cas de scans de rapproche, permet d'annuler la recherhce
  int id_alarm_ps; //id du timer de recherche du program station (en cas de scans de rapproche, permet d'annuler la recherhce
  
  int index_aff_rt=28;
  
    
  //init du module radio sur bus I2c
  int resetPin = 4;
  int SDIO = 2; //alias SDA
  int SCLK = 3; //alias SCL
  Si4703_Breakout radio(resetPin, SDIO, SCLK);
  
  //definition des caracteres spéciaux pour l'appel de commandes sur la sortie vidéo
  #define CHAR_ESC 0x1B
  #define CHAR_DLE 0x10 
  
  
  void setup()   {
    
        //init des memoires a 88MHz lors de la premiere utlisation, a commenter apres usage
        // exemple: 88Mhz X 10 - 875 = ( <- valeur a stocker
        //EEPROM.write(1,5);EEPROM.write(2,20);EEPROM.write(3,55);EEPROM.write(4,70);EEPROM.write(5,95);
    
        //conf du click du joystick
        pinMode(SEL,INPUT);
        //activation de la resistence pullup du click (evite les court-jus)
        digitalWrite(SEL,HIGH);
        
        //conf du buzzer de minuterie
        pinMode(BUZZER,OUTPUT);
        digitalWrite(BUZZER,LOW);
        
        //allumage  de l'écran LCD (relais a optocoupleur)
        pinMode(A4,OUTPUT);
        digitalWrite(A4,HIGH);
        delay(400); //400ms suffisent
        digitalWrite(A4,LOW);
        
        //extinction ampli audio
        pinMode(A5,OUTPUT);
        digitalWrite(A5,LOW);
  
        //init port serie sur usb
        //pas necessaire en prod
        Serial.begin(9600);
                
        //attente de la fin de l'init du port serie usb
        delay(2500);

        
        //demarrage bus I2C
        delay(200);
        Wire.begin();
              
   
        lcd.init();
        lcd.init();
        delay(200);
        lcd.backlight();
        lcd.clear();
        
        //demarrage de la synchro de l'horloge temps réel sur bus I2C
        setSyncProvider(RTC.get);
        //synchro toute les heures
        setSyncInterval(3600);
        if(timeStatus()== timeSet) {       
          lcd.setCursor(0, 0);
          lcd.print(F("Sync Horloge RTC OK")); 
          Serial.println(F("Sync Horloge RTC OK"));
          Serial.println(String(day())+"/"+String(month())+"/"+String(year())+" Heure:"+String(hour())+":"+String(minute())+":"+String(second()) );
        } else {
          lcd.setCursor(0, 0);
          lcd.print(F("Sync Horloge RTC NOK"));
          Serial.println(F("Sync Horloge RTC NOK"));
          delay(10000);     
        }
        
        //demarrage capteur BMP085 sur bus I2C
        if (baro.begin()) {
          lcd.setCursor(0, 1);
          lcd.print(F("BMP085 press I2C OK"));
         } else {
          lcd.setCursor(0, 1);
          lcd.print(F("BMP085 press I2C NOK"));
          delay(10000); 
        }
        
        //demarrge du module FM
        if (radio.powerOn()) {
          lcd.setCursor(0, 2);
          lcd.print(F("Module FM - OK"));
        } else {
          lcd.setCursor(0, 2);
          lcd.print(F("Module FM - POK"));
          delay(10000);  
        }
        
       
        heure_courante=hour();
        minute_courante=minute();
        seconde_courante=second();
        mois_courant=month(); 
        jour_courant=day();
        annee_courante=year();
        heure_ete=EEPROM.read(200);
        
        Serial.print("ete: ");
        Serial.println(heure_ete);
        
       /*
       {2014,30,3,26,10},
       */
        
        for(int i= 0; i<26; i++){
          if(heures_ete[i][0]==annee_courante){
             //test debut heure ete
             if( ( (heures_ete[i][2]==mois_courant && jour_courant>=heures_ete[i][1]) || (mois_courant>heures_ete[i][2] )) && heure_ete==0){
               if(heure_courante!=23){
                 
                 setTime(heure_courante+1,minute_courante,seconde_courante,jour_courant,mois_courant,annee_courante);
                 RTC.set(now());
                 // ecriture eeprom pour stocker un booleen heure d'été à 1
                 EEPROM.write(200,1);
                 heure_ete=1;
                 lcd.setCursor(0, 3);
                 lcd.print(F("Heure -> ete  OK"));
                 delay(5000); 
                 break; 
               }
             } 
             //test fin heure ete
             if( ( (heures_ete[i][4]==mois_courant && jour_courant>=heures_ete[i][3]) || (mois_courant>heures_ete[i][4] )) && heure_ete==1){
               if(heure_courante!=0){
                 setTime(heure_courante-1,minute_courante,seconde_courante,jour_courant,mois_courant,annee_courante);
                 RTC.set(now()); 
                 // ecriture eeprom pour stocker un booleen heure d'été à 0 
                 EEPROM.write(200,0);  
                 heure_ete=0;       
                 lcd.setCursor(0, 3);
                 lcd.print(F("Heure -> hiver  OK"));
                 delay(5000); 
                  break;                
               }
             }
             break;  
          }        
        }
        
        
        heure_courante=100;
        minute_courante=100;
        seconde_courante=100;
        mois_courant=100; 
        jour_courant=100;
        annee_courante=1900;
        
        delay(800);
        lcd.clear();
        
        //petite anime de demarrage de la sortie vidéo
        splash_screen_alex();
        
        
        //maj des info d'horloge, de temperature et de pression atmo sur la sortie video 
        //permet d'eviter d'attendre le declenchement des timers ci-apres        
        maj_pression_temp();
        horloge_radiotext_minuterie();
        
        //timer toutes les 5sec pour l'affichage de la tendance et de la pression atmo
        Alarm.timerRepeat(5,temp_press);
        
        //timer toutes les minutes pour la maj de la pression atmo
        Alarm.timerRepeat(60,maj_pression_temp);
        
        //timer toutes les sec pour l'horloge et le scroll du radio text (RDS)
        Alarm.timerRepeat(1,horloge_radiotext_minuterie);
        
        //timer de recherche des infos radiotext sur le RDS
        Alarm.timerRepeat(300,chercher_RT);
 
        //allumage ampli audio
        digitalWrite(A5,HIGH);
        
        radio.setVolume(15);
        
        
        //rappel de la memoire FM n°1 au demarrage
        ps[0]='\0';
        rt[0]='\0';
        memoire_FM(1);
        
        //aff des choses invariantes sur le LCD
        lcd.setCursor(9, 1);
        lcd.print("*-");
        lcd.setCursor(19, 1);
        lcd.print("+");
        lcd.setCursor(0, 3);
        lcd.print(F(" -00+   Depart Arret"));
        

        
  } 
  
  
  void loop(){
      
     //trigger de declenchement de verif des timers d'alarme
     //permet de "debouncer" le joystick (200ms)
      Alarm.delay(200);

      //verif du joystick et deplacement ou execution en conséquence (click)
      check_joystick();
           

  }
  

  void check_joystick(){  
  
      //gestion du joystick en fonction du tableau ou est stockee la matrice des deplacements
      
      vertical = analogRead(VERT); // entre 0 et 1023 - joystick au repos a environ 512
      horizontal = analogRead(HORIZ); // entre 0 et 1023 - joystick au repos a environ 512
      /*
      Serial.print("hor");
      Serial.println(vertical);
      Serial.print("ver");
      Serial.println(horizontal);
      */
      select = digitalRead(SEL); //LOW-0 si cliqué
      int i;
      

     
      //verif manip du joystick sur les 4 directions
      if (vertical>925) { 
          for ( i = 0; i < 8; i++) {
                  if (position_courante_curseur==i) {
                    lcd.setCursor(col_position_courante_curseur,ligne_position_courante_curseur);
                    lcd.write(caractere_position_courante_curseur) ;
                    position_courante_curseur=pseudo_blink[i][3];
                    ligne_position_courante_curseur=pseudo_blink[position_courante_curseur][1];
                    col_position_courante_curseur=pseudo_blink[position_courante_curseur][0];
                    caractere_position_courante_curseur=pseudo_blink[position_courante_curseur][2];
                    blink_pseudo_curseur_et_minuterie();
                    
                    Alarm.delay(500);
                    break;
                  }
          }  
      }
      if (vertical<125) {
          for ( i = 0; i < 8; i++) {
                  if (position_courante_curseur==i) {
                    lcd.setCursor(col_position_courante_curseur,ligne_position_courante_curseur);
                    lcd.write(caractere_position_courante_curseur) ;                   
                    position_courante_curseur=pseudo_blink[i][4];
                    ligne_position_courante_curseur=pseudo_blink[position_courante_curseur][1];
                    col_position_courante_curseur=pseudo_blink[position_courante_curseur][0];
                    caractere_position_courante_curseur=pseudo_blink[position_courante_curseur][2];
                    blink_pseudo_curseur_et_minuterie();
                    Alarm.delay(500);
                    break;
                  }
          } 
       }       
      if (horizontal>925) {
          for ( i = 0; i < 8; i++) {
                  if (position_courante_curseur==i) {
                    lcd.setCursor(col_position_courante_curseur,ligne_position_courante_curseur);
                    lcd.write(caractere_position_courante_curseur) ;                  
                    position_courante_curseur=pseudo_blink[i][5];
                    ligne_position_courante_curseur=pseudo_blink[position_courante_curseur][1];
                    col_position_courante_curseur=pseudo_blink[position_courante_curseur][0];
                    caractere_position_courante_curseur=pseudo_blink[position_courante_curseur][2];
                    blink_pseudo_curseur_et_minuterie();
                    Alarm.delay(500);
                    break;
                  }
          } 
      }
      if (horizontal<125) {
          for ( i = 0; i < 8; i++) {
                  if (position_courante_curseur==i) {
                    lcd.setCursor(col_position_courante_curseur,ligne_position_courante_curseur);
                    lcd.write(caractere_position_courante_curseur) ;                   
                    position_courante_curseur=pseudo_blink[i][6];
                    ligne_position_courante_curseur=pseudo_blink[position_courante_curseur][1];
                    col_position_courante_curseur=pseudo_blink[position_courante_curseur][0];
                    caractere_position_courante_curseur=pseudo_blink[position_courante_curseur][2];
                    blink_pseudo_curseur_et_minuterie();
                    Alarm.delay(500);
                    break;
                  }
          } 
      }
      
      if(select == LOW) { //verif du click sur le joystick
         switch (position_courante_curseur) {
            case 1:
               memoire_FM_memoriser(1); 
             break;  
            case 2:
             recherche_FM('b'); // <- 'b' pour bas
             break; 
            case 3:
             recherche_FM('h');// <- 'h' pour haur
             break; 

            case 4:
             //decremente minuterie
             minuterie_dec();
             break;
            case 5:
             //incremente minuterie
             minuterie_inc();
             break;
            case 6:
             minuterie_depart();
             break;
            case 7:
             minuterie_arret();
             break;
          }              
        
      }
      
  
        blink_pseudo_curseur_et_minuterie();  //affichage du pseudo curseur et rafraichissement minuterie
      
  }
  
  
  void recherche_FM(char sens){  
    if (sens=='h')  {
      if(id_alarm_rt!=0){ 
        Alarm.free(id_alarm_rt);
        id_alarm_rt=0;
      }
      if(id_alarm_ps!=0){ 
        Alarm.free(id_alarm_ps);
        id_alarm_ps=0;
      }
      frequence=(float) radio.seekUp()/10;
      affiche_infos_radios();
    }
    if (sens=='b')  { 
      if(id_alarm_rt!=0){ 
        Alarm.free(id_alarm_rt);
        id_alarm_rt=0;
      }
      if(id_alarm_ps!=0){ 
        Alarm.free(id_alarm_ps);
        id_alarm_ps=0;
      }
      frequence=(float) radio.seekDown()/10;
      affiche_infos_radios();
    } 
  }
  
  
  void memoire_FM(int mem_FM){   
    //l'eeprom stock par adresse 1 octet -> de 0 à 255
    //le module FM travail en 1/10eme de Mhz (de 875 à 1080)
    // comme 875+255=1130 > 1080 , un seul octet est necessaire pour le stockage de chaque memoire 
    int canal=EEPROM.read(mem_FM);
    canal=canal+875;
    radio.setChannel(canal);
    frequence=(float) canal/10;
    affiche_infos_radios();
  }
  
  void memoire_FM_memoriser(int mem_FM){ 
    //deduction de l'octet de stockage en focntion de la frequence
    //passage en 1/10 de Mhz puis soustraction de 875
    int canal=frequence*10-875;
    EEPROM.write(mem_FM,canal);
    memoire_FM_OK=0;
  }  
  
  void affiche_infos_radios(){ 
      int i; 
      lcd.setCursor(11, 1);
      lcd.print(F("     "));      
      lcd.setCursor(11, 1);
      lcd.print(frequence,1); 
      lcd.setCursor(16, 1);
      lcd.print(F("Mhz"));
      id_alarm_ps=Alarm.timerOnce(5, chercher_PS);
      id_alarm_rt=Alarm.timerOnce(10, chercher_RT);
  }
 
void chercher_PS(){ 
    lcd.setCursor(0, 1);
    lcd.print(F("         "));
    ps[0]='\0';
    radio.readPS(ps);
    lcd.setCursor(0, 1);
    lcd.print(ps);

  }
  
  
  void chercher_RT(){ 
    int i;
    chercher_PS();//on en profite pour mettre à jour les infos PS au passage
    index_aff_rt=28;
    lcd.setCursor(0,2);
    lcd.print(F("Rech RDS Click=>Stop"));
    radio.readRT(rt);

    lcd.setCursor(0,2);
    lcd.print(F("                    "));
    for ( i = 63 ; i >0 ; i--) {
        if (rt[i]==' ') {
          rt[i]='\0';
        }else{
          break;
        }   
    }
    //debug serie usb
    //Serial.println(rt);
    affiche_RT(); 
  }
  
  void affiche_RT(){ 
      int i;   
      lcd.setCursor(0,2);
      for ( i =index_aff_rt-21 ; i < index_aff_rt-1; i++) {
        if (rt[i]=='\0') {
          index_aff_rt=20;
          //rt1[i]='\0';
          lcd.print(" ");
          break;
        }else{
          //rt1[i]=rt[i];
          lcd.print(rt[i]);
        }
          
      }
     index_aff_rt++;
        
  }  
  

   void minuterie_depart(){ 
    if(minuterie!=0 && minuterie_en_cours==0) {
        minuterie_en_cours=1;
        minuterie_temp_fin=minuterie*60+now();
        //le calcul suivant est ensuite effectue par le timer d'horloge 1 fois par seconde
        minuterie_temps_restant=(minuterie_temp_fin-now())/60; 
        //lancement de l'alarme de minuterie (en seconces)
        minuterie_id_timer=Alarm.timerOnce(minuterie*60, minuterie_beep);  
            
    }
  }
  
   void minuterie_arret(){ 
    if(minuterie_temps_restant!=0 && minuterie_en_cours==1) { 
        minuterie_en_cours=0;
        minuterie_temp_fin=0;
        minuterie=0;
        minuterie_temps_restant=0;
        Alarm.free(minuterie_id_timer);
        update_aff_minuterie();
    }
   }
 
   void minuterie_beep(){ 
      minuterie_en_cours=0;
      minuterie_temp_fin=0;
      minuterie=0;
      minuterie_temps_restant=0;
      update_aff_minuterie(); 

      Serial1.print(F("Cliquer pour arreter !"));
      int select;
      long timeout=8000;
      unsigned long  endTime = millis() + timeout;
      while(millis() < endTime) {
        digitalWrite(BUZZER,HIGH);
        Alarm.delay(100);
        digitalWrite(BUZZER,LOW);
        Alarm.delay(900);
        select = digitalRead(SEL);
        if(select==LOW){

          Serial1.print(F("                      "));
	  return;
	}
      }
      timeout=8000;
      endTime = millis() + timeout;
      while(millis() < endTime) {
        digitalWrite(BUZZER,HIGH);
        Alarm.delay(300);
        digitalWrite(BUZZER,LOW);
        Alarm.delay(700);
        select = digitalRead(SEL);
        if(select==LOW){

          Serial1.print(F("                      "));
	  return;
	}
      }
      timeout=10000;
      endTime = millis() + timeout;
      while(millis() < endTime) {
        digitalWrite(BUZZER,HIGH);
        Alarm.delay(700);
        digitalWrite(BUZZER,LOW);
        Alarm.delay(300);
        select = digitalRead(SEL);
        if(select==LOW){

          Serial1.print(F("                      "));
	  return;
	}
      }

      Serial1.print(F("                      "));
       
  }   
  
  void  minuterie_inc(){
    if (minuterie_en_cours!=1) {
        if(minuterie>90) {
            minuterie=90;
        }    
        if(minuterie>=40 && minuterie<90) {
            minuterie=minuterie+10;
        }
        if(minuterie>=10 && minuterie<=40) {
            minuterie=minuterie+5;
        } 
        if(minuterie<=10) {
            minuterie++;
        }
        update_aff_minuterie();
    }
  }
  
  void  minuterie_dec(){
    if (minuterie_en_cours!=1) {
        if(minuterie<11) {
            minuterie--;
         }
         if(minuterie<1) {
            minuterie=0;
         }    
        if(minuterie>10 && minuterie<41) {
            minuterie=minuterie-5;
        }
        if(minuterie>40 ) {
            minuterie=minuterie-10;
        }   
        update_aff_minuterie();
    }
  }
 
  void update_aff_minuterie(){ 

      lcd.setCursor(2, 3);
      if(minuterie<10 ) {
       lcd.print("0");
      }
      lcd.print(minuterie);
  } 
  
  void temp_press(){ 
      maj_pression_temp();
      lcd.setCursor(0, 0);
      lcd.print(F("        "));
      lcd.setCursor(0, 0);
      if (tend==0) {
          lcd.print(pression);
          lcd.setCursor(4, 0);
          lcd.print(F("hPa"));
          tend=1;
      } else {
          lcd.print(tendance);
          tend=0;
      }  
  }
  
  void horloge_radiotext_minuterie(){ 
      
      //on profite de l'aff de l'horloge pour scroller le radio text
      affiche_RT();
   
      //calcul du temps restant de la minuterie
      if (minuterie_en_cours==1) {
        minuterie_temps_restant=(minuterie_temp_fin-now())/60;      
      } 
      
      //gestion de l'horloge    
      if (hour()!=heure_courante) {
         lcd.setCursor(15,0); 
         if (hour()<10) {lcd.print("0");}
         lcd.print(hour());
         heure_courante=hour();
      } 
      if (minute()!=minute_courante) {
         lcd.setCursor(18,0); 
         if (minute()<10) {lcd.print("0");} 
         lcd.print(minute());
         
         lcd.setCursor(12,0);
         if (day()<10) {lcd.print("0");} 
         lcd.print(day());
         
         lcd.setCursor(8,0);
         switch (weekday()) {
            case 1:
              lcd.print("Dim.");
              break;
            case 2:
              lcd.print("Lun.");
              break;
            case 3:
              lcd.print("Mar.");
              break;
            case 4:
              lcd.print("Mer.");
              break;             
            case 5:
              lcd.print("Jeu.");
              break;             
            case 6:
              lcd.print("Ven.");
              break; 
            case 7:
              lcd.print("Sam.");
              break;             
         }
         
         minute_courante=minute();
      }  
      if (second()!=seconde_courante) {
         lcd.setCursor(17,0);         
         if ( (second() % 2) == 0) {
             lcd.print(" ");
         } else {
             lcd.print(":");
         }
         seconde_courante=second();
         
          
      }
    


  }
  
  void maj_pression_temp() {   
      //fonction de prise de mesure de la pression atmo et de la temprature ambiente
      pression=baro.readPressure()/100+12;  
    
      if (pression < 985)                         { sprintf(tendance, "Tempete");}    
      if (pression >= 985  &&  pression < 1005 )  { sprintf(tendance, "Pluie  "); }   
      if (pression >= 1005  &&  pression < 1021 ) { sprintf(tendance, "Varia. "); }   
      if (pression >= 1021  &&  pression < 1030 ) { sprintf(tendance, "Beau   "); }      
      if (pression >= 1030)                       { sprintf(tendance, "Sec"); } 
  }
  
 
  void blink_pseudo_curseur_et_minuterie(){  
    //clignotement du pseudo curseur en fonction de sa position su l'écran
    if (ligne_position_courante_curseur!=0 && col_position_courante_curseur!=0){
        lcd.setCursor(col_position_courante_curseur,ligne_position_courante_curseur);
        if(blink_cur==0 || blink_cur==1 ){
          lcd.write(' ');
          blink_cur++;
        }else{
          lcd.write(caractere_position_courante_curseur) ;
          if(blink_cur==5){
             blink_cur=0;
          }else{
             blink_cur++;
          }
        }
    }    
    //si une minuterie est demarree affichage de la valeur du temps restant
    //et clignotement
    if (minuterie_en_cours==1) {
        lcd.setCursor(2, 3);
        if( blink_cur>1 ){
          if(minuterie_temps_restant<10 ) {
                lcd.print("0");
            }
            lcd.print(minuterie_temps_restant); 
            
            if(minuterie_temps_restant<1 ) {
                lcd.setCursor(2, 3);
                if(minuterie_temp_fin-now()<10 ) {
                    lcd.print("0");
                }
                lcd.print(minuterie_temp_fin-now()); 
            } 
        }else{
           lcd.print(F("  ")); 
        } 
    } 
   
  }
   
 void splash_screen_alex(){ 
      lcd.setCursor(0, 1);
      lcd.print(F("   - RaDioDuino -   ")); 
      lcd.setCursor(0, 3);
      lcd.print(F("     A. OGER 2013-14"));
      delay(2000);
      lcd.clear();

  }
  


  
  

  
  
