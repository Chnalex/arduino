
///////////////////////////////
//// Alexandre OGER 2014   ////
///////////////////////////////


//puissance 4 robotisé

//un afficheur LCD I2c
//un ensemble de bouton (1 a 7 pour le choix de la colonne) relié à des ponts de résitance en entrée sur A4
//7 leds (une par bouton) en sortie numerique sur les pins  4, 5, 6, 7, 8, 9 et 23(A5)
//une carte lynx motion SSC-32 reliée par un port serie soft sur les pins 10 et 11 (la carte pilote 6 servo moteurs)


/*|
                              
                                            ________| |__
                              MOSI          | |()|  | | | SCK
                              RX_LED/SS     |       | | | MISO
                              1/TX          |      ICSP | VIN
                              0/RX          |           | GND
                              RESET         |           | RESET
                              GND           |           | +5V
 SDA I2C (AFF LCD)  -------   2/SDA         | ARDUINO   | NC
 SCL I2C (AFF LCD)  -------   3(PWM)/SCL    |           | NC
            LED 1   -------   4/A6          |           | A5   -------- (pin 23) LED 7
            LED 2   -------   5 (PWM)       |   MICRO   | A4   -------- reseaux pont de resistance et boutons
            LED 3   -------   6 (PWM)/A7    |           | A3
            LED 4   -------   7             |           | A2
            LED 5   -------   8/A9          |           | A1
            LED 6   -------   9 (PWM)/A10   |           | A0
    RX SERIE SOFT   -------   10 (PWM) /A11 |           | AREF
    TX SERIE SOFT   -------   11(PWM)       |   _____   | 3.3V
                              12 (PWM)/A12  |  [ USB ]  | 13 (PWM)
                                            ----|___|---

*/

//ecran lcd
#include "Wire.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);


//init du port serie soft sur les pin 10 et 11 pour dialoguer avec la carte de controle du robot
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX

//robot actif ou non
int robot_actif=1;

//les 42 cases du plateau de jeu
int plateau[7][6];
int plateau_sauv[7][6];
int plateau_ia[7][6];

//variable stockage scores pour l'IA
int tab_score[7];

//hauteur courante jouée par l'IA
int hauteur_ia;

//variable tempo joueur courant 1->blanc 2->noir
int joueur=1;

//nbr de coups joues depuis le debut de la partie
int compteur_coups=0;

//type de partie 0-> humain contre humain; 1->humain contre ordinateur
int type_partie;

//valeur du bouton pressé
int bouton;

//colonne courante jouée
int colonne;

//hauteur courante jouée
int hauteur;

//pins des leds de 1 à 7
int LedPins[] = {4, 5, 6, 7, 8, 9, 23};






void setup() {

  // demarrage port serie soft sur les pins 10 et 11
  mySerial.begin(38400);
  Serial.begin(38400); 
  Serial.println(F("Hello"));
  Serial.println("");
  Serial.println(F("Puissance 4 - A. OGER  2011-2014"));
  Serial.println("");

  Serial.println(F("init phase 1"));
  
  
  
  lcd.init();
  Serial.println(F("init phase 2"));
  lcd.backlight();
  delay(300);
  lcd.setCursor(0, 0);
  lcd.print(F("     *********"));
  lcd.setCursor(0, 1);
  lcd.print(F("     * HELLO *"));
  lcd.setCursor(0, 2);
  lcd.print(F("     *********"));
  
  Serial.println(F("init phase 3"));

  for (int i = 0; i < 8; i++) {
    pinMode(LedPins[i], OUTPUT);
  }

 
  for (int i = 0; i < 7; i++) {
    digitalWrite(LedPins[i], HIGH);
    delay(200);
    digitalWrite(LedPins[i], LOW);
  }
  
  Serial.println(F("init phase 4"));
  

  
  
  Serial.println(F("init phase 5"));
  randomSeed(analogRead(A3));
  
  
  if(robot_actif==1){
    //retour robot position initiale
    Serial.println(F("init phase 6"));
    robot_position_initiale();
  }
    
  Serial.println(F("fin init"));
  Serial.println("");
     
}



void loop(){ 
      nouvelle_partie();   
      while(1){        
          if (joueur==1){
               colonne=0;
               while(verif_coup_possible(colonne)){ 
                  colonne=quelle_colonne(); 
               }
               joue_pion(colonne);
               if(robot_actif==1){
                 robot_prendre_jeton(joueur);
                 robot_depose_jeton_colonne(colonne);
               }
                
               aff_plateau();
               if(verif_partie_gagnee()) { 
                   break; 
               }         
          }else{
              if (type_partie==1){ //humain vs humain
                   colonne=0;
                   while(verif_coup_possible(colonne)){ 
                      colonne=quelle_colonne(); 
                   }
                   joue_pion(colonne);
                   if(robot_actif==1){
                     robot_prendre_jeton(joueur);
                     robot_depose_jeton_colonne(colonne); 
                   }
                   aff_plateau();
                   if(verif_partie_gagnee()) { 
                       break; 
                   }
               }else{ //humain vs IA
                   digitalWrite(LedPins[colonne-1], LOW);
                   colonne=col_joueur_ia();
                   bouton=colonne;
                   joue_pion(colonne);
                   lcd_led_ia();
                   if(robot_actif==1){
                     robot_prendre_jeton(joueur);
                     robot_depose_jeton_colonne(colonne); 
                   }
                   aff_plateau();
                   if(verif_partie_gagnee()) { 
                       break; 
                   }          
              } 
           }           
           //changement joueur
           if(joueur==1){
               joueur=2;
           }else{
               joueur=1;
           }      
      }      
      partie_gagnee();
}





//*********************************************************************
//fonctions annexes
//*********************************************************************



void  joue_pion(int colonne_jouee){ 
      compteur_coups++;
      for (int i=0; i < 6; i++){
          if(plateau[colonne_jouee-1][i]==0){
            plateau[colonne_jouee-1][i]=joueur;
            hauteur=i;
            break;
          }
       }
}

void  aff_plateau(){ 
      Serial.println("");
      for (int i=5; i >= 0; i--){
          for (int j=0; j < 7; j++){
              Serial.print(plateau[j][i]);
              Serial.print(F(" "));
          }
          Serial.println("");
      }
      Serial.println("");
}



boolean verif_coup_possible(int colonne_souhaitee){
     if(colonne_souhaitee==0) {
         return 1;
     }    
     if(plateau[colonne_souhaitee-1][5]!=0) {
         return 1;
     }else{
         return 0;
     }     
}

boolean verif_coup_possible_ia(int colonne_souhaitee){
     if(colonne_souhaitee==0) {
         return 1;
     }    
     if(plateau_ia[colonne_souhaitee-1][5]!=0) {
         return 1;
     }else{
         return 0;
     }     
}


void restaur_plateau_ia(){
    for (int i=0; i < 7; i++){ 
      for (int j=0; j < 6; j++){ 
        plateau_ia[i][j]=plateau_sauv[i][j];
      }    
    } 
}

void set_plateau_ia(){
    for (int i=0; i < 7; i++){ 
      for (int j=0; j < 6; j++){ 
        plateau_ia[i][j]=plateau[i][j];
      }    
    } 
}

void sauv_plateau_ia(){
    for (int i=0; i < 7; i++){ 
      for (int j=0; j < 6; j++){ 
        plateau_sauv[i][j]=plateau_ia[i][j];
      }    
    } 
}

void  aff_plateau_ia(){ 
      Serial.println("");
      for (int i=5; i >= 0; i--){
          for (int j=0; j < 7; j++){
              Serial.print(plateau_ia[j][i]);
              Serial.print(F(" "));
          }
          Serial.println("");
      }
      Serial.println("");
}

void joue_pion_ia(int colonne_jouee,int joueur_simu){ 
      for (int i=0; i < 6; i++){
          if(plateau_ia[colonne_jouee][i]==0){
            plateau_ia[colonne_jouee][i]=joueur_simu;
            hauteur_ia=i;
            break;
          }
       }
}

void lcd_led_ia(){ 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("  ***   Robot  ***"));
    lcd.setCursor(0, 2);
    lcd.print(F("      Colonne "));
    lcd.print(colonne);
    digitalWrite(LedPins[colonne-1], HIGH);
}

int evalue_score(int colonne_jouee,int niveau,int adv){ 
    
    int matrice_score[2][5]={ {0,100,200,600,900},{0,0,0,-500,-800} };
    int compteur;
    int compteur_final=0;
    int niv_joueur[2]={2,1};
    int joueur_ia=niv_joueur[niveau-1];
    
     
     //Verif en ligne
     //à gauche
     compteur=0;
     for (int i=1; i < 4; i++){
         if( (colonne_jouee-1-i)>=0 && plateau_ia[colonne_jouee-i][hauteur_ia]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     //à droite
     for (int i=1; i < 4; i++){
         if( (colonne_jouee-1+i)<7 && plateau_ia[colonne_jouee+i][hauteur_ia]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     if(compteur>compteur_final){
       compteur_final=compteur;
     }
     //Verif en colonne
     compteur=0;
     //en haut
     for (int i=1; i < 4; i++){
       if( (hauteur_ia+i)<6 && plateau_ia[colonne_jouee][hauteur_ia+i]==joueur_ia ){
           compteur++;
       }else{
           break;
       }     
     }
     //en bas
     for (int i=1; i < 4; i++){
       if( (hauteur_ia-i)>=0 && plateau_ia[colonne_jouee][hauteur_ia-i]==joueur_ia ){
           compteur++;
       }else{
           break;
       }     
     }
     if(compteur>compteur_final){
       compteur_final=compteur;
     }
     //Verif diag type slash
     compteur=0;
     //haut droite
     for (int i=1; i < 4; i++){
         if( (hauteur_ia+i)<6 && (colonne_jouee+i)<7 && plateau_ia[colonne_jouee+i][hauteur_ia+i]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     //bas gauche
     for (int i=1; i < 4; i++){
         if( (hauteur_ia-i)>=0 && (colonne_jouee-i)>=0 && plateau_ia[colonne_jouee-i][hauteur_ia-i]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     if(compteur>compteur_final){
       compteur_final=compteur;
     }
     //Verif diag type antislash
     compteur=0;
     //haut gauche
     for (int i=1; i < 4; i++){
         if( (hauteur_ia+i)<6 && (colonne_jouee-i)>=0 && plateau_ia[colonne_jouee-i][hauteur_ia+i]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     //bas droite
     for (int i=1; i < 4; i++){
         if( (hauteur_ia-i)>=0 && (colonne_jouee+i)<7 && plateau_ia[colonne_jouee+i][hauteur_ia-i]==joueur_ia ){
             compteur++;
         }else{
             break;
         }     
     }
     if(compteur>compteur_final){
       compteur_final=compteur;
     }
     
     int valeur_retour=matrice_score[niveau-1][compteur_final+1];

    
     if(niveau==2 && adv!=colonne_jouee && compteur_final>2){
        valeur_retour=11111;
        //Serial.print("achtung!!!"); 
        //aff_plateau_ia();
     }
     
     //Serial.print(colonne_jouee);
     //Serial.print(" : ");
     //Serial.print(hauteur_ia);
     //Serial.print(" : ");
     //Serial.print(compteur_final);
     //Serial.print(" : ");
     //Serial.print(valeur_retour);
     //Serial.print(" : ");
     
    
     return valeur_retour;
}


void stock_score(int colonne_jouee,int score){ 
 
    //if(score>tab_score[colonne_jouee]){
    //     tab_score[colonne_jouee]=score;
    //}
    tab_score[colonne_jouee]=tab_score[colonne_jouee]+score;
    //Serial.println(tab_score[colonne_jouee]); 
}


int analyse_ia(){ 
  int col_max=0;
  int max_ana=0;

  for (int i=0; i < 7; i++){
   if(tab_score[i]>max_ana){
       max_ana=tab_score[i];
       col_max=i;
   } 
  }
  return col_max;
}

int col_joueur_ia(){  
    int score=0; 
    int col_ia=0;
    int col_humain=0;
    for (int i=0; i < 7; i++){
          tab_score[i]=0;   
    }
    //le 1er coup est joué en random entre les colonnes 3 et 5 en excluant la col jouée par l'adversaire
    if(compteur_coups==1){
        for (int i=0; i < 7; i++){
          if (plateau[i][0]==1){
            col_humain=i;
            break;
          }
        }
        Serial.println(col_humain);
        if (col_humain<2 || col_humain>4){
          col_ia=random(3,5) ;
        } else {
           col_ia= col_humain-1;
        }
        return col_ia+1;
    } 
    set_plateau_ia();
    for (int i=0; i < 7; i++){
        score=0; 
        set_plateau_ia();
        joue_pion_ia(i,2);
        score=evalue_score(i,1,50);
        if (score==900){
           return i+1;
           break ;
        }
        stock_score(i,score);
        sauv_plateau_ia();
        for (int j=0; j < 7; j++){
            score=0;
            restaur_plateau_ia();
            joue_pion_ia(j,1);
            score=evalue_score(j,2,i);
            if (score==11111){
               return j+1;
               break ;
            }
            stock_score(j,score);    
            
        }   
    }
    col_ia=analyse_ia(); 

    return col_ia+1;
}



boolean verif_partie_gagnee(){ 
     //Verif en ligne
     int compteur=0;
     //à gauche
     for (int i=1; i < 4; i++){
         if( (colonne-1-i)>=0 && plateau[colonne-1-i][hauteur]==joueur ){
             compteur++;
         }else{
             
             break;
         }     
     }
     //à droite
     for (int i=1; i < 4; i++){
         if( (colonne-1+i)<7 && plateau[colonne-1+i][hauteur]==joueur ){
             compteur++;
         }else{
             //Serial.println("en ligne: ");
             //Serial.println(compteur);
             break;
         }     
     }
     if(compteur>=3) {
        Serial.println(F("partie gagnee en ligne"));
        return 1;
     }
     
     
     //Verif en colonne
     compteur=0;
     //en haut
     for (int i=1; i < 4; i++){
         if( (hauteur+i)<6 && plateau[colonne-1][hauteur+i]==joueur ){
             compteur++;
         }else{
             break;
         }     
     }
     //en bas
     for (int i=1; i < 4; i++){
         if( (hauteur-i)>=0 && plateau[colonne-1][hauteur-i]==joueur ){
             compteur++;
         }else{
             //Serial.println("en colonne: ");
             //Serial.println(compteur);
             break;
         }     
     }
     if(compteur>=3) {
        Serial.println(F("partie gagnee en colonne"));
        return 1;
     }
     
     
     //Verif diag type slash
     compteur=0;
     //haut droite
     for (int i=1; i < 4; i++){
         if( (hauteur+i)<6 && (colonne-1+i)<7 && plateau[colonne-1+i][hauteur+i]==joueur ){
             compteur++;
         }else{
             break;
         }     
     }
     //bas gauche
     for (int i=1; i < 4; i++){
         if( (hauteur-i)>=0 && (colonne-1-i)>=0 && plateau[colonne-1-i][hauteur-i]==joueur ){
             compteur++;
         }else{
             //Serial.println("en slash: ");
             //Serial.println(compteur);
             break;
         }     
     }
     if(compteur>=3) {
        Serial.println(F("partie gagnee en slash"));
        return 1;
     }
     
     
     //Verif diag type antislash
     compteur=0;
     //haut gauche
     for (int i=1; i < 4; i++){
         if( (hauteur+i)<6 && (colonne-1-i)>=0 && plateau[colonne-1-i][hauteur+i]==joueur ){
             compteur++;
         }else{
             break;
         }     
     }
     //bas droite
     for (int i=1; i < 4; i++){
         if( (hauteur-i)>=0 && (colonne-1+i)<7 && plateau[colonne-1+i][hauteur-i]==joueur ){
             compteur++;
         }else{
             //Serial.println("en antislash: ");
             //Serial.println(compteur);
             break;
         }     
     }
     if(compteur>=3) {
        Serial.println(F("partie gagnee en antislash"));
        return 1;
     }
    
     Serial.println("");
     return 0;
}



int quelle_colonne(){ 
      digitalWrite(LedPins[bouton-1], LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("  *** Joueur "));
      lcd.print(joueur);
      lcd.print(F(" ***"));
      lcd.setCursor(0, 2);
      lcd.print(F("   Choix colonne ?  "));
      
      Serial.print(F("*** Joueur "));
      Serial.print(joueur);
      Serial.println(F(" ***"));
      Serial.println(F("Choix colonne ?"));

      while(scan_boutons()){  
      }
      digitalWrite(LedPins[bouton-1], HIGH);

      lcd.setCursor(0, 2);
      lcd.print(F("      Colonne "));
      lcd.print(bouton);
      lcd.print(F("    "));
      
      
      return bouton;
}






void nouvelle_partie(){
      
        
      //ttes les LED en OFF
      for (int i = 0; i < 7; i++) {
        digitalWrite(LedPins[i], LOW);
      }
      //aff sur lcd nouvelle partie
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("nouvelle partie ?"));
      lcd.setCursor(0, 2);
      lcd.print(F("1 - humain vs ordi"));
      lcd.setCursor(0, 3);
      lcd.print(F("2 - humain vs humain"));
      
      Serial.println(F("nouvelle partie ?"));
      Serial.println(F("1 - humain vs ordi"));
      Serial.println(F("2 - humain vs humain"));
      
 
      while(scan_boutons() || bouton==0 || bouton>2){  
      }
      digitalWrite(LedPins[bouton-1], HIGH);

      lcd.clear();
      lcd.setCursor(0, 1);
  
      if (bouton==1)   {
         type_partie=0;     
         Serial.println(F("humain vs ordi")); 
         lcd.print(F("   humain vs ordi"));  
      } else { 
         type_partie=1;
         Serial.println(F("humain vs humain"));
         lcd.print(F("  humain vs humain"));
      }
      
      delay(500);
    
      
      //raz du plateau de jeu
      for (int i=0; i<7; i++){
         for (int j=0; j<6; j++){
           plateau[i][j]=0;
         } 
      } 
     //joueur blanc en premier    
     joueur=1;
     
     //raz nbr de coups joués
     compteur_coups=0;
   
    delay(1000);  
}

void anim_lcd(){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(F("********************"));
      lcd.setCursor(0, 1);
      lcd.print("*     Joueur ");lcd.print(joueur);lcd.print("     *");
      lcd.setCursor(0, 2);
      lcd.print(F("*    Victoire !!   *"));
      lcd.setCursor(0, 3);
      lcd.print(F("********************"));
}

void anim_led(){
  
      for (int i = 0; i < 7; i++) {
        digitalWrite(LedPins[i], LOW);
      }
      
      for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 7; i++) {
          digitalWrite(LedPins[i], HIGH);
          delay(100);
          if (i!=6){
            digitalWrite(LedPins[i], LOW);
          }
        }
        for (int i = 7; i >= 0; i--) {
          if (i!=6){
            digitalWrite(LedPins[i], HIGH);
          }
          delay(100);
          digitalWrite(LedPins[i], LOW);
        }
      }
      
      for (int h = 0; h < 4; h++) {
        for (int i = 0; i < 7; i++) {
            digitalWrite(LedPins[i], HIGH);
        }
        delay(150);
        for (int i = 0; i < 7; i++) {
            digitalWrite(LedPins[i], LOW);
        }
        delay(200);
      }
}




void partie_gagnee(){ 
    //anim lcd
    anim_lcd();
    //anim partie gagnee du robot
    anim_led();
    int cpt=0;
    //X2
    if(robot_actif==1){
        while (cpt<2){ 
            mySerial.print(F("#0 P1140  S800 #1 P1840 S800 #2 P2250 S800 #3 P940 S800 #4  P900 S800 #5 P1000 S800\r"));
            robot_attente_fin_mouvement();
            lcd.clear();
            mySerial.print(F("#0 P2060  S800 #1 P1840 S700 #2 P2250 S800 #3 P940 S800 #4  P1900 S800 #5 P2000 S800\r"));
            anim_lcd();
            robot_attente_fin_mouvement();
            
            cpt++;
        }
        robot_position_initiale();
        mySerial.print(F("#5 P1520  S600\r"));
        robot_attente_fin_mouvement();
     }
    lcd.clear(); 
    lcd.setCursor(0, 2);
    lcd.print(F("Presser le bouton 7"));
    Serial.print(F("Presser le bouton 7"));
    bouton=0;     
    while(scan_boutons() || bouton!=7){  
    }
    digitalWrite(LedPins[bouton-1], HIGH);
    delay(600);
 
}



int scan_boutons(){ 
   //raz du bouton
   bouton=0;
   //on prend deux mesures à 100ms d'ecard
   int val = analogRead(A4);
   delay (100);
   int val2 = analogRead(A4);
   //on calcul le rapport entre les deux mesures
   float ratio= val/val2;
   //tolerance acceptable -> debounce -> 10%
   float tolerance=0.05;
   
   Serial.println(val);
   
   //si le ratio est compris dans la valeur de tolérance
   // on considere la prise de valeur comme stable et valable
   if ( (1-tolerance)<ratio && ratio< (1+tolerance) &&  val>70 && val<950) {        
       if( val>90 && val<140){bouton=1;}
       if(val>200 && val<250){bouton=2;}
       if(val>310 && val<350){bouton=3;}
       if(val>410 && val<470){bouton=4;}
       if(val>520 && val<560){bouton=5;}
       if(val>630 && val<690){bouton=6;}
       if(val>790 && val<860){bouton=7;}
       Serial.print(F(" Bouton: "));
       Serial.println(bouton);
       Serial.println("");
       return 0;
   }
   
      
   //lecture surle port serie
   if(Serial.available()) {
        int ch = Serial.read();
         if(isDigit(ch) ) {
            bouton=(ch - '0');
            Serial.println("");
            Serial.print(F("Bouton: "));
            Serial.println(bouton);
            Serial.println("");
            return 0;
         }
   }
   

  //si aucune entrée serie ou bouton on retourne 1
  return 1;
}


void robot_prendre_jeton(int couleur){
  if (couleur==1) {
      mySerial.print(F("#0 P840 S600 #1 P1730 S400 #2 P2350 S400 #3 P990 S400 #4  P1380 S400 #5 P1030 S200\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1500 S400 #2 P2264 S400 #3 P1230 S400\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#5 P1520  S600\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1730 S500 #2 P2350 S500 #3 P990 S500\r"));
      robot_attente_fin_mouvement();      
        
           
  }
  if (couleur==2) {
      mySerial.print(F("#0 P2320 S600 #1 P1730 S400 #2 P2350 S400 #3 P990 S400 #4  P1380 S400 #5 P1030 S200\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1500S400 #2 P2260 S400 #3 P1230 S400\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#5 P1520  S600\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1730 S500 #2 P2350 S500 #3 P990 S500\r"));
      robot_attente_fin_mouvement();          
  }
  
  //retour position initiale
  robot_position_initiale();
}

void robot_depose_jeton_colonne(int colonne_pion){
  
  //deplacement au dessus de la bonne colonne
  switch (colonne_pion) {
    case 1:
      mySerial.print(F("#0 P1100 S500 #3 P760 S500 #4  P1770 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1040 S400 #2 P1950 S400\r"));
      robot_attente_fin_mouvement(); 
    break;
    
    case 2:
      mySerial.print(F("#0 P1220 S500 #3 P800 S500 #4  P1670 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1130 S400 #2 P2140 S400\r"));
      robot_attente_fin_mouvement();
    break;
    
    case 3:
      mySerial.print(F("#0 P1370 S500  #3 P800 S500 #4  P1550 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1180 S400 #2 P2240 S400\r"));
      robot_attente_fin_mouvement(); 
    break;
    
    case 4:
      mySerial.print(F("#0 P1580 S500  #3 P800 S500 #4  P1385 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1220 S400 #2 P2270 S400\r"));
      robot_attente_fin_mouvement(); 
    break;
    
    case 5:
      mySerial.print(F("#0 P1750 S500  #3 P770 S500 #4  P1230 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1170 S400 #2 P2200 S400\r"));
      robot_attente_fin_mouvement(); 
    break;
    
    case 6:
      mySerial.print(F("#0 P1890 S500  #3 P730 S500 #4  P1100 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P1100 S400 #2 P2060 S400\r"));
      robot_attente_fin_mouvement();
    break;
    
    case 7:
      mySerial.print(F("#0 P2020 S500 #3 P675 S500 #4  P960 S500\r"));
      robot_attente_fin_mouvement();
      mySerial.print(F("#1 P950 S400 #2 P1820 S400\r"));
      robot_attente_fin_mouvement();
     break;

  }

  //ouverture pince
  mySerial.print(F("#5 P1100 S500\r"));
  robot_attente_fin_mouvement();
  
  //retour position initiale
  robot_position_initiale();
  
  //extinction de la LED de la colonne jouee
  if (bouton!=0){
    digitalWrite(LedPins[bouton-1], LOW);
  }
  
}



void robot_position_initiale(){ 
      Serial.println(F("debut init robot")); 
      mySerial.print(F("#0 P1540  S500 #1 P1840 S500 #2 P2310 S500 #3 P800 S500 #4  P1370 S500\r"));     
      Serial.println(F("fin init robot")); 
      robot_attente_fin_mouvement(); 
}


void robot_attente_fin_mouvement(){
   char valeur='+';   
   //interrogation de la carte controleur du robot -> envoi du caractere 'Q' (query)
   //si le caractere '+' est retourné, le deplacement est encore en cours, sinon le cartactere '.' est renvoyé
   
   while (valeur=='+') {
       mySerial.print("Q\r");
       delay(100);
       if (mySerial.available()){
          valeur=mySerial.read();
          lcd.setCursor(19,3);
          lcd.print(valeur);
       }
       delay(100);
   }
   Serial.println(F("fin mouvement robot robot"));

}
