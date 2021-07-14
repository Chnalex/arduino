
///////////////////////////////
////  Alexandre OGER 2021  ////
////     sharp ce-12X      ////
///////////////////////////////

//imprimante pour pocket-pc sharp
// l'arduino est connecte au port 11pin sharp via des resitance de tirage à la masse
// en sortie sur port serie soft ets connecte une imprimante thermique compatible avec la bibliotheque adafruit 
  
/*
                                                    
              +----[PWR]-------------------| USB |--+
              |                            +-----+  |
              |         GND/RST2  [ ][ ]            |
              |       MOSI2/SCK2  [ ][ ]  A5/SCL[ ] |    
              |          5V/MISO2 [ ][ ]  A4/SDA[ ] |    
              |                             AREF[ ] |
              |                              GND[ ] |
              | [ ]N/C                    SCK/13[ ] |   
              | [ ]IOREF                 MISO/12[ ] |   
              | [ ]RST                   MOSI/11[ ]~|________ port serie soft RX
              | [ ]3V3    +---+               10[ ]~|________ port serie soft TX
              | [ ]5v    -| A |-               9[ ]~|________ Busy - sharp 11pins  -> 10kohm pulldown ->gnd    
              | [ ]GND   -| R |-               8[ ] |________ Dout - sharp 11pins  -> 10kohm pulldown ->gnd   
              | [ ]GND   -| D |-                    |
              | [ ]Vin   -| U |-               7[ ] |   
              |          -| I |-               6[ ]~|________ Xout - sharp 11pins  -> 10kohm pulldown ->gnd 
              | [ ]A0    -| N |-               5[ ]~|
              | [ ]A1    -| O |-               4[ ] |________ ACK  - sharp 11pins  -> 10kohm pulldown ->gnd  
              | [ ]A2     +---+           INT1/3[ ]~|________ SEL2 - sharp 11pins  -> 10kohm pulldown ->gnd 
              | [ ]A3                     INT0/2[ ] |________ SEL1 - sharp 11pins  -> 10kohm pulldown ->gnd   
              | [ ]A4/SDA  RST SCK MISO     TX>1[ ] |    
              | [ ]A5/SCL  [ ] [ ] [ ]      RX<0[ ] |    
              |            [ ] [ ] [ ]              |
              | UNO WIFI2  GND MOSI 5V  ____________/
               \_______________________/
                         
 
 */



// sur base du code de : Walter Fischer, 3270 Scheibbs deutshland-Allemagne
// merci a lui pour l'analyse du comportement des pins du connecteur 11-pins sharp
// son site : http://www.cavefischer.at/spc/html/CE-126P_Emulator.html


const int IN_SEL1 = 2; // Pin D2, SEL1, pin 11 -> connecteur 11 broches sharp
const int IN_SEL2 = 3; // Pin D3, SEL2, pin 10 -> connecteur 11 broches sharp 
const int OUT_ACK = 4; // Pin D4, ACK, pin 9  -> connecteur 11 broches sharp
const int IN_Xout = 6; // Pin D6, Xout, pin 7  -> connecteur 11 broches sharp
const int IN_Dout = 8; // Pin D8, Dout, pin 5  -> connecteur 11 broches sharp
const int IN_Busy = 9; // Pin D9, Busy, pin 4  -> connecteur 11 broches sharp
const int InfoLED = 13;
boolean Busy;
boolean Xout;
boolean SEL1;
boolean SEL2;
int DataBit;  
int DataByte;
long Timeout;
int i;
int ok1250=0;

int debut_ligne=1;

//declaration de l'imprimante thermique sur port serie soft
#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#define TX_PIN 10 // port serie soft TX
#define RX_PIN 11 // port serie soft RX
SoftwareSerial mySerial(RX_PIN, TX_PIN); 
Adafruit_Thermal printer(&mySerial);  


void setup() {
Serial.begin(115200);
pinMode(OUT_ACK, OUTPUT);
pinMode(IN_Xout, INPUT);
pinMode(IN_Dout, INPUT);
pinMode(IN_Busy, INPUT);
pinMode(InfoLED, OUTPUT);
pinMode(IN_SEL1, INPUT);
pinMode(IN_SEL2, INPUT);


mySerial.begin(9600);  //demmarrage port serie soft
printer.begin();    // demarrage de l'imprimante sur le port serie soft
printer.setHeatConfig(30, 250,100);
printer.setFont('A');
printer.println();
printer.println(F("--------------------------------"));

printer.doubleHeightOn();
//printer.println(F("Hello ;-)"));
printer.println(F("Emulation imp. Pocket Sharp"));
printer.doubleHeightOff();
printer.println("By Alex (c) 2021");
printer.println();
printer.doubleHeightOn();
printer.println(F("Imprimante ok !"));
printer.doubleHeightOff();
printer.println(F("--------------------------------"));
printer.println();
printer.println();
printer.println();

}

void loop() {

digitalWrite(InfoLED, LOW);
SEL1 = digitalRead(IN_SEL1);
SEL2 = digitalRead(IN_SEL2);
Xout = digitalRead(IN_Xout);
Busy = digitalRead(IN_Busy);


//verification si le pocket-pc est pret à imprimer selon le modele 

//detection pour les pc-1260 ou equivalent
if (SEL2 && SEL1) {
    ok1260=1;
    digitalWrite(InfoLED, HIGH);
    delayMicroseconds(50);
    i = 0;
    do {
      SEL1 = digitalRead(IN_SEL1);
    } while (SEL1);
    Timeout = millis();
    do {
      Busy = digitalRead(IN_Busy);
      if (millis() - Timeout > 50) break;
    } while (!Busy);
    delayMicroseconds(50);
    digitalWrite(OUT_ACK, HIGH);
    Timeout = millis();
    do {
      SEL1 = digitalRead(IN_SEL1);
      if (millis() - Timeout > 50) break;
    } while (!SEL1);
    delayMicroseconds(150);
    digitalWrite(OUT_ACK, LOW);
    do {
      SEL1 = digitalRead(IN_SEL1);
    } while (SEL1);
    delayMicroseconds(150);
}


// detection pour les PC-1401 pc-1403 pc-1350 pc-500e 
if (Xout) {
    digitalWrite(InfoLED, HIGH);  
    delayMicroseconds(50);
    i = 0;
    do {
      digitalWrite(OUT_ACK, HIGH);
      Timeout = millis();
      do {
        Busy = digitalRead(IN_Busy);
        if (millis() - Timeout > 50) break;
      } while (!Busy);
      delayMicroseconds(50);
      digitalWrite(OUT_ACK, LOW);
      do {
         Busy = digitalRead(IN_Busy);
      } while (Busy); 
      delayMicroseconds(50);
      i++;
    } while (i < 8);

}





//detection et impression pour les pc-1250 ou equivalent
Busy = digitalRead(IN_Busy);
if (SEL2 && Busy) { 
      // Daten:
      digitalWrite(InfoLED, HIGH);
        //Serial.println("mode :1250");
      i = 0;
      DataByte = 0;
      do {
        Timeout = millis();
        do {
          Busy = digitalRead(IN_Busy);
          if (millis() - Timeout > 50) break;
        } while (!Busy);
        delayMicroseconds(500);
        DataBit = digitalRead(IN_Dout);
        digitalWrite(OUT_ACK, HIGH);
        do {
          Busy = digitalRead(IN_Busy);
        } while (Busy);
        delayMicroseconds(480);
        digitalWrite(OUT_ACK, LOW);
        DataByte = DataByte | (DataBit << i);
        i++;
      } while (i < 8);
      // Ausgabe:
      if (debut_ligne==1){
        debut_ligne=0;
        printer.print("   ");
      }
      switch (DataByte) {
        case 13:
          Serial.println();
          printer.println("");
          printer.print("   ");
         break;
        default:
          if (DataByte > 31 && DataByte < 127) {
             Serial.print(char(DataByte));                        
             printer.print(char(DataByte));
        }
      }
}


// impression pour les pc-1260 ou equivalent
if (Busy && ok1260) { 
    // Daten:
    digitalWrite(InfoLED, HIGH);
    i = 0;
    DataByte = 0;
    do {
      do {
          Busy = digitalRead(IN_Busy);
      } while (!Busy);
      delayMicroseconds(500);
      DataBit = digitalRead(IN_Dout);
      digitalWrite(OUT_ACK, HIGH);
      do {
        Busy = digitalRead(IN_Busy);
      } while (Busy);
      delayMicroseconds(480);
      digitalWrite(OUT_ACK, LOW);
      DataByte = DataByte | (DataBit << i);
      i++;
    } while (i < 8);
    // Ausgabe:
    if (debut_ligne==1){
      debut_ligne=0;
      printer.print("   ");
    }
    switch (DataByte) {
      case 13:
        Serial.println();
        printer.println("");
        printer.print("   ");
        break;
    default:
        if (DataByte > 31 && DataByte < 127) {
          Serial.print(char(DataByte));                        
          printer.print(char(DataByte));
        }
    }
}

// impression pour les PC-1401 pc-1403 pc-1350 pc-500e 
if (Busy && !Xout) { 
    // Daten:
    digitalWrite(InfoLED, HIGH);
    //Serial.println("data");
    i = 0;
    DataByte = 0;
    do {
      do {
        Busy = digitalRead(IN_Busy);
      } while (!Busy);
      delayMicroseconds(30);
      DataBit = digitalRead(IN_Dout);
      digitalWrite(OUT_ACK, HIGH);
      do {
        Busy = digitalRead(IN_Busy);
      } while (Busy);
      delayMicroseconds(30);
      digitalWrite(OUT_ACK, LOW);
      DataByte = DataByte | (DataBit << i);
      i++;
    } while (i < 8);
    if (debut_ligne==1){
      debut_ligne=0;
      printer.print("   ");
    }
    switch (DataByte) {
    case 13:
       Serial.println();
       printer.println("");
       printer.print("   ");
    break;
    case 48:
        Serial.print("O"); 
        printer.print("O");
    break;
    case 240:
         Serial.print("0"); 
         printer.print("0"); 
    break;
    default:
        if (DataByte > 31 && DataByte < 127) {
          Serial.print(char(DataByte));                        
          printer.print(char(DataByte));
        }
    }
}

} 
