
///////////////////////////////
////  Alexandre OGER 2021  ////
////     RADIODUINO2      ////
///////////////////////////////

// radio FM/DAB+ basee sur le DAB shield https://www.dabshield.com
// afficheur nextion 4.2" (communication sur port serie -> serial1)
// buzzer piezzo pour le compte minute
// encodeur rotatif pour l'IHM
// ampli adafruit class D 3.7w stereo (shutdown piloté par l'arduino)
  
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
              | [ ]IOREF                 MISO/12[ ] |   
              | [ ]RST                   MOSI/11[ ]~|   
              | [ ]3V3    +---+               10[ ]~|________ BUZZER
              | [ ]5v    -| A |-               9[ ]~|   
              | [ ]GND   -| R |-               8[ ] |________ SPI SELECTPIN DABSHIELD  
              | [ ]GND   -| D |-                    |
              | [ ]Vin   -| U |-               7[ ] |   
              |          -| I |-               6[ ]~|_________ bouton heure ete/hiver 
              | [ ]A0    -| N |-               5[ ]~|________ ENCODER ROTATIF BOUTON   
              | [ ]A1    -| O |-               4[ ] |________ ENCODER ROTATIF + 
              | [ ]A2     +---+           INT1/3[ ]~|________ ENCODER ROTATIF -  
              | [ ]A3                     INT0/2[ ] |   
              | [ ]A4/SDA  RST SCK MISO     TX>1[ ] |________ SERIAL 1 -> RX NEXTION   
              | [ ]A5/SCL  [ ] [ ] [ ]      RX<0[ ] |________ SERIAL 1 -> TX NEXTION    
              |            [ ] [ ] [ ]              |
              | UNO WIFI2  GND MOSI 5V  ____________/
               \_________________|_____/
                                 |
                                 |
                                 SPI_MOSI DABSHIELD
                                 SPI_MISO DABSHIELD
                                 SPI_SCK  DABSHIELD
 
 map mémoire:
 
address_offset=1;
address_fm1=2;
address_fm2=3;
address_dabmode=4;
address_mutliplex=5;
address_servicestation=6;
address_nbr_mltplx_dernierscan=7;
address_multiplex1=8;
address_multiplex2=9;
address_multiplex3=10;
address_multiplex4=11;
address_multiplex5=12;
address_multiplex6=13;
address_multiplex7=14;
address_multiplex8=15;
address_multiplex9=16;
address_multiplex10=17;

adress_nommultplex1= 20
adress_nommultplex2= 36
adress_nommultplex3= 52

adress_nommultplexX= 20+(X-1)*16;

 
 */

//appel de la librairie du watchdog processeur pour les resets auto
#include <avr/wdt.h>

#include <SPI.h>
#include <DABShield.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <SPI.h>
#include <EEPROM.h>
#include <RotaryEncoder.h>
#include <EasyNextionLibrary.h>

//creation des objets pour la gestion de l'afficheur nextion, de l'encodeur rotatif et du tuner
EasyNex myNex(Serial1);
RotaryEncoder encoder(4, 3);
DAB Dab;


//init des variables pour les pins du SPI
const byte SCKPin = 13;
const byte MISOPin = 12;
const byte MOSIPin = 11;
const byte slaveSelectPin=8;

//init des variables pour la gestion du tuner
int dabmode = 1;
DABTime dabtime;
uint8_t vol = 63;
uint8_t service = 0;
uint8_t srv = 0;
uint8_t multiplex=0;
uint8_t servicestation=0;
uint16_t freqfm=0;
uint8_t listmultiplex[32];
char nomsmultiplex[32][17];
int nbrmultiplexdispo=0;

//init des variables de stockage des infos station, radiotext,heure et heure  courante
byte rxindex = 0;
char page_defaut[10];
char rxdata[32];
char ps_fm[32];
char rt_fm[128];
char date_fm[11];
char heure_fm[6];
char ps_fm_courant[32];
int num_ps=0;
char rt_fm_courant[128];
char date_fm_courant[11];
char heure_fm_courant[6];


//init des variables  
int heure_offset=0;

//init des addresses de stockage en EEPROM: heure ete/hiver, freq fm en mhz (fm1x100 + fm2)
int address_offset=1;
int address_fm1=2;
int address_fm2=3;
int address_dabmode=4;
int address_mutliplex=5;
int address_servicestation=6;
int address_nbr_mltplx_dernierscan=7;
int address_multiplex1=8;
int address_multiplex2=9;
int address_multiplex3=10;
int address_multiplex4=11;
int address_multiplex5=12;
int address_multiplex6=13;
int address_multiplex7=14;
int address_multiplex8=15;
int address_multiplex9=16;
int address_multiplex10=17;

//buffer lecteur chaine en eeprom:
char buff_lit_eeprom[17];

//init des variables pour l'encodeur rotatif
int pos = 4;
int newPos =4;
int page =0;
int change_pos=1;
int entree_bouton=5;

//init des variables de gestion de la minuterie
int minuterie=0;
int minuterie_en_cours=0;
long minuterie_temp_fin=0;
int minuterie_temps_restant=0;
int minuterie_id_timer=10;

//init de la variable buzzer
int BUZZER = 10;

//init de la variable du pin du bp heure ete/hiver
int BPEH = 6;

int t6=1;

//init variable mode dab (station ou multiplex) station (0) ou multiplex(1)
int modedab=0;



void setup() {

  //init switch de l'encodeur rotatif et du buzzer
  pinMode(entree_bouton,INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
  digitalWrite(BUZZER,LOW);

  pinMode (BPEH, INPUT_PULLUP);


  //init port serie
  Serial.begin(115200);
  while(!Serial);

  //init liaison avec l'afficheur nextion sur serial1
  myNex.begin(9600);                    
  delay(700);  

  //init du tuner mode fm ou dab,vol, station par defaut
  //EEPROM.write(address_dabmode, 1);
  Serial.print("mode dab: ");
  dabmode=EEPROM.read(address_dabmode);
  Serial.println(dabmode);
  
  //reset de l'afficheur
  myNex.writeStr("rest");
  delay(700); 
  if(EEPROM.read(address_dabmode)== dabmode){
    page =0; 
  }else{
    page =1; 
  }
  sprintf(page_defaut,"page %01d",page);
  myNex.writeStr(page_defaut);
  myNex.writeStr("t0.txt", "BOOT...");
  myNex.writeNum("dim", 85);
  myNex.writeNum("g0.tim", 250);
  myNex.writeStr("g0.txt", "RADIODUINO 2  by Alex :-)");
  encoder.setPosition(4);

  
   
  //recup en eeprom offset heure hiver/ete
  heure_offset = EEPROM.read(address_offset);
  if(heure_offset==1){
    myNex.writeStr("t6.txt", "HIVER");
    
  }else{
    myNex.writeStr("t6.txt", "ETE");
  }
  // la variable bool defini au boot : t6 . check de la variable pour ne plus affichier ete ou hiver a partir du moment ou le nom de la station est ok en rds

  //raz de toutes les chaines de caracteres
  sprintf(ps_fm,"%s","");
  sprintf(rt_fm,"%s","");
  sprintf(date_fm,"%s","00/00/0000");
  sprintf(heure_fm,"%02d:%02d",0+heure_offset, 0);
  //sprintf(heure_fm,"%s","--:--");
  sprintf(ps_fm_courant,"%s","");
  sprintf(rt_fm_courant,"%s","");
  sprintf(date_fm_courant,"%s","00/00/0000");
  sprintf(heure_fm_courant,"%02d:%02d",0+heure_offset, 0);
  //sprintf(heure_fm_courant,"%s","--:--");

  //demarrage du bus SPI pour le tuner FM/DAB+ dabshield
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
  SPI.begin();
  Serial.println(F("Initialising.....")); 
  Serial.println(F(""));
  
  //damarrage dabshield
  Serial.println(F("*****************************************************"));
  Serial.println(F("!!! envoie sur le port serie en CR pas de CR/LF !!"));
  Serial.println(F("commandes: "));
  Serial.println(F(""));
  Serial.println(F("fm ou dab"));
  Serial.println(F("commandes: "));
  Serial.println(F("tune (fm: 8850-10800 dab: 0-37)"));
  Serial.println(F("service : dab changer de station"));
  Serial.println(F("volume 0-63"));
  Serial.println(F("info"));
  Serial.println(F("seek up ou down"));
  Serial.println(F("status "));
  Serial.println(F("time")); 
  Serial.println(F("aff +texte -> affiche sur le champ heure une chaine")); 
  Serial.println(F("*****************************************************"));
  Serial.println(F(""));
  Dab.setCallback(ServiceData);
  Dab.begin();
  if(Dab.error != 0)
  {
    Serial.print(F("ERROR: "));
    Serial.print(Dab.error);
    Serial.println(F("\nCheck DABShield is Connected and SPI Communications\n"));
  }
  else  
  {
    Serial.print(F("done\n\n")); 
  }


  Dab.begin(dabmode);
  Dab.vol(0);
  

  
  Dab.vol(62);

  //reglege bidon date et heure
  setTime(12,0,0,1,1,2021);

  //init FM
  //EEPROM.write(address_fm1, 92);
  //EEPROM.write(address_fm2, 60);
  Serial.print("freq par defaut en eeprom: ");
  freqfm=(uint16_t)(EEPROM.read(address_fm1)*100+EEPROM.read(address_fm2));
  Serial.println(freqfm);


  //init station DAB
  //EEPROM.write(address_mutliplex, 15);
  //EEPROM.write(address_servicestation, 1);
  multiplex=EEPROM.read(address_mutliplex);
  servicestation=EEPROM.read(address_servicestation);
  Serial.print("multiplex et station par defaut en eeprom: ");
  Serial.print(multiplex);Serial.print(",");
  Serial.println(servicestation);


  //init multiplex
  //EEPROM.write(address_nbr_mltplx_dernierscan, 2);
  //EEPROM.write(address_multiplex1, 15);
  //EEPROM.write(address_multiplex2, 26);
  //ecrit_chaine_eeprom(20,"amiens-local");
  //ecrit_chaine_eeprom(36,"amiens-etendu");
  nbrmultiplexdispo=EEPROM.read(address_nbr_mltplx_dernierscan);
  Serial.print("nbr de multiplex en eeprom:");Serial.println(nbrmultiplexdispo);
  for (int j=0; j< nbrmultiplexdispo;j++){
    lit_chaine_eeprom(20+j*16);
    sprintf(nomsmultiplex[j],"%s",buff_lit_eeprom);
    Serial.print(nomsmultiplex[j]);
    listmultiplex[j]=(uint8_t) EEPROM.read(8+j);
    Serial.print("-");Serial.println(listmultiplex[j]);
  }

  
  if(dabmode == 0)
  {
    Serial.print(F("DAB>"));
    Dab.set_service(servicestation);
    Dab.tune(multiplex); 
     

  }
  else
  {
    Serial.print(F("FM>"));
    Dab.tune(freqfm);
    sprintf(ps_fm, "%d.%02d Mhz", (int)Dab.freq / 100, (int)((Dab.freq % 100)/10)*10);
    sprintf(ps_fm_courant, "%d.%02d Mhz", (int)Dab.freq / 100, (int)((Dab.freq % 100)/10)*10);
    myNex.writeStr("t0.txt", ps_fm);
  }
 
  

}



void loop() {
    //taches recurentes pour le tuner, les alarms, l'update de la minuterie, l'etat de l'encoder, la maj de l'icone en focus sur l'afficheur, lecture de l'etat du bouton + action
    Dab.task();
    Alarm.delay(1);
    update_minuterie(); 
    encoder.tick();
    check_encodeur();
    focus_icone();
    lecture_bouton();
    set_heure_hiver_ete();
  
    if (Serial.available() > 0)
    {
      rxdata[rxindex] = Serial.read();
      if (rxdata[rxindex] == '\r')  //return
      {
        Serial.print(F("\n"));
        rxdata[rxindex] = '\0';
  
        process_command(rxdata);
        rxindex = 0;
      }
       else  //other char
      {
        Serial.print(rxdata[rxindex]);
        rxindex++;
        if (rxindex >= 32)
        {
          rxindex = 0;
        }
      }
    }
    
    if(strcmp(ps_fm, ps_fm_courant) != 0)
    { 
      
       Serial.print("changement ps:");
       Serial.print("!");Serial.print(ps_fm);Serial.println("!");
       if(page!=2){
        myNex.writeStr("t0.txt", ps_fm);
       }      
       sprintf(ps_fm_courant,"%s",ps_fm);
       if(t6==1){
        myNex.writeStr("t6.txt", "  ");
        t6=0;  
       } 
    }
    
    if(strcmp(rt_fm, rt_fm_courant) != 0)
    {
      Serial.print("changement rt:");
      Serial.print("!");Serial.print(rt_fm);Serial.println("!");
      if(page!=2){
        myNex.writeStr("g0.txt", rt_fm);
      }       
      sprintf(rt_fm_courant,"%s",rt_fm);
    }
    if(strcmp(date_fm, date_fm_courant) != 0)
    {
      Serial.print("changement date:");
      Serial.println(date_fm);
      sprintf(date_fm_courant,"%s",date_fm);
    }
    if(strcmp(heure_fm, heure_fm_courant) != 0)
    {
      Serial.print("changement heure:");
      Serial.println(heure_fm);  
      if(page!=2){
        myNex.writeStr("t6.txt", heure_fm);
      } 
      sprintf(heure_fm_courant,"%s",heure_fm);
    }
}



void check_encodeur(){
  newPos = encoder.getPosition();
  if (pos != newPos) {
    Serial.println(newPos);
    if (page==0 && newPos==8) {
      encoder.setPosition(0);
      newPos=0;
      
    }
    if (page==0 && newPos==-1) {
      newPos=7;
      encoder.setPosition(7);
      
    }
    if (page==1 && newPos==8) {
      encoder.setPosition(0);
      newPos=0;
      
    }
    if (page==1 && newPos==-1) {
      newPos=7;
      encoder.setPosition(7);
      
    }
    if (page==2 && newPos==14) {
      encoder.setPosition(13);
      newPos=13;
      
    }
    if (page==2 && newPos==-1) {
      newPos=0;
      encoder.setPosition(0);
      
    }
    if (page==2 && newPos> Dab.numberofservices-1 && newPos>pos && modedab==0 ) {
      newPos=13;
      encoder.setPosition(13); 
      
       Serial.println("monte"); 
    }
    if (page==2 && newPos== 12 && newPos<pos && modedab==0) {
       Serial.println("baisse"); 
      newPos=Dab.numberofservices-1;
      encoder.setPosition(Dab.numberofservices-1); 
    }
    if (page==2 && newPos> nbrmultiplexdispo-1 && newPos>pos && modedab==1 && newPos!=13 ) {
      newPos=12;
      encoder.setPosition(12);
    }
    if (page==2 && newPos==11 && newPos<pos && modedab==1 ) {
      newPos=nbrmultiplexdispo-1;
      encoder.setPosition(nbrmultiplexdispo-1);
    }
    Serial.println(newPos); 
    pos = newPos;
    change_pos=1;
  }
}



void lecture_bouton(){
     int lecture_bouton = digitalRead(entree_bouton);
     if (lecture_bouton==0) {
          Serial.println("click");
          analyse_bouton();    
          delay(650);
     }       
}



void analyse_bouton(){
   int i=0;
   char champtext[8];
   char bufferint[3];
   uint8_t freq_index=0;
   uint8_t multiplex1=0;
   if (page==0) {
       switch (pos) {
            case 0:   
              myNex.writeNum("p6.pic", 8);
              delay(200);
              myNex.writeNum("p6.pic", 7);
              myNex.writeStr("g0.txt", "");
              myNex.writeStr("t0.txt", "");
              Dab.seek(0, 1);
              sprintf(ps_fm, "%d.%02d Mhz", (int)Dab.freq / 100, (int)((Dab.freq % 100)/10)*10);
              myNex.writeStr("t0.txt", ps_fm); 
              break;   
            case 1:
              myNex.writeNum("p7.pic", 10);
              delay(200);
              myNex.writeNum("p7.pic", 9);
              myNex.writeStr("g0.txt", "");
              myNex.writeStr("t0.txt", "");
              Dab.seek(1, 1);   
              sprintf(ps_fm, "%d.%02d  Mhz", (int)Dab.freq / 100, (int)((Dab.freq % 100)/10)*10);
              myNex.writeStr("t0.txt", ps_fm);           
              break;   
            case 2:
              myNex.writeNum("p8.pic", 14);
              delay(200);
              myNex.writeNum("p8.pic", 13);
              Serial.println((int)Dab.freq / 100);
              Serial.println((int)((Dab.freq % 100)/10)*10);
              EEPROM.write(address_fm1, (int)Dab.freq / 100);
              EEPROM.write(address_fm2, (int)((Dab.freq % 100)/10)*10);
              break;   
            case 3:
              myNex.writeNum("p9.pic", 16);
              delay(200);
              myNex.writeNum("p9.pic", 15);
              myNex.writeStr("page 1");
              myNex.writeNum("p9.pic", 15);
              if(minuterie_en_cours==1) {
                  myNex.writeNum("n0.val", minuterie_temps_restant); 
                  myNex.writeNum("n0.pco", 61923); 
                  myNex.writeNum("t1.pco", 61923); 
              }
              myNex.writeStr("t0.txt", "Init. DAB");
              myNex.writeStr("t6.txt", "--:--");
              dabmode = 0;
              Dab.begin(dabmode); 
              Dab.tune(multiplex); 
              delay(1000);
              if(Dab.servicevalid() == true)
              {
                  Serial.println(Dab.Ensemble);
                  Serial.println(Dab.service[servicestation].Label);
                  Dab.set_service(servicestation);
              }
              encoder.setPosition(2);   
              page=1;         
              break;   
            case 4:
              myNex.writeNum("p5.pic", 5);
              delay(200);
              myNex.writeNum("p5.pic", 4);
              minuterie_inc();
              myNex.writeNum("n0.val", minuterie);
              break;   
            case 5:
              myNex.writeNum("p4.pic", 1);
              delay(200);
              myNex.writeNum("p4.pic", 0);
              minuterie_dec();
              myNex.writeNum("n0.val", minuterie);
              break;   
            case 6:
              myNex.writeNum("p3.pic", 3);
              delay(200);
              myNex.writeNum("p3.pic", 2);
              if(minuterie!=0 ) {
                  minuterie_depart();
                  myNex.writeNum("n0.val", minuterie_temps_restant); 
                  myNex.writeNum("n0.pco", 61923); 
                  myNex.writeNum("t1.pco", 61923); 
                  myNex.writeNum("p2.pic", 11);
                  myNex.writeNum("p6.pic", 8);
                  myNex.writeNum("p3.pic", 3);
                  encoder.setPosition(7);              
              }             
              break;   
            case 7:
              myNex.writeNum("p2.pic", 12);
              delay(200);
              myNex.writeNum("p2.pic", 11);
              minuterie_arret();
              myNex.writeNum("n0.val", minuterie);
              myNex.writeNum("n0.pco", 36863); 
              myNex.writeNum("t1.pco", 36863);
              break;                                         
      }
    return;
  }
  if (page==1) {
     switch (pos) {
            case 0: 
              page=2;
              modedab=0;
              myNex.writeStr("page 2");
              delay(200);
              myNex.writeStr("t14.txt", "STATIONS");
              myNex.writeStr("t13.txt", "retour");
              myNex.writeNum("t0.pco", 63488);
              for (i = 0; i < Dab.numberofservices; i++)
              {
                strcpy(champtext,"t");
                sprintf(bufferint, "%d", i);
                strcat(champtext, bufferint);
                strcat(champtext, ".txt");
                Serial.print(champtext);
                Serial.print(" ");
                Serial.println(Dab.service[i].Label);
                myNex.writeStr(champtext, Dab.service[i].Label);
              }
              encoder.setPosition(0); 
            break; 
            case 1: 
              myNex.writeNum("p8.pic", 14);
              delay(300);
              myNex.writeNum("p8.pic", 13);
              Serial.println(multiplex);
              Serial.println(servicestation);
              EEPROM.write(address_mutliplex, (int)multiplex);
              EEPROM.write(address_servicestation, (int)servicestation);
            break; 
            case 2: 
              myNex.writeNum("p9.pic", 16);
              delay(200);
              myNex.writeNum("p9.pic", 15);
              myNex.writeStr("page 0");
              if(minuterie_en_cours==1) {
                  myNex.writeNum("n0.val", minuterie_temps_restant); 
                  myNex.writeNum("n0.pco", 61923); 
                  myNex.writeNum("t1.pco", 61923); 
              }
              page=0;
              encoder.setPosition(3);
              dabmode = 1;
              sprintf(rt_fm_courant,"%s"," ");
              sprintf(ps_fm_courant,"%s"," ");
              myNex.writeStr("g0.txt", " ");
              myNex.writeStr("t0.txt", " ");
              Dab.begin(dabmode);  
              Dab.vol(0);
              Dab.tune((uint16_t)(EEPROM.read(address_fm1)*100+EEPROM.read(address_fm2)));  
              Dab.vol(55);          
            break; 
            case 3: 
              page=2;
              modedab=1;
              myNex.writeStr("page 2");
              delay(200);
              myNex.writeStr("t14.txt", "bouquets");
              myNex.writeStr("t13.txt", "retour");
              myNex.writeStr("t12.txt", "nouv. scan");
              myNex.writeNum("t0.pco", 63488);
              encoder.setPosition(0); 
              for (i=0; i< nbrmultiplexdispo;i++){
                  Serial.print(nomsmultiplex[i]);
                  strcpy(champtext,"t");
                  sprintf(bufferint, "%d", i);
                  strcat(champtext, bufferint);
                  strcat(champtext, ".txt");
                  Serial.print(champtext);
                  Serial.print(" ");
                  Serial.print("-");Serial.println(listmultiplex[i]);
                  //if(listmultiplex[i]==multiplex){
                  //  encoder.setPosition(i); 
                  //}
                  myNex.writeStr(champtext, nomsmultiplex[i]);
              }


            break; 
           case 4:
              myNex.writeNum("p5.pic", 5);
              delay(200);
              myNex.writeNum("p5.pic", 4);
              minuterie_inc();
              myNex.writeNum("n0.val", minuterie);
              break;   
            case 5:
              myNex.writeNum("p4.pic", 1);
              delay(200);
              myNex.writeNum("p4.pic", 0);
              minuterie_dec();
              myNex.writeNum("n0.val", minuterie);
              break;   
            case 6:
              myNex.writeNum("p3.pic", 3);
              delay(200);
              myNex.writeNum("p3.pic", 2);
              if(minuterie!=0 ) {
                  minuterie_depart();
                  myNex.writeNum("n0.val", minuterie_temps_restant); 
                  myNex.writeNum("n0.pco", 61923); 
                  myNex.writeNum("t1.pco", 61923); 
                  myNex.writeNum("p2.pic", 11);
                  myNex.writeNum("p6.pic", 21);
                  myNex.writeNum("p3.pic", 3);
                  encoder.setPosition(7);              
              }             
              break;   
            case 7:
              myNex.writeNum("p2.pic", 12);
              delay(200);
              myNex.writeNum("p2.pic", 11);
              minuterie_arret();
              myNex.writeNum("n0.val", minuterie);
              myNex.writeNum("n0.pco", 36863); 
              myNex.writeNum("t1.pco", 36863);
              break;             
     }
     return;
  }
  if (page==2) {


    if (pos<13){
      if(modedab==0){  
        servicestation=pos;           
        myNex.writeStr("page 1");
        myNex.writeNum("p9.pic", 16);
        delay(200);
        myNex.writeNum("p9.pic", 15);
        page=1;
        encoder.setPosition(2);  
        if(minuterie_en_cours==1) {
            myNex.writeNum("n0.val", minuterie_temps_restant); 
            myNex.writeNum("n0.pco", 61923); 
            myNex.writeNum("t1.pco", 61923); 
        }
        sprintf(ps_fm_courant,"%s","");
        sprintf(rt_fm_courant,"%s","");
        sprintf(heure_fm_courant,"%s","");
        sprintf(date_fm_courant,"%s","");

        Dab.set_service(servicestation); 
        return;       
      }else{
        if(pos==12){ 
            
            reset_couleur_police(0,13); 
            for (i = 0; i < nbrmultiplexdispo; i++)
            {
              strcpy(champtext,"t");
              sprintf(bufferint, "%d", i);
              strcat(champtext, bufferint);
              strcat(champtext, ".txt");
              myNex.writeStr(champtext, " ");
            }
            myNex.writeStr("t14.txt", "Patienter...");
            myNex.writeStr("t13.txt", "retour");
            myNex.writeStr("t12.txt", "nouv. scan");
            myNex.writeNum("t0.pco", 63488);
            encoder.setPosition(0);
            i=0;
            memset(listmultiplex,0,sizeof(listmultiplex));
            for (freq_index = 0; freq_index < DAB_FREQS; freq_index++){
              Dab.tune(freq_index);
              if(Dab.servicevalid() == true)
              {
                strcpy(champtext,"t");
                sprintf(bufferint, "%d", i);
                strcat(champtext, bufferint);
                strcat(champtext, ".txt");
                Serial.print(champtext);
                Serial.print("-");
                Serial.print(20+i*16);
                Serial.print("-");
                Serial.print(Dab.Ensemble);
                Serial.print("-");
                Serial.println((int)freq_index);
                myNex.writeStr(champtext, Dab.Ensemble);
                ecrit_chaine_eeprom(20+i*16,Dab.Ensemble);
                listmultiplex[i]=freq_index;
                EEPROM.write(8+i, (int)freq_index);
                nbrmultiplexdispo=i+1;
                i++;
              }
            }  
            EEPROM.write(address_nbr_mltplx_dernierscan, nbrmultiplexdispo);  
            myNex.writeStr("t14.txt", "bouquets");    
        }
        if(pos < nbrmultiplexdispo+1 ){  
            modedab=0;
            myNex.writeStr("page 2");
            page=2;  
            myNex.writeStr("t14.txt", "Patienter...");
            myNex.writeStr("t13.txt", "retour");
            myNex.writeNum("t0.pco", 63488);
            encoder.setPosition(0); 
            multiplex1=listmultiplex[pos];
            //if(multiplex1!=multiplex){
              Dab.tune(multiplex1);
            //}
            
            for (i = 0; i < Dab.numberofservices; i++)
            {
              strcpy(champtext,"t");
              sprintf(bufferint, "%d", i);
              strcat(champtext, bufferint);
              strcat(champtext, ".txt");
              Serial.print(champtext);
              Serial.print(" ");
              Serial.println(Dab.service[i].Label);
              myNex.writeStr(champtext, Dab.service[i].Label);
              
            }
            myNex.writeStr("t14.txt", "STATIONS");
        }

        return;      
      }
      
    }else if (pos==13){
      if(modedab==0){  
        myNex.writeStr("page 1");
        myNex.writeNum("p9.pic", 16);
        delay(200);
        sprintf(ps_fm_courant,"%s","");
        sprintf(rt_fm_courant,"%s","");
        sprintf(heure_fm_courant,"%s","");
        sprintf(date_fm_courant,"%s","");
        page=1;
        encoder.setPosition(2); 
        if(minuterie_en_cours==1) {
            myNex.writeNum("n0.val", minuterie_temps_restant); 
            myNex.writeNum("n0.pco", 61923); 
            myNex.writeNum("t1.pco", 61923); 
        }         
        return;       
      }else{
        modedab=0;
        myNex.writeStr("page 2");
        page=2;
        delay(200);
        myNex.writeStr("t14.txt", "stations");
        myNex.writeStr("t13.txt", "retour");
        myNex.writeNum("t0.pco", 63488);
        encoder.setPosition(0); 

        Dab.tune(multiplex);
        delay(800);
        for (i = 0; i < Dab.numberofservices; i++)
        {
          strcpy(champtext,"t");
          sprintf(bufferint, "%d", i);
          strcat(champtext, bufferint);
          strcat(champtext, ".txt");
          Serial.print(champtext);
          Serial.print(" ");
          Serial.println(Dab.service[i].Label);
          myNex.writeStr(champtext, Dab.service[i].Label);
        }
        return;       
      }      
    }
       
       return;
  }


}



void focus_icone(){
  
   if(change_pos==1){    
      if (page==0) {
          switch (pos) {
            case 0:
              myNex.writeNum("p6.pic", 7);
              myNex.writeNum("p7.pic", 10);
              myNex.writeNum("p2.pic", 12);
              Serial.println("ok");
              break;            
            case 1:
              myNex.writeNum("p7.pic", 9);
              myNex.writeNum("p6.pic", 8);
              myNex.writeNum("p8.pic", 14);
              Serial.println("ok");
              break;
            case 2:
              myNex.writeNum("p8.pic", 13);
              myNex.writeNum("p7.pic", 10);
              myNex.writeNum("p9.pic", 16);
              Serial.println("ok");
              break;
            case 3:
              myNex.writeNum("p9.pic", 15);
              myNex.writeNum("p5.pic", 5);
              myNex.writeNum("p8.pic", 14);
              Serial.println("ok");
              break;
             case 4:
              myNex.writeNum("p5.pic", 4);
              myNex.writeNum("p4.pic", 1);
              myNex.writeNum("p9.pic", 16);
              Serial.println("ok");
              break;
            case 5:
              myNex.writeNum("p4.pic", 0);
              myNex.writeNum("p5.pic", 5);
              myNex.writeNum("p3.pic", 3);
              Serial.println("ok");
              break;
            case 6:
              myNex.writeNum("p3.pic", 2);
              myNex.writeNum("p4.pic", 1);
              myNex.writeNum("p2.pic", 12);
              Serial.println("ok");
              break;
            case 7:
              myNex.writeNum("p2.pic", 11);
              myNex.writeNum("p6.pic", 8);
              myNex.writeNum("p3.pic", 3);
              Serial.println("ok");
              break;                                                                                 
          }
      }    
      if (page==1) {
          switch (pos) {
            case 0:
              myNex.writeNum("p6.pic", 20);
              myNex.writeNum("p8.pic", 14);
              myNex.writeNum("p2.pic", 12);
              Serial.println("ok");
              break;            
            case 1:
              myNex.writeNum("p8.pic", 13);
              myNex.writeNum("p6.pic", 21);
              myNex.writeNum("p9.pic", 16);
              Serial.println("ok");
              break;
            case 2:
              myNex.writeNum("p9.pic", 15);
              myNex.writeNum("p8.pic", 14);
              myNex.writeNum("p7.pic", 19);
              Serial.println("ok");
              break;
            case 3:
              myNex.writeNum("p7.pic", 18);
              myNex.writeNum("p9.pic", 16);
              myNex.writeNum("p5.pic", 5);
              Serial.println("ok");
              break;
             case 4:
              myNex.writeNum("p5.pic", 4);
              myNex.writeNum("p4.pic", 1);
              myNex.writeNum("p7.pic", 19);
              Serial.println("ok");
              break;
            case 5:
              myNex.writeNum("p4.pic", 0);
              myNex.writeNum("p5.pic", 5);
              myNex.writeNum("p3.pic", 3);
              Serial.println("ok");
              break;
            case 6:
              myNex.writeNum("p3.pic", 2);
              myNex.writeNum("p4.pic", 1);
              myNex.writeNum("p2.pic", 12);
              Serial.println("ok");
              break;
            case 7:
              myNex.writeNum("p2.pic", 11);
              myNex.writeNum("p6.pic", 21);
              myNex.writeNum("p3.pic", 3);
              Serial.println("ok");
              break;                                                                                
          }
      }
      if (page==2) {
          if (modedab==0 && pos== Dab.numberofservices-1){
                reset_couleur_police(Dab.numberofservices,13);              
          } 
          if (modedab==1 && pos==nbrmultiplexdispo-1){
                reset_couleur_police(nbrmultiplexdispo,13);              
          } 
          switch (pos) {
            case 0:
              myNex.writeNum("t13.pco", 36863);
              myNex.writeNum("t0.pco", 63488);
              myNex.writeNum("t1.pco", 36863);
              Serial.println("ok");
              break;            
            case 1:
              myNex.writeNum("t0.pco", 36863);
              myNex.writeNum("t1.pco", 63488);
              myNex.writeNum("t2.pco", 36863);
              Serial.println("ok");
              break;
            case 2:
              myNex.writeNum("t1.pco", 36863);
              myNex.writeNum("t2.pco", 63488);
              myNex.writeNum("t3.pco", 36863);
              Serial.println("ok");
              break;
            case 3:
              myNex.writeNum("t2.pco", 36863);
              myNex.writeNum("t3.pco", 63488);
              myNex.writeNum("t4.pco", 36863);
              Serial.println("ok");
              break;
             case 4:
              myNex.writeNum("t3.pco", 36863);
              myNex.writeNum("t4.pco", 63488);
              myNex.writeNum("t5.pco", 36863);
              Serial.println("ok");
              break;
            case 5:
              myNex.writeNum("t4.pco", 36863);
              myNex.writeNum("t5.pco", 63488);
              myNex.writeNum("t6.pco", 36863);
              Serial.println("ok");
              break;
            case 6:
              myNex.writeNum("t5.pco", 36863);
              myNex.writeNum("t6.pco", 63488);
              myNex.writeNum("t7.pco", 36863);
              Serial.println("ok");
              break;
            case 7:
              myNex.writeNum("t6.pco", 36863);
              myNex.writeNum("t7.pco", 63488);
              myNex.writeNum("t8.pco", 36863);
              Serial.println("ok");
              break;    
            case 8:
              myNex.writeNum("t7.pco", 36863);
              myNex.writeNum("t8.pco", 63488);
              myNex.writeNum("t9.pco", 36863);
              Serial.println("ok");
              break;  
            case 9:
              myNex.writeNum("t8.pco", 36863);
              myNex.writeNum("t9.pco", 63488);
              myNex.writeNum("t10.pco", 36863);
              Serial.println("ok");
              break;   
            case 10:
              myNex.writeNum("t9.pco", 36863);
              myNex.writeNum("t10.pco", 63488);
              myNex.writeNum("t11.pco", 36863);
              Serial.println("ok");
              break;       
            case 11:
              myNex.writeNum("t10.pco", 36863);
              myNex.writeNum("t11.pco", 63488);
              myNex.writeNum("t12.pco", 36863);
              Serial.println("ok");
              break;   
            case 12:
              reset_couleur_police(nbrmultiplexdispo-1,13);              
              myNex.writeNum("t12.pco", 63488);
              myNex.writeNum("t13.pco", 36863);
              Serial.println("ok");
              break; 
            case 13:
             if (modedab==0 ){
                reset_couleur_police(Dab.numberofservices-1,13);              
             } 
             if (modedab==1 ){
                reset_couleur_police(nbrmultiplexdispo,13);              
             } 
             myNex.writeNum("t13.pco", 63488);
             Serial.println("ok");
             break;                                                                          
          }
      }
   }
  change_pos=0;
}



void reset_couleur_police(int debut,int fin){
   char champtext[8];
   char bufferint[3];
   for (int i = debut; i < fin+1; i++){
          //Serial.print(i);  
          //Serial.print(" "); 
          strcpy(champtext,"t");
          sprintf(bufferint, "%d", i);
          strcat(champtext, bufferint);
          strcat(champtext, ".pco");
          //Serial.println(champtext);
          myNex.writeNum(champtext, 36863);
    }
  
}




void ServiceData(void)
{
  Serial.println("");
  Serial.println("appel callback");
  if (dabmode == 0)
  {
    sprintf(ps_fm,"%s",Dab.service[servicestation].Label);
    sprintf(rt_fm,"%s",Dab.ServiceData);   
    Dab.time(&dabtime);    
    sprintf(date_fm,"%02d/%02d/%04d",  dabtime.Days,dabtime.Months,dabtime.Year);
    sprintf(heure_fm,"%02d:%02d",dabtime.Hours,dabtime.Minutes);
    //Serial.println(Dab.service[servicestation].CompID); 
    //Serial.print("!");Serial.print(ps_fm);Serial.println("!");
    //Serial.print("!");Serial.print(rt_fm);Serial.println("!");
    //Serial.println(date_fm);
    //Serial.println(heure_fm);
  }
  else
  {
    sprintf(ps_fm,"%s",Dab.ps);
    sprintf(rt_fm,"%s",Dab.ServiceData);
    sprintf(date_fm,"%02d/%02d/%04d", Dab.Days, Dab.Months, Dab.Year);
    sprintf(heure_fm,"%02d:%02d",Dab.Hours+heure_offset, Dab.Minutes);
    char freqstring[32];
    Dab.status();
    sprintf(freqstring, "Freq = %3d.", (uint16_t)Dab.freq / 100);
    Serial.print(freqstring);
    sprintf(freqstring, "%1d MHz : ", (uint16_t)(Dab.freq % 100)/10);
    Serial.print(freqstring);   
    sprintf(freqstring,"RSSI = %d, ",Dab.signalstrength);
    Serial.print(freqstring);
    sprintf(freqstring,"SNR = %d\n",Dab.snr);
    Serial.print(freqstring);  
  }
  
}



void set_heure_hiver_ete(){ 

 int bp= digitalRead(BPEH);
 if(bp == LOW) {
      int valeur = EEPROM.read(address_offset);
      if(valeur==1){
        EEPROM.write(address_offset, 2);
      }else{
        EEPROM.write(address_offset, 1);
      }
      delay(500);
      reboot();
      delay(500);
      
  }
}

void reboot()
{
  int Pin=A0; 
  pinMode(Pin, OUTPUT); 
  digitalWrite(Pin, LOW);
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while(1)
  {

  }
}


void update_minuterie(){
   if (minuterie_en_cours==1 && minuterie_temps_restant!=int((minuterie_temp_fin-now())/60)) {
        minuterie_temps_restant=int((minuterie_temp_fin-now())/60);  
        if(minuterie_temps_restant<0){
          minuterie_temps_restant=0;
        }
        myNex.writeNum("n0.val", minuterie_temps_restant+1);  
        myNex.writeNum("n0.pco", 61923); 
        myNex.writeNum("t1.pco", 61923);   
        Serial.println("update minuterie");  
   } 
}



void minuterie_depart(){ 
    if(minuterie!=0 && minuterie_en_cours==0) {
        minuterie_en_cours=1;
        minuterie_temp_fin=minuterie*60+now();
        //le calcul suivant est ensuite effectue par le timer d'horloge 1 fois par seconde
        minuterie_temps_restant=(minuterie_temp_fin-now())/60; 
        //lancement de l'alarme de minuterie (en seconces)
        minuterie_id_timer=Alarm.timerOnce(minuterie*60, minuterie_beep); 
        Serial.print("timer minuterie: ");   
        Serial.println(minuterie_id_timer);  
            
    }
 }


  
void minuterie_arret(){ 
    //if(minuterie_temps_restant!=0 && minuterie_en_cours==1) { 
        minuterie_en_cours=0;
        minuterie_temp_fin=0;
        minuterie=0;
        minuterie_temps_restant=0;
        Alarm.free(minuterie_id_timer);
        minuterie_id_timer=10;
        Serial.print("minuterie arret ");  
    //}
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
    }
}



void minuterie_beep(){ 
      minuterie_en_cours=0;
      minuterie_temp_fin=0;
      minuterie=0;
      minuterie_temps_restant=0;
      myNex.writeNum("n0.val", minuterie);
      Serial.println("BEEEEEEEEEPPP!"); 
      int select;
      long timeout=8000;
      unsigned long  endTime = millis() + timeout;
      unsigned long temp_actuel;
      while(millis() < endTime) {
          Alarm.delay(1);
          digitalWrite(BUZZER,HIGH);
          temp_actuel = millis();
          while(millis()-temp_actuel<100){
             select = digitalRead(entree_bouton);
             if(select==LOW){
                digitalWrite(BUZZER,LOW);
                myNex.writeNum("n0.val", minuterie);
                myNex.writeNum("n0.pco", 36863); 
                myNex.writeNum("t1.pco", 36863);
                return;
             }     
          }
          //Alarm.delay(100);
          digitalWrite(BUZZER,LOW);
          temp_actuel = millis();
          while(millis()-temp_actuel<900){
             select = digitalRead(entree_bouton);
             if(select==LOW){
                myNex.writeNum("n0.val", minuterie);
                myNex.writeNum("n0.pco", 36863); 
                myNex.writeNum("t1.pco", 36863);
                return;
             }     
          }
          //Alarm.delay(900);

      }
      timeout=8000;
      endTime = millis() + timeout;
      while(millis() < endTime) {
          Alarm.delay(1);
          digitalWrite(BUZZER,HIGH);
          //Alarm.delay(300);
          temp_actuel = millis();
          while(millis()-temp_actuel<300){
             select = digitalRead(entree_bouton);
             if(select==LOW){
                digitalWrite(BUZZER,LOW);
                myNex.writeNum("n0.val", minuterie);
                myNex.writeNum("n0.pco", 36863); 
                myNex.writeNum("t1.pco", 36863);
                return;
             }     
          }
          digitalWrite(BUZZER,LOW);
          //Alarm.delay(700);
          temp_actuel = millis();
          while(millis()-temp_actuel<700){
             select = digitalRead(entree_bouton);
             if(select==LOW){
                myNex.writeNum("n0.val", minuterie);
                myNex.writeNum("n0.pco", 36863); 
                myNex.writeNum("t1.pco", 36863);
                return;
             }     
          }
      }
      timeout=10000;
      endTime = millis() + timeout;
      while(millis() < endTime) {
            Alarm.delay(1);
            digitalWrite(BUZZER,HIGH);
            //Alarm.delay(700);
            temp_actuel = millis();
            while(millis()-temp_actuel<700){
               select = digitalRead(entree_bouton);
               if(select==LOW){
                  digitalWrite(BUZZER,LOW);
                  myNex.writeNum("n0.val", minuterie);
                  myNex.writeNum("n0.pco", 36863); 
                  myNex.writeNum("t1.pco", 36863);
                  return;
               }     
            }
            digitalWrite(BUZZER,LOW);
            //Alarm.delay(300);
            temp_actuel = millis();
            while(millis()-temp_actuel<300){
               select = digitalRead(entree_bouton);
               if(select==LOW){
                  myNex.writeNum("n0.val", minuterie);
                  myNex.writeNum("n0.pco", 36863); 
                  myNex.writeNum("t1.pco", 36863);
                  return;
               }     
            }

      }      
 }  


void ecrit_chaine_eeprom(int nbr, char* chaine){
  int i=0;
  for (i = 0; i < strlen(chaine)+1; i++){
    EEPROM.write(nbr+i, chaine[i]);    
  }  
}


void lit_chaine_eeprom(int nbr){
  int i=0;
  for (i = 0; i < 16; i++){
   buff_lit_eeprom[i]=(char)EEPROM.read(nbr+i);
   if (buff_lit_eeprom[i]=='\0'){
    return;
   }
  } 
}




void process_command(char *command)
{
  char *cmd;
  cmd = strtok(command, " \r");
  if (strcmp(cmd, "tune") == 0)
  {
    cmd = strtok(NULL, " \r");
    if(dabmode == 0)
    {
      srv = (int)strtol(cmd, NULL, 10);
      service = 0;
      multiplex=srv;
      Dab.tune(srv);
      Ensemble_Info();
    }
    else
    {
      uint32_t freqkhz = strtol(cmd, NULL, 10);
      if((freqkhz >= 87500) && (freqkhz <= 107900))
      {
        Dab.tune((uint16_t)(freqkhz/10));
        FM_status();      
      }
      else if((freqkhz >= 8750) && (freqkhz <= 10790))
      {
        Dab.tune((uint16_t)freqkhz);      
        FM_status();
      }
      else
      {
        Serial.print(F("Freq not in range\n"));
      }
    }
  }
  else if (strcmp(cmd, "service") == 0)
  {
    cmd = strtok(NULL, " \r");
    service = (uint8_t)strtol(cmd, NULL, 10);
    Dab.set_service(service);
    servicestation=service;
    Serial.print(Dab.service[service].Label);
    Serial.print(F("\n"));
  }
  else if (strcmp(cmd, "volume") == 0)
  {
    cmd = strtok(NULL, " \r");
    vol = (uint8_t)strtol(cmd, NULL, 10);
    Dab.vol(vol);
  }
  else if (strcmp(cmd, "info") == 0)
  {
    Ensemble_Info();
  }
  else if (strcmp(cmd, "scan") == 0)
  {
    if(dabmode == 0)
    {
      DAB_scan();
    }
    else
    {
      FM_scan();
    }
  }
  else if (strcmp(cmd, "fm") == 0)
  {
    
    dabmode = 1;
    Dab.begin(dabmode);
    Dab.tune((uint16_t)8750);

  }  
  else if (strcmp(cmd, "dab") == 0)
  {
    
    dabmode = 0;
    Dab.begin(dabmode);

  }  
  else if (strcmp(cmd, "seek") == 0)
  {
    bool valid = false;
    cmd = strtok(NULL, " \r");
    if(strcmp(cmd, "up") == 0)
    {
      valid = Dab.seek(1, 1);
    }
    else if(strcmp(cmd, "down") == 0)
    {
      valid = Dab.seek(0, 1);
    }
    if(valid == true)
    {
      FM_status();      
    }
  }
  else if (strcmp(cmd, "aff") == 0)
  {
    bool valid = false;
    cmd = strtok(NULL, " \r");
    myNex.writeStr("t6.txt", cmd);

  }
  else if (strcmp(cmd, "status") == 0)
  {
    FM_status(); 
  }
  else if (strcmp(cmd, "time") == 0)
  {
    char timestring[16];
    Dab.time(&dabtime);
    sprintf(timestring,"%02d/%02d/%02d ", dabtime.Days,dabtime.Months,dabtime.Year);
    Serial.print(timestring);
    sprintf(timestring,"%02d:%02d\n", dabtime.Hours,dabtime.Minutes);
    Serial.print(timestring);
  } 
  else if (strlen(command) == 0)
  {
    //no command
  }
  else
  {
    Serial.print(F("Unknown command\n"));
  }
  if(dabmode == 0)
  {
    Serial.print(F("DAB>"));
  }
  else
  {
    Serial.print(F("FM>"));
  }
}



void Ensemble_Info(void)
{
  char freqstring[32];
  uint8_t i;
  Serial.print(F("\n\nEnsemble Freq "));
  sprintf(freqstring, "%02d\t %03d.", Dab.freq_index, (uint16_t)(Dab.freq_khz(Dab.freq_index) / 1000));
  Serial.print(freqstring);
  sprintf(freqstring, "%03d MHz", (uint16_t)(Dab.freq_khz(Dab.freq_index) % 1000));
  Serial.print(freqstring);
  Serial.print(F("\n"));
  Serial.print(Dab.Ensemble);
  Serial.print(F("\n"));
  Serial.print(F("\nServices: \n"));
  Serial.print(F("ID\tName\tcompid\n\n"));
  for (i = 0; i < Dab.numberofservices; i++)
  {
    Serial.print(i);
    Serial.print(F(":\t"));
    Serial.print(Dab.service[i].Label);
    Serial.print(F(":\t"));
    Serial.print(Dab.service[i].CompID);
    Serial.print(F("\n"));
  }
  Serial.print(F("\n"));
}



void DAB_scan(void)
{
  uint8_t freq_index;
  char freqstring[32];
  for (freq_index = 0; freq_index < DAB_FREQS; freq_index++)
  {
    Serial.print(F("\nScanning Freq "));
    sprintf(freqstring, "%02d\t %03d.", freq_index, (uint16_t)(Dab.freq_khz(freq_index) / 1000));
    Serial.print(freqstring);
    sprintf(freqstring, "%03d MHz", (uint16_t)(Dab.freq_khz(freq_index) % 1000));
    Serial.print(freqstring);
    Dab.tune(freq_index);
    if(Dab.servicevalid() == true)
    {
      Ensemble_Info();
    }
  }
  Serial.print(F("\n\n"));
}



void FM_status(void)
{
  char freqstring[32];
  Dab.status();
  sprintf(freqstring, "Freq = %3d.", (uint16_t)Dab.freq / 100);
  Serial.print(freqstring);
  sprintf(freqstring, "%1d MHz : ", (uint16_t)(Dab.freq % 100)/10);
  Serial.print(freqstring);   
  sprintf(freqstring,"RSSI = %d, ",Dab.signalstrength);
  Serial.print(freqstring);
  sprintf(freqstring,"SNR = %d\n",Dab.snr);
  Serial.print(freqstring);     
}



void FM_scan(void)
{
  uint16_t startfreq = Dab.freq;   
  Dab.vol(0);
  Dab.tune((uint16_t)8750);
  while(Dab.seek(1, 0) == true)
  {
    FM_status();
  }
  Dab.tune(startfreq);
  Dab.vol(vol);
}



void DABSpiMsg(unsigned char *data, uint32_t len)
{
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));    //2MHz for starters...
  digitalWrite (slaveSelectPin, LOW);
  SPI.transfer(data, len);
  digitalWrite (slaveSelectPin, HIGH);
  SPI.endTransaction();
}
