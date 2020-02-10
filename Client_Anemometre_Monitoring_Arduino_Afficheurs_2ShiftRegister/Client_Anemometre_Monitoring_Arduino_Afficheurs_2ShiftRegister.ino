#include <SoftwareSerial.h>
#include <Arduino_CRC16.h>  // Librairie à ajouter via le gestionnaire de lib de l'appli arduino

#include "config.h"
#include "fonctions.h"

/****************/
/* DÉCLARATIONS */
/****************/

//——— Prototypes Fonctions ———//
void Reception(void);

//——— Radio HC-12 ———//
SoftwareSerial HC12(txPin, rxPin);

//——— Calcul CRC ———//
Arduino_CRC16 crc16;

//https://plaisirarduino.fr/moniteur-serie/
//https://forum.arduino.cc/index.php?topic=553743.0

//######################
//# VARIABLES GLOBALES #
//######################

float tension_batterie_float;
float Courant_float;
int VitesseVent_int;
int rpmEolienne_int;

// Partie MONITORING
float Voltage=12;
float MiniC=0.08;           // courant minimum mesuré en dessous duquel la valeur est forcée à 0.

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
void Reception(void) {
  
  uint8_t i = 0;
  uint8_t x = 0;
  uint8_t ESPACE1 = 0;
  uint8_t ESPACE2 = 0;
  uint8_t ESPACE3 = 0;
  uint8_t ESPACE4 = 0;
  uint8_t ESPACE5 = 0;
  uint8_t index = 0;
  uint8_t index_ESPACE = 1;

  char acquis_data;
  
  char VitesseVent[4] = {0, 0, 0, 0};
  char rpmEolienne[5] = {0, 0, 0, 0, 0};
  char tension_batterie[8] = {0, 0, 0, 0, 0, 0, 0, 0};   // Tableau de 8 caractères + null pour stocker le texte
  char Courant[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  char ChecksumReceived[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  char msg[40];   // création du tableau de réception vide
  flushTableau(msg, 40);

//  float Courant_float;
  float Voltage=12;
  float MiniC=0.08;           // courant minimum mesuré en dessous duquel la valeur est forcée à 0.
  
  String chaine;
  String chaineCONTROLE;

  x = 0;
  while (HC12.available()>0) {  // If HC-12 has data
    acquis_data = HC12.read();
    msg[x] = acquis_data; // remplissage l'une après l'autre de chaque colonne du tableau, avec les valeurs qui arrivent à la suite les unes des autres, pour reformer le message
    x++;  // incrément pour changer de colonne
//    Serial.print("x : ");
//    Serial.println(x);    
//    Serial.println(msg);

    if (msg[0] != char_VT) {
      Serial.println("msg tronqué à la réception");
    }
    else {

// message reçu de la forme 
// |49 33 11.405 -12.500 3cfe1c9
// Pour chaque ligne on fait :
      if (acquis_data == char_LF) {   //détection de fin de ligne : méthodes ascii meilleure !
        Serial.print("msg :");
        Serial.println(msg);
  
        for(index = 0; (index < 40) ; index++) {
          if (msg[index] == char_SPACE) {
            switch(index_ESPACE) {
              case 1:
                ESPACE1 = index;
                break;
              case 2:
                ESPACE2 = index;
                break;
              case 3:
                ESPACE3 = index;
                break;
              case 4:
                ESPACE4 = index;
                break;  
              case 5:
                ESPACE5 = index;
                break;          
            }
            index_ESPACE++;
          } 
        }
  
  //      Serial.print("ESPACE1 :");
  //      Serial.println(ESPACE1);
  //      Serial.print("ESPACE2 :");
  //      Serial.println(ESPACE2);
  //      Serial.print("ESPACE3 :");
  //      Serial.println(ESPACE3);
  //      Serial.print("ESPACE4 :");
  //      Serial.println(ESPACE4);     
  //      Serial.print("ESPACE5 :");
  //      Serial.println(ESPACE5);  
  
  // Extraction des données provenant du message
      // Boucle qui relit le tableau pour récupérer la vitesse du vent
        x = 0;
        for (i = 1; i < ESPACE1; i++) {
          VitesseVent[x] = msg[i]; 
          x++;     
        }
         
      // Boucle qui relit le tableau pour récupérer la vitesse éolienne
        x = 0;
        for (i = (ESPACE1+1); i < ESPACE2; i++) {
          rpmEolienne[x] = msg[i]; 
          x++;     
        }
       
      // Boucle qui relit le tableau pour récupérer le Voltage
        x = 0;
        for (i = (ESPACE2+1); i < ESPACE3; i++) {
          tension_batterie[x] = msg[i]; 
          x++;        
        }
      
      // Boucle qui relit le tableau pour récupérer le Courant
        x = 0;
        for (i = (ESPACE3+1); i < ESPACE4; i++) {
          Courant[x] = msg[i]; 
          x++;        
        }
      
      // Boucle qui relit le tableau pour récupérer le ChecksumReceived
        x = 0;
        for ((i = ESPACE4+1); i < ESPACE5; i++) {
          ChecksumReceived[x] = msg[i]; 
          x++;        
        }
  
   // Conversions utiles pour la fonction CalculReception()
          VitesseVent_int = atoi(VitesseVent);  // char convertie en Float, avec 0 décimales
          rpmEolienne_int = atoi(rpmEolienne);                    // char convertie en Float, avec 0 décimales
          tension_batterie_float = atof(tension_batterie),3;  // char convertie en Float, avec 3 décimales
          Courant_float = atof(Courant),2;                    // char convertie en Float, avec 2 décimale
        
  // Reconstruction de la chaine
        chaineCONTROLE = char_VT + String(VitesseVent) + char_SPACE + String(rpmEolienne) + char_SPACE + String(tension_batterie) + char_SPACE + String(Courant);
        Serial.print("chaineCONTROLE :");
        Serial.println(chaineCONTROLE);  
        
  // Calcul du Checksum
        uint16_t const ChecksumCalcul = crc16.calc((uint8_t const *)chaineCONTROLE.c_str(), strlen(chaineCONTROLE.c_str()));
        Serial.print("Checksum Calculé = 0x");
        Serial.println(ChecksumCalcul, HEX);
        Serial.print("Checksum reçu :");
        Serial.println(ChecksumReceived);    
        
  // Affichage de contrôle  
        Serial.print("VENT :");
        Serial.println(VitesseVent); 
        Serial.print("EOLIENNE :");
        Serial.println(rpmEolienne);
        Serial.print("VOLTS: ");
        Serial.println(tension_batterie_float,3); // float avec 3 décimales
        Serial.print("AMPERES: ");
        Serial.println(Courant_float,3);   
  //      Serial.print("Watt: ");
  //      Serial.println(Courant_float*Voltage,0); // float avec 0 décimales
        Serial.println("");
  
  // Comparaison du Checksum calculé au Checksum reçu
        if (String(ChecksumCalcul, HEX) == ChecksumReceived) {
  
  // Affichage LCD courant et Puissance
          if ( Courant_float < MiniC ) {   // remise à zero forcée si valeur mesurée très petite
            Courant_float = 0;
          }


        } 
        else {
          Serial.println("");
          Serial.println("ERREUR DURANT LA RECEPTION");
          Serial.println("");
        }
      }
    }    
  }
}


void CalculReception ()
{
//VitesseVent=359.29;
//rpmEolienne=321.00;
  VitesseVentC = VitesseVent_int / 100;   // par calcul, extrait la centaine  
  VitesseVentD = VitesseVent_int % 100 / 10;   // par calcul, extrait la dizaine
  VitesseVentU = VitesseVent_int % 10;  // par calcul, extrait l'unités
    Serial.print(" VitesseVentC: ");
    Serial.println(VitesseVentC);
    Serial.print(" VitesseVentD: ");
    Serial.println(VitesseVentD); 
    Serial.print(" VitesseVentU: ");
    Serial.println(VitesseVentU); 

  VitesseEolienneC = rpmEolienne_int / 100;   // par calcul, extrait la centaine  
  VitesseEolienneD = rpmEolienne_int % 100 / 10;   // par calcul, extrait la dizaine
  VitesseEolienneU = rpmEolienne_int % 10;  // par calcul, extrait l'unités
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
