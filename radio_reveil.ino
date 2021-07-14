///////////////////////////////
////  Alexandre OGER 2017  ////
///////////////////////////////


//radio reveil
// afficher LCD, horloge temps reel DS1307 et tuner FM sur bus I2C
// un servomoteur sur pin 3
// afficheur LCD sur pins 5,4,6,11,10,9 et 8
// reseau de bp sur pont divieur connecte a un pin ADC pour lire la valeur obtenue et determiner le bp qui a ete presse 

 /*                             
                                            ________| |__
                              MOSI          | |()|  | | | SCK
                              RX_LED/SS     |       | | | MISO
                              1/TX          |      ICSP | VIN
                              0/RX          |           | GND
                              RESET         |           | RESET
                              GND           |           | +5V
I2C-SDA (LCD-horloge-TUNER)---2/SDA         | ARDUINO   | NC
I2C-SCL (LCD-horloge-TUNER)---3(PWM)/SCL    |           | NC
            reset tuner fm ---4/A6          |           | A5  
                              5 (PWM)       |   MICRO   | A4   
                              6 (PWM)/A7    |           | A3    
                              7             |           | A2    
                              8/A9          |           | A1    
                              9 (PWM)/A10   |           | A0    --- reseau BP sur pont diviseur de tension
                              10 (PWM) /A11 |           | AREF
                              11(PWM)       |   _____   | 3.3V
  pin enable ampli class D ---12 (PWM)/A12  |  [ USB ]  | 13 (PWM)
                                            ----|___|---

*/

  //conf afficheur LCD 12x4
  #include <Wire.h> 
  #include <LiquidCrystal_I2C_ALEX.h>
  LiquidCrystal_I2C_ALEX lcd(0x27,20,4);
  
  // gestion du temps (horloge RTC, temps, timer)
  #include <DS1307RTC.h>
  #include <Time.h>
  #include <TimeAlarms.h>
  
  
  //appel librairie module radio sur bus I2C
  #include <Si4703_Breakout.h>
  
  //appel librairie EEPROM pour le stockage/lecture de la station mémorisée
  #include <EEPROM.h>
  
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



  uint8_t c0[8] = {     // caractere perso 0
      B00011,
      B00111,
      B01111,
      B11111,
      B00000,
      B00000,
      B00000,
      B00000
   };
  
  uint8_t c1[8] = {     // caractere perso 1
      B11000,
      B11100,
      B11110,
      B11111,
      B00000,
      B00000,
      B00000,
      B00000
   };
  
  uint8_t c2[8] = {    // caractere perso 2
      B11111,
      B11111,
      B11111,
      B11111,
      B00000,
      B00000,
      B00000,
      B00000
    };
  
  uint8_t c3[8] = {    // caractere perso 3
      B11111,
      B11110,
      B11100,
      B11000,
      B00000,
      B00000,
      B00000,
      B00000
    };
  
  uint8_t c4[8] = {   // caractere perso 4
      B11111,
      B01111,
      B00111,
      B00011,
      B00000,
      B00000,
      B00000,
      B00000
    };
  
  uint8_t c5[8] = {    // caractere perso 5
      B00011,
      B00111,
      B01111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111
    };
  
  uint8_t c6[8] = {    // caractere perso 6
      B11000,
      B11100,
      B11110,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111
    };
  
  uint8_t c7[8] = {     // caractere perso 7
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111
    };
  
  uint8_t cloche[8] = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
  
  
  // tblx des caracteres perso pour former les nums sur 3 colonnes + le 'N' sur 4 colonnes
  //             0                1         2          3           4              5          6          7           8            9    
  char bn1[]={ 5 ,2 , 6,     0 ,255,16,   0,2,6,    0, 2, 6,    255,16, 255,   255,2,3,   5, 2,1,    4,2 ,255,   5, 2, 6,     5,2, 6 };
  char bn2[]={ 255,16,255,   16,255,16,   5,2,3,    16,2,255,   4 , 2, 255,    2, 2,6,    255,2,6,   16, 5,3,    255,2,255,   4,2,255};
  char bn3[]={ 4 ,2 , 3,      2, 2 , 2,   4,2,3,    4, 2, 3,    16 ,16 ,2,     4, 2,3,    4, 2,3,    16,2,16,    4, 2, 3,     4,2, 3 };
  


  //init des variables de gestion de la FM et du RDS
  float  frequence;
  char ps[9]; //stockage des donnees RDS-PS (Program String -> nom de la station)
  int memoire_FM_OK=0;
  int id_ps; //id du timer de recherche du program station 
  int id_alarm_fin; //id du timer du fin de l'alarme (exticntion radio)
  int id_check;//id du timer de l'alarme


  //init du module radio sur bus I2c
  int resetPin = 4;
  int SDIO = 2; //alias SDA
  int SCLK = 3; //alias SCL
  Si4703_Breakout radio(resetPin, SDIO, SCLK);
  
  int heure_courante=0;
  int minute_courante=0;

  int heure_alarm=0;
  int minute_alarm=0;

  int alarme_active=0;
  int alarme=1;
  int alarme_longue=1;

  int radio_on_off=0;
  int reglage_on=0;
  int etape=0;

  int bouton=0;

  int ampli=12;
  int entree_bouton=A0;
  

void setup() {

  //init port serie sur usb
  Serial.begin(19200);

  //EEPROM.write(10,9);
  //EEPROM.write(11,31);
  //EEPROM.write(200,0);
  //EEPROM.write(1,51);
  //lecture heure et minute de l'alarme stockees dans l'eeprom -> adresse 10 et 11
  heure_alarm=EEPROM.read(10);
  minute_alarm=EEPROM.read(11);

  //extinction ampli audio
  pinMode(ampli,OUTPUT);
  digitalWrite(ampli,LOW);

  //entree boutons poussoirs
  pinMode(entree_bouton,INPUT);


  //demarrage I2C
  delay(200);
  Wire.begin();
  
  //init LCD
  lcd.init();                       
  lcd.init(); 
  lcd.backlight();
  lcd.clear();

  //on pousse les caracteres perso dans l'afficheur
  lcd.createChar(0,c0);
  lcd.createChar(1,c1);
  lcd.createChar(2,c2);
  lcd.createChar(3,c3);
  lcd.createChar(4,c4);
  lcd.createChar(5,c5);
  lcd.createChar(6,c6);
  lcd.createChar(7,cloche);


  delay(400);                                


  //definition de la synchro de l'horloge interne avec le module RTC toutes les 15 minutes
  setSyncProvider(RTC.get);
  setSyncInterval(900);
  delay(200);  
  lcd.setCursor(1,2);
  if(timeStatus()== timeSet) {       
       lcd.print(F("Synchro Horloge OK"));
    } else {
       lcd.print(F("Synchro impossible"));
   }
   
   //demarrge du module FM
  lcd.setCursor(1, 3);
  if (radio.powerOn()) {   
    lcd.print(F("Module FM - OK"));
  } else {
    lcd.print(F("Module FM - POK"));  
  }

  //extinction radio
  extinction_radio();

  delay(1000);

  lcd.clear();

  //petite anime de demarrage 
  splash_screen_alex(); 



  //mise à zéro chaine affichage RDS               
  ps[0]='\0';


  //timer toutes les sec pour l'alarme 
  id_check=Alarm.timerRepeat(1,alarme_radio);
  
  //timer toutes les 30 min pour la bascule heure ete/hiver
  Alarm.timerRepeat(20,ete_hiver);

}


void loop() {
  
     aff_heure();
  
     //trigger de declenchement de verif des timers d'alarme
     Alarm.delay(200);

     verif_bouton();

}



void verif_bouton(){
  
     int lecture_bouton = analogRead(entree_bouton);
     delay(50);
     int btn2 = analogRead(entree_bouton);
     
           //Serial.println(lecture_bouton);
          
     if (abs(lecture_bouton-btn2)<5 && lecture_bouton > 150) {
           Serial.println(lecture_bouton);
             if (lecture_bouton > 810 && lecture_bouton < 830 ) {
                bouton=1;
                //Serial.println("Set");
                reglage();
                return;
             }
             /*
             if (lecture_bouton > 55 && lecture_bouton < 85) {
                bouton=2;
                Serial.println("+");                
                if (reglage_on==1){   
                 
                }                 
                delay(300);
                return;
             }
             if (lecture_bouton > 180 && lecture_bouton < 210) {
                bouton=3;
                Serial.println("-");
                if (reglage_on==1){   
                 
                }                  
                delay(300);
                return;
             }
             */
             if (lecture_bouton > 760 && lecture_bouton < 780) {
                bouton=4;
                //Serial.println("FM+");
                if (radio_on_off==1){   
                  recherche_FM('h');
                }
                delay(300);
                return;
             }
             if (lecture_bouton > 1018 && lecture_bouton < 1021) {
                bouton=5;
                //Serial.println("MEM");
                delay(300);
                if (radio_on_off==1){   
                    memoire_FM_memoriser(1);
                }                
                return;
             }  
             if (lecture_bouton > 1014 && lecture_bouton < 1017) {
                bouton=6;
                //Serial.println("FM-");
                if (radio_on_off==1){   
                    recherche_FM('b');
                }                
                delay(300);
                return;
             }  
             if  (lecture_bouton > 715 && lecture_bouton < 735) {
                bouton=7;
                //Serial.println("alarm on/off");
                if (alarme==1){   
                    alarme=0;
                }else{
                    alarme=1;
                }
                alarme_radio();
                delay(1500);
                
                return;
             }  
             if (lecture_bouton > 935 && lecture_bouton < 955) {
                bouton=8;
                //Serial.println("timer");
                if (id_alarm_fin != 0) {
                  Alarm.free(id_alarm_fin);
                  id_alarm_fin=0;
                }                
                if (id_ps != 0) {
                  Alarm.free(id_ps);
                  id_ps=0;                  
                }
                lcd.setCursor(0, 3);
                lcd.print(F("                    "));
                lcd.setCursor(0, 3);
                lcd.print(F("Timer 1 heure"));
                delay(2000);
                allumage_radio();
                id_alarm_fin=Alarm.timerOnce(3600, extinction_radio);
                 radio_on_off=1;
                return;
             }    
             if (lecture_bouton > 865 && lecture_bouton < 885){
                bouton=9;
                //Serial.println("radio on/off");
                if (radio_on_off==1){   
                    extinction_radio();
                }else{
                    allumage_radio();
                }               
                delay(1500);
                return;
             }                                                      
     }
 
}

void refresh_reglage(){
    lcd.clear();  
    lcd.setCursor(0, 0);
    lcd.print("Alarme:");    
    lcd.setCursor(7, 1);
    if(heure_alarm<10 ){
       lcd.print("0");    }
    lcd.print(heure_alarm);
    lcd.print(":");
    if(minute_alarm<10 ){
      lcd.print("0");
    } 
    lcd.print(minute_alarm);         
    lcd.setCursor(0, 2);
    lcd.print("Heure:");      
    lcd.setCursor(7, 3);
    if(heure_courante<10 ){
       lcd.print("0");
    }
    lcd.print(heure_courante);
    lcd.print(":");
    if(minute_courante<10 ){
      lcd.print("0");
    } 
    lcd.print(minute_courante);  
    switch (etape) {
        case 1:
            lcd.setCursor(6,1);
            lcd.print(">");  
          break;
        case 2:
            lcd.setCursor(12,1);
            lcd.print("<");  
          break;
        case 3:
            lcd.setCursor(6,3);
            lcd.print(">");  
          break;
        case 4:
            lcd.setCursor(12,3);
            lcd.print("<");  
          break;              
     }
  
}

void reglage(){ 

    int boucle=0;
    int heure_alarme_stock=heure_alarm;
    int minute_alarme_stock=minute_alarm;
    int heure_stock=heure_courante;
    int minute_stock=minute_courante;    
    etape=1;
    if (id_check!=0){
       Alarm.free(id_check);
       id_check=0;      
    }  
    lcd.clear();
    refresh_reglage();
    delay(800);
    
    while(boucle==0){

         int lecture_bouton = analogRead(entree_bouton);
         delay(50);
         int btn2 = analogRead(entree_bouton);
         if (abs(lecture_bouton-btn2)<5 && lecture_bouton > 150) {
          Serial.println(btn2);
                 if (lecture_bouton > 810 && lecture_bouton < 830 ) {
                    //
                    etape++;
                    refresh_reglage();
                    if (etape==5){
                       lcd.clear(); 
                       boucle=1;
                       if (heure_alarme_stock != heure_alarm){
                            EEPROM.write(10,heure_alarm);
                      
                       }
                       if (minute_alarme_stock != minute_alarm){
                            EEPROM.write(11,minute_alarm);                          
                       }    
                       if (heure_stock != heure_courante || minute_stock != minute_courante){                           
                            //setTime(hr,min,sec,day,month,yr)
                            setTime(heure_courante,minute_courante,0,day(),month(),year());
                            RTC.set(now());                          
                       }                                            
                    }
                    delay(1000);                   
                 }
                 if (lecture_bouton > 1022 && lecture_bouton < 1024) {                               
                   switch (etape) {
                        case 1:
                          heure_alarm++;
                          if(heure_alarm==24){heure_alarm=0;}
                          refresh_reglage();
                          break;
                        case 2:
                          minute_alarm++;
                          if(minute_alarm==60){minute_alarm=0;}
                          refresh_reglage();
                          break;
                        case 3:
                          heure_courante++;
                          if(heure_courante==24){heure_courante=0;}
                          refresh_reglage();
                          break;
                        case 4:
                          minute_courante++;
                          if(minute_courante==60){minute_courante=0;}
                          refresh_reglage();
                          break;              
                    }           
                    delay(400);
                    
                 }
                 if (lecture_bouton > 990 && lecture_bouton < 1000) {
                    switch (etape) {
                        case 1:
                          heure_alarm--;
                          if(heure_alarm<0){heure_alarm=23;}
                          refresh_reglage();
                          break;
                        case 2:
                          minute_alarm--;
                          if(minute_alarm<0){minute_alarm=59;}
                          refresh_reglage();
                          break;
                        case 3:
                          heure_courante--;
                          if(heure_courante<0){heure_courante=23;}
                          refresh_reglage();
                          break;
                        case 4:
                          minute_courante--;
                          if(minute_courante<0){minute_courante=59;}
                          refresh_reglage();
                          break;              
                    }              
                    delay(400);
                    
                 }
          
         }
    }
    lcd.clear();
    heure_courante=0;
    minute_courante=0;
    aff_heure();
    alarme_radio();
    id_check=Alarm.timerRepeat(1,alarme_radio);
    if (radio_on_off==1){   
       affiche_infos_radios();
    }
}



void alarme_radio(){ 
  


  if (alarme==1){   
    aff_heure_alarme(1);
  }else{
    aff_heure_alarme(0);
  }
  if (alarme_active==1 && heure_alarm==heure_courante && minute_alarm+1==minute_courante ) {
    alarme_active=0;

 
  }
  if (alarme_active==0 && alarme==1 && heure_alarm==heure_courante && minute_alarm==minute_courante ) {
    allumage_radio();
    id_alarm_fin=Alarm.timerOnce(1800, extinction_radio);
    alarme_active=1;
    radio_on_off=1;
 
  }

  //Serial.println(alarme_active);


}

void aff_heure_alarme(int mode){ 
  lcd.setCursor(14,3);
  if (mode==1){   
     if (alarme_longue==1){ 
        if(heure_alarm<10 ){
           lcd.print(" ");
        }
        lcd.print(heure_alarm);
        lcd.print(":");
        if(minute_alarm<10 ){
          lcd.print("0");
        } 
        lcd.print(minute_alarm); 
     }
     lcd.setCursor(19,3);
     lcd.write(7);
  } else {
    if (alarme_longue==1){
        lcd.print("     "); 
    }   
    lcd.setCursor(19,3);
    lcd.print(" ");  
  }
     
}

void allumage_radio(){ 
  lcd.setCursor(0, 3);
  lcd.print(F("                    "));
  alarme_longue=0;
  aff_heure_alarme(1);
  lcd.setCursor(0, 3);
  if (radio.powerOn()) {
    lcd.print(F("FM - OK"));
  } else {
    lcd.print(F("FM - POK"));  
  }
  //volume module radio a fond
  radio.setVolume(10);
  ps[0]='\0';
  memoire_FM(1); 
  delay(500);
  //allumage ampli audio
  digitalWrite(ampli,HIGH);
  radio_on_off=1;
}

void extinction_radio(){ 
  //extinction ampli audio
  digitalWrite(ampli,LOW); 
  if (id_alarm_fin != 0) {
    Alarm.free(id_alarm_fin);
    id_alarm_fin=0;
  }                
  if (id_ps != 0) {
    Alarm.free(id_ps);
    id_ps=0;                  
  }
  lcd.setCursor(0, 3);
  lcd.print(F("                    "));  
  //extinction module FM
  lcd.setCursor(0, 3);
  if (radio.powerDown()) {
    lcd.print(F("Arret FM - OK"));
  } else {
    lcd.print(F("Arret FM - POK"));
  }
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print(F("                    "));
  alarme_longue=1;
  aff_heure_alarme(1);
  
  radio_on_off=0;
}

void affiche_infos_radios(){ 
      int i; 
      lcd.setCursor(0, 3);
      lcd.print(F("             "));      
      lcd.setCursor(9, 3);
      lcd.print(frequence,1); 
      lcd.setCursor(14, 3);
      lcd.print(F("FM"));
      id_ps=Alarm.timerOnce(5, chercher_PS);
}

void chercher_PS(){ 
    if (id_ps != 0) {
      Alarm.free(id_ps);
      id_ps=0;                  
    }
    lcd.setCursor(0, 3);
    lcd.print(F("        "));
    ps[0]='\0';
    radio.readPS(ps);
    lcd.setCursor(0, 3);
    lcd.print(ps);

}

void recherche_FM(char sens){  
    if (sens=='h')  {
      frequence=(float) radio.seekUp()/10;
      affiche_infos_radios();
    }
    if (sens=='b')  { 
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


void ete_hiver(){ 
      //format stockage: {2012,25,3,28,10}
      //                    0  1  2  3  4
      int i;
      heure_courante=hour();
      minute_courante=minute();
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


void aff_heure()
{



  if (second()%2 == 0){
    lcd.setCursor(10,0);
    lcd.write(161);
    lcd.setCursor(10,1);
    lcd.write(161);
  }else{
    lcd.setCursor(10,0);
    lcd.print(" ");
    lcd.setCursor(10,1);
    lcd.print(" ");
  }


   if(minute()!=minute_courante || hour()!=heure_courante){    

        if(hour()<10 ){
           affgrandnombre(0,1);
           affgrandnombre(hour(),2);
        } else {
           affgrandnombre((int) (hour()/10),1);
           affgrandnombre(hour()-(10*(int) (hour()/10)),2);
        }
        
        if(minute()<10 ){
          affgrandnombre(0,3);
          affgrandnombre(minute(),4);
        } else {
          affgrandnombre((int) (minute()/10),3);
          affgrandnombre(minute()-(10*(int) (minute()/10)),4);          
        }
        minute_courante=minute(); 
        heure_courante=hour(); 
   }
  
 

   
}

void affgrandnombre(int digit, int place)
{
    switch (place) {
        case 1:
          place=2;
          break;
        case 2:
          place=6;
          break;
        case 3:
          place=11;
        break;
        case 4:
           place=15;
        break;        
      }


        // chiffre de 0 à 9 sur 3 colonnes   
        // Ligne 1
        
        lcd.setCursor(place,0);
        lcd.write(bn1[digit*3]);
        lcd.write(bn1[digit*3+1]);
        lcd.write(bn1[digit*3+2]);
        // Ligne 2
        lcd.setCursor(place,1);
        lcd.write(bn2[digit*3]);
        lcd.write(bn2[digit*3+1]);
        lcd.write(bn2[digit*3+2]);
        // Ligne 3
        lcd.setCursor(place,2);
        lcd.write(bn3[digit*3]);
        lcd.write(bn3[digit*3+1]);
        lcd.write(bn3[digit*3+2]);    
     
  
}


 void splash_screen_alex(){ 
      lcd.setCursor(0, 1);
      lcd.print(F("   - RaDioReveil -  ")); 
      lcd.setCursor(0, 3);
      lcd.print(F("       A. OGER  2017"));
      delay(3000);
      lcd.clear();

  }
