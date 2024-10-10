/*
///////////////////////////////
////  Alexandre OGER 2024  ////
///////////////////////////////

gadget indiquant le nombre de jours restants jusqu'au départ de mon camarade Guillaume
montage simple autour d'un arduino pro micro, d'un afficheur LCD I2C 16X2 car., et d'une horloge RTC DS3231

au besoin, il est possible via le port serie (USB), de regler les date et heure courantes, de regler l'heure de mise en route 
du retroéclairage ainsi le jour/mois/année du départ en retraite

                                                            +-----+
                                               +------------| USB |------------+
                                               | O O J1     +-----+            |
                                          TX 1 | [ ]TX0                 RAW[ ] |
                                          RX 0 | [ ]RXI  *PWRLED        GND[ ] |
                                               | [ ]GND       /\        RST[ ] |
                                               | [ ]GND      /AT\       VCC[ ] | (5V)
  LCD I2C + RTC (DS3231)-----------------SDA 2 | [ ]2       /MEGA\       A3[ ] | 21 A3
  LCD I2C + RTC (DS3231)-----------------SCL 3 | [ ]3       \32U4/       A2[ ] | 20 A2
                                          A6 4 | [ ]4        \  /        A1[ ] | 19 A1
                                             5 | [ ]5         \/         A0[ ] | 18 A0
                                          A7 6 | [ ]6                    15[ ] | 15 SCLK
                                             7 | [ ]7    *RXLED TXLED*   14[ ] | 14 MISO
                                          A8 8 | [ ]8                    16[ ] | 16 MOSI
                                          A9 9 | [ ]9      PRO MICRO     10[ ] | 10 A10  
                                               +-------------------------------+




*/


  #include <Wire.h>
  #include <DS1307RTC.h>
  #include <LiquidCrystal_I2C_ALEX.h>
  #include <Time.h>
  #include <EEPROM.h>


  time_t maintenant=0;  
  long resultat;
  tmElements_t jour_retraite; 
  int jours;
  unsigned long millis_update_affichage=0;
  char buffer_serie[32];

  LiquidCrystal_I2C_ALEX lcd(0x27,16,2); 

void setup()   {
  Wire.begin();
  Wire.setWireTimeout(25000, true);
  Serial.begin(115200);

  delay(3000);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Hello, world!");
  lcd.setCursor(0,1);
  lcd.print("      Nalex 2024");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  setSyncProvider(RTC.get);
    if(timeStatus() != timeSet){
        Serial.println("sync RTC KO :-( ");
        lcd.print("sync RTC KO :-(");
     }else{
        Serial.println("sync RTC OK !");
        lcd.print("Sync. Horloge");
        lcd.setCursor(6,1);
        lcd.print("OK !!");
     }
  delay(1000);
  lcd.clear();  delay(1000);
  lcd.clear();

  Serial.print("Retroeclairage: de ");
  Serial.print(EEPROM.read(1));
  Serial.print("h à ");
  Serial.print(EEPROM.read(2));
  Serial.println("h");

  Serial.print("Jour du départ en retraite: ");
  Serial.print(EEPROM.read(5));
  Serial.print("/");
  Serial.print(EEPROM.read(6));
  Serial.print("/");
  Serial.println(EEPROM.read(7));

  jour_retraite.Second = 0;
  jour_retraite.Hour = 0; 
  jour_retraite.Minute = 0;
  jour_retraite.Day = EEPROM.read(5);
  jour_retraite.Month = EEPROM.read(6) - 1;      
  jour_retraite.Year = (EEPROM.read(7)+2000) - 1970; 



} 


void loop(){

  ecoute_serie();


  if (Wire.getWireTimeoutFlag())
	{
		Wire.clearWireTimeoutFlag();
    Serial.println("Defaut communication I2C");
	}
  //lcd.backlight(0);

  if (millis()-millis_update_affichage> 10000 ) {
    millis_update_affichage=millis();

    if( hour()>=EEPROM.read(1) && hour()<=EEPROM.read(2)){
      lcd.backlight();
    }else{
      lcd.noBacklight();
    }    

    maintenant = now();
    resultat = maintenant - makeTime(jour_retraite);
    //Serial.print("compteur secondes: ");
    //Serial.println(resultat);
    jours= int((((resultat/60)/60)/24));
    Serial.print("compteur jours: ");
    Serial.println(jours);
    lcd.clear();
    lcd.setCursor(0,0);
    
    if(jours>0){
      lcd.print("depuis:");
    }else{
      lcd.print("dans:");
    }
    //Serial.print("Centrage chaine (nbr car): ");
    //Serial.println( (16-(longueur_entier(abs(jours))+6))/2 );
    lcd.setCursor(int((16-(longueur_entier(abs(jours))+6))/2),1);
    lcd.print(jours);    
    lcd.print(" jours"); 
    Serial.print("Date/heure courante: ");  
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
    Serial.println(F("******************************"));
    Serial.println(F("Reglage Horloge"));    
    Serial.println(F("pour mettre a jour l'heure"));
    Serial.println(F("taper une chaine comme suit:"));
    Serial.println(F("HHMMSSJJMMAAAA")); 
    Serial.println(""); 
    Serial.println(F("Reglage heure debut / fin du retreclairage"));    
    Serial.println(F("debuter la chaine de caractères par un E majuscule"));
    Serial.println(F("la chaine doit être comme suit (E suivi de l'heure debut sur 2 digits puis de l'heure fin sur 2 digits):"));
    Serial.println(F("EHHHH")); 
    Serial.println(""); 
    Serial.println(F("Reglage jour/mois/année du départ en retraite"));    
    Serial.println(F("debuter la chaine de caractères par un R majuscule"));
    Serial.println(F("la chaine doit être comme suit :"));
    Serial.println(F("RJJMMAAAA"));     
    Serial.println(F("******************************"));
    Serial.println(""); 
    Serial.println(""); 
    Serial.println(""); 
  }  

}

int longueur_entier(int num){
  uint8_t count = 0;
  while(num)
  {
    num = num / 10;
    count++;
  }
  return count;
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
   Serial.print(inByte);
    buffer_serie[cpt]=inByte;
    cpt++;
    delay(5); 
  }
  
 if(cpt>0){
      Serial.println(buffer_serie);
    if(buffer_serie[0]=='E'){

      EEPROM.write(1,((buffer_serie[1] - '0')*10) + (buffer_serie[2]- '0')); //heure debut eclairage
      EEPROM.write(2,((buffer_serie[3] - '0')*10) + (buffer_serie[4]- '0')); //heure fin eclairage


      Serial.print(EEPROM.read(1));
      Serial.println(EEPROM.read(2));

    }

    else if(buffer_serie[0]=='R'){

      EEPROM.write(5,((buffer_serie[1] - '0')*10) + (buffer_serie[2]- '0')); //jour
      EEPROM.write(6,((buffer_serie[3] - '0')*10) + (buffer_serie[4]- '0')); //mois
      EEPROM.write(7,((buffer_serie[5] - '0')*1000) + ((buffer_serie[6] - '0')*100)+((buffer_serie[7] - '0')*10)+(buffer_serie[8]- '0')-2000); //année (4 digits)

      Serial.print(EEPROM.read(5));
      Serial.print(EEPROM.read(6));
      Serial.println(EEPROM.read(7));

    }

    else {
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
      lcd.clear();
      lcd.setCursor(0,0);
      setSyncProvider(RTC.get);
      if(timeStatus() != timeSet){
          Serial.println("sync RTC KO :-( ");
          lcd.print("sync RTC KO :-(");
      }else{
          Serial.println("sync RTC OK !");
          lcd.print("Sync. Horloge");
          lcd.setCursor(6,1);
          lcd.print("OK !!");
      }
      delay(2000);
      millis_update_affichage=millis()-10000;
    }
 }
}

void raz_buffer()  {  
  for(int i=0;i<32;i++){
    buffer_serie[i]='\0';
  } 
}
