#include <SoftwareSerial.h>

/****************/
/* DÉCLARATIONS */
/****************/

// Radio HC-12
SoftwareSerial HC12(8, 9); // HC-12 TX Pin, HC-12 RX Pin
#define ATpin A3 // used to switch HC-12 to AT mode
char acquis_data;
String chaine;
//https://plaisirarduino.fr/moniteur-serie/
int VitesseVent;   // variable des data reçue
int rpmEolienne;   // variable des data reçue
float VitesseVent_float;   // variable des data reçue
float rpmEolienne_float;   // variable des data reçue
//https://forum.arduino.cc/index.php?topic=553743.0

// Partie MONITORING
float Voltage=12;
float MiniC=0.08;           // courant minimum mesuré en dessous duquel la valeur est forcée à 0.
float tension_batterie_float;
float Courant_float;

//==== Afficheurs ====//
#define Data0 2  // Input LSB Afficheur DLG7137
#define Data1 3
#define Data2 4
#define Data3 5
#define Data4 6
#define Data5 7
#define Data6 10  // Input MSB Afficheur DLG7137
#define datapin 12   // pin  pour les données vers les registres à décallages
#define clockpin 11  // pin  pour l'horloge qui coordonne les registres à décallages
static const uint8_t latchpin1 = A0; // utilise la pin RX/TX donc sérial print impossible sans désassigner
static const uint8_t latchpin2 = A1; // ces pins ni les dessouder
//static const uint8_t latchpin1 = 25;
//static const uint8_t latchpin2 = 25;
int VitesseVentC = 0;
int VitesseVentD = 0;
int VitesseVentU = 0;
int VitesseEolienneC = 0;
int VitesseEolienneD = 0;
int VitesseEolienneU = 0;
byte dataArray[16];    // Tableau de données

/*********/
/* SETUP */
/*********/
void setup() {
  Serial.begin(9600);  // init du mode débug
  HC12.begin(9600);               // Serial port to HC12
//  HC12.begin(115200);               // Serial port to HC12
  pinMode(ATpin, OUTPUT);
  digitalWrite(ATpin, LOW); // Set HC-12 into AT Command mode
  delay(500);
  HC12.print("AT+C010");  // passer sur le canal 036 (433.4Mhz + 36x400KHz)
  delay(500);
  digitalWrite(ATpin, HIGH); // HC-12 en normal mode
  
//==== Afficheurs ====//
  pinMode(Data0, OUTPUT);
  pinMode(Data1, OUTPUT);
  pinMode(Data2, OUTPUT);
  pinMode(Data3, OUTPUT);
  pinMode(Data4, OUTPUT);
  pinMode(Data5, OUTPUT);
  pinMode(Data6, OUTPUT);
  pinMode(clockpin, OUTPUT);     // pin correspondant à "clockpin" initialisée en sortie
  pinMode(datapin, OUTPUT);      // pin correspondant à "datakpin" initialisée en sortie
  pinMode(latchpin1, OUTPUT);     // pin correspondant à "latchpin" initialisée en sortie
  pinMode(latchpin2, OUTPUT);     // pin correspondant à "latchpin" initialisée en sortie

// low byte - 8 bits premier registre
  dataArray[0] = B01111111;  // Case du tableau qui contient la valeur binaire pour permettre d'adresser l'afficheur unité
  dataArray[1] = B10111111;
  dataArray[2] = B11011111; 
  dataArray[3] = B11101111; 
  dataArray[4] = B11110111; 
  dataArray[5] = B11111011; 
  dataArray[6] = B11111101;
  dataArray[7] = B11111110;
// high byte - 8 bits second registre
  dataArray[8] = B01111111;
  dataArray[9] = B10111111;
 dataArray[10] = B11011111;  
 dataArray[11] = B11101111; 
 dataArray[12] = B11110111; 
 dataArray[13] = B11111011; 
 dataArray[14] = B11111101;
 dataArray[15] = B11111110;   
}

/*************/
/* PROGRAMME */
/*************/
void loop() {
  Reception();
  CalculReception();
  Afficheurs();
  delay(500);   // délai de rafraichissement des afficheurs

//=== pour tests ===//
//  VitesseVentC = 2;
//  VitesseVentD = 7;
//  VitesseVentU = 4;
//  VitesseEolienneC = 5;
//  VitesseEolienneD = 6;
//  VitesseEolienneU = 2;
// TestAfficheurs();
}

/*************/
/* FONCTIONS */
/*************/
void Reception() {
// surtout pas de delay dans cette boucle, sinon les data reçues sont erronnées.
  while (HC12.available()) {        // If HC-12 has data
    acquis_data = HC12.read();
    chaine = chaine + acquis_data;
//    Serial.println (chaine);      // Attention, chaine est donc une String

/* message reçu de la forme 
5 KMH / 60 RPM / 11.435 VOL / -11.371 AMP
pour chaque ligne on fait :*/
    if (acquis_data == 10) {      //détection de fin de ligne : méthodes ascii meilleure !
//    if (chaine.endsWith("\n")) {      //détection de fin de ligne : méthodes String
//      Serial.println ("fin de ligne");            // debug
//      String phrase = "12.665 VOL / 0.045 AMP";   //debug
      char Ventchar[5];
      char Eolchar[5];
      char tension_batterie[7];   // chaine de 6 caractères pour stocker le texte avant le mot VOL
      char Courant[3];            // chaine de 4 caractères pour stocker le texte avant le mot AMP
// scanf tout en char, puis conversion float :
//      sscanf(chaine.c_str(), "%s KMH / %s RPM / %s VOL / %s AMP", Ventchar, Eolchar, tension_batterie, Courant);  // la chaine à parser est dans une String, avec la méthode c_str()
//      VitesseVent_float = atof(Ventchar),0;               // char convertie en Float, avec 0 décimales
//      rpmEolienne_float = atof(Eolchar),0;                // char convertie en Float, avec 0 décimales
//      tension_batterie_float = atof(tension_batterie),3;  // char convertie en Float, avec 3 décimales
//      Courant_float = atof(Courant),2;                    // char convertie en Float, avec 2 décimales
//      Serial.print("VENT:");
//      Serial.println(VitesseVent_float,0);
//      Serial.print("EOLIENNE:");
//      Serial.println(rpmEolienne_float,0);

// scanf avec int et char, puis conversion float pour les valeurs en char :
      sscanf(chaine.c_str(), "%d KMH / %d RPM / %s VOL / %s AMP", &VitesseVent, &rpmEolienne, tension_batterie, Courant);  // la chaine à parser est dans une String, avec la méthode c_str()
      tension_batterie_float = atof(tension_batterie),3;  // char convertie en Float, avec 3 décimales
      Courant_float = atof(Courant),2;                    // char convertie en Float, avec 2 décimales
      Serial.print("VENT:");
      Serial.println(VitesseVent);
      Serial.print("EOLIENNE:");
      Serial.println(rpmEolienne);

      Serial.print("VOLTS: ");
      Serial.println(tension_batterie_float,3); // float avec 3 décimales
      Serial.print("AMPERES: ");
      Serial.println(Courant_float,3);
      Serial.print("Watt: ");
      Serial.println(Courant_float*Voltage,0); // float avec 0 décimales
      Serial.println(' ');
      
      chaine = "";    // vide la String chaine
    }
  }
}

void CalculReception ()
{
//VitesseVent=359.29;
//rpmEolienne=321.00;
  VitesseVentC = VitesseVent / 100;   // par calcul, extrait la centaine  
  VitesseVentD = VitesseVent % 100 / 10;   // par calcul, extrait la dizaine
  VitesseVentU = VitesseVent % 10;  // par calcul, extrait l'unités
    Serial.print(" VitesseVentC: ");
    Serial.println(VitesseVentC);
    Serial.print(" VitesseVentD: ");
    Serial.println(VitesseVentD); 
    Serial.print(" VitesseVentU: ");
    Serial.println(VitesseVentU); 

  VitesseEolienneC = rpmEolienne / 100;   // par calcul, extrait la centaine  
  VitesseEolienneD = rpmEolienne % 100 / 10;   // par calcul, extrait la dizaine
  VitesseEolienneU = rpmEolienne % 10;  // par calcul, extrait l'unités
    Serial.println("  ");
    Serial.print(" VitesseEolienneC: ");
    Serial.println(VitesseEolienneC);
    Serial.print(" VitesseEolienneD: ");
    Serial.println(VitesseEolienneD); 
    Serial.print(" VitesseEolienneU: ");
    Serial.println(VitesseEolienneU); 
}


void Afficheurs() {
 //==== Afficheurs ====//
 
// adresse le chiffre des Dizaines sur l'afficheur du bas
  switch (VitesseEolienneD)
  {
  case 0:
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Zero();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Un();
  digitalWrite(latchpin1, 0); 
  break;

  case 2:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Deux();
  digitalWrite(latchpin1, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Trois();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Quatre();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Cinq();
  digitalWrite(latchpin1, 0); 
  break;

  case 6:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Six();
  digitalWrite(latchpin1, 0); 
  break;

  case 7:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Sept();
  digitalWrite(latchpin1, 0); 
  break;

  case 8:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Huit();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Neuf();
  digitalWrite(latchpin1, 0); 
  break;
  }
// adresse le chiffre des Centaines sur l'afficheur du Haut
  switch (VitesseVentC) 
  {
  case 0:
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Zero();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Un();
  digitalWrite(latchpin1, 0); 
  break;

  case 2:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Deux();
  digitalWrite(latchpin1, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Trois();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Quatre();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Cinq();
  digitalWrite(latchpin1, 0); 
  break;

  case 6:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Six();
  digitalWrite(latchpin1, 0); 
  break;

  case 7:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Sept();
  digitalWrite(latchpin1, 0); 
  break;

  case 8:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Huit();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Neuf();
  digitalWrite(latchpin1, 0); 
  break;
  }
// adresse le chiffre des Dizaines sur l'afficheur du Haut 
  switch (VitesseVentD)
  {
  case 0:
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Zero();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Un();
  digitalWrite(latchpin1, 0); 
  break;

  case 2:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Deux();
  digitalWrite(latchpin1, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Trois();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Quatre();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Cinq();
  digitalWrite(latchpin1, 0); 
  break;

  case 6:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Six();
  digitalWrite(latchpin1, 0); 
  break;

  case 7:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Sept();
  digitalWrite(latchpin1, 0); 
  break;

  case 8:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Huit();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Neuf();
  digitalWrite(latchpin1, 0); 
  break;
  }
// adresse le chiffre des Unité sur l'afficheur du Haut
  switch (VitesseVentU)
  {
  case 0:
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Zero();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Un();
  digitalWrite(latchpin1, 0); 
  break;

  case 2:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Deux();
  digitalWrite(latchpin1, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Trois();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Quatre();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Cinq();
  digitalWrite(latchpin1, 0); 
  break;

  case 6:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Six();
  digitalWrite(latchpin1, 0); 
  break;

  case 7:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Sept();
  digitalWrite(latchpin1, 0); 
  break;

  case 8:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Huit();
  digitalWrite(latchpin1, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin1, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Neuf();
  digitalWrite(latchpin1, 0); 
  break;
  }
  
// adresse les Unités de mesures  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[4]); 
  K();
  digitalWrite(latchpin1, 0);            
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[5]); 
  m();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[7]); 
  m();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[8]); 
  p();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[9]); 
  h();
  digitalWrite(latchpin2, 0); 
  
// adresse le chiffre des Unités sur l'afficheur du Bas
  switch (VitesseEolienneU)
  {
  case 0:
  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Zero();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Un();
  digitalWrite(latchpin2, 0); 
  break;

  case 2:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Deux();
  digitalWrite(latchpin2, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Trois();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Quatre();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Cinq();
  digitalWrite(latchpin2, 0); 
  break;

  case 6:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Six();
  digitalWrite(latchpin2, 0); 
  break;

  case 7:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Sept();
  digitalWrite(latchpin2, 0); 
  break;

  case 8:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Huit();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Neuf();
  digitalWrite(latchpin2, 0); 
  break;
  }
  
// adresse les Unités de mesures  
  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[13]); 
  r();
  digitalWrite(latchpin2, 0); 
             
// adresse le chiffre des Centaines sur l'afficheur du Bas 
  switch (VitesseEolienneC)
  {
  case 0:
  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Zero();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 1: 
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Un();
  digitalWrite(latchpin2, 0); 
  break;

  case 2:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Deux();
  digitalWrite(latchpin2, 0); 
  break;
 
  case 3:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Trois();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 4:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Quatre();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 5:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Cinq();
  digitalWrite(latchpin2, 0); 
  break;

  case 6:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Six();
  digitalWrite(latchpin2, 0); 
  break;

  case 7:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Sept();
  digitalWrite(latchpin2, 0); 
  break;

  case 8:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Huit();
  digitalWrite(latchpin2, 0); 
  break;
  
  case 9:   
  digitalWrite(latchpin2, 1);
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Neuf();
  digitalWrite(latchpin2, 0); 
  break;
  }
 // delay(300);
}






void Nodata() {
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  blank();
  digitalWrite(latchpin1, 0);  
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  N();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  o();
  digitalWrite(latchpin1, 0);             
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  D();
  digitalWrite(latchpin1, 0);            
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[4]); 
  a();
  digitalWrite(latchpin1, 0); 
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[5]); 
  t();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[7]); 
  blank();
  digitalWrite(latchpin1, 0); 
    digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[8]); 
  blank();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[9]); 
  a();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  blank();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[13]); 
  blank();
  digitalWrite(latchpin2, 0);   

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  blank();
  digitalWrite(latchpin2, 0); 

    delay(5000);
}


//==== Fonctions Afficheurs ====//

void Zero ()
{
  digitalWrite(Data6, LOW);
  digitalWrite(Data5, HIGH);
  digitalWrite(Data4, HIGH);
  digitalWrite(Data3, LOW);
  digitalWrite(Data2, LOW);
  digitalWrite(Data1, LOW);
  digitalWrite(Data0, LOW);
}
void Un ()
{
  digitalWrite(Data6, LOW);
  digitalWrite(Data5, HIGH);
  digitalWrite(Data4, HIGH);
  digitalWrite(Data3, LOW);
  digitalWrite(Data2, LOW);
  digitalWrite(Data1, LOW);
  digitalWrite(Data0, HIGH);
}
void Deux ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 0);
}
void Trois ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void Quatre ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void Cinq ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 1);
}
void Six ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 0);
}
void Sept ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void Huit ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void Neuf ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 1);
}
void r ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 0);
}
void p ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void m ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 1);
}
void K ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 0);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void h ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void slash ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void x ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void N ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 0);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 0);
}
void o ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void W ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 0);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 1);
}
void i ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 1);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 1);
}
void LettreF ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 0);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 1);
  digitalWrite(Data0, 0);
}
void a ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 1);
}
void D ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 0);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void t ()
{
  digitalWrite(Data6, 1);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 1);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 1);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}
void blank ()
{
  digitalWrite(Data6, 0);
  digitalWrite(Data5, 1);
  digitalWrite(Data4, 0);
  digitalWrite(Data3, 0);
  digitalWrite(Data2, 0);
  digitalWrite(Data1, 0);
  digitalWrite(Data0, 0);
}


void TestAfficheurs() {
          
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[0]); 
  Sept();
  digitalWrite(latchpin1, 0);  
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[1]); 
  Zero();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[2]); 
  Un();
  digitalWrite(latchpin1, 0);             
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[3]); 
  Deux();
  digitalWrite(latchpin1, 0);            
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[4]); 
  Trois();
  digitalWrite(latchpin1, 0); 
  
  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[5]); 
  Quatre();
  digitalWrite(latchpin1, 0); 

  digitalWrite(latchpin1, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[7]); 
  p();
  digitalWrite(latchpin1, 0); 



  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[8]); 
  r();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[9]); 
  Cinq();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
  Huit();
  digitalWrite(latchpin2, 0); 

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[13]); 
  Neuf();
  digitalWrite(latchpin2, 0);   

  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
  Six();
  digitalWrite(latchpin2, 0); 
  
//  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
//  shiftOut(datapin, clockpin, LSBFIRST, dataArray[12]); 
//  x();
//  digitalWrite(latchpin2, 0); 
//
//  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
//  shiftOut(datapin, clockpin, LSBFIRST, dataArray[13]); 
//  x();
//  digitalWrite(latchpin2, 0); 
//
//  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
//  shiftOut(datapin, clockpin, LSBFIRST, dataArray[14]); 
//  x();
//  digitalWrite(latchpin2, 0); 
//
//  digitalWrite(latchpin2, 1);                              // latch à l'état HAUT pour autoriser le transfert des données série   
//  shiftOut(datapin, clockpin, LSBFIRST, dataArray[15]); 
//  x();
//  digitalWrite(latchpin2, 0); 
  delay(300);
}
