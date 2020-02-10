//———— Que fait ce programme ? ————
// ---> Récupére les données transmise par un module HC-12,
// ---> Affichage la mesure de la tension d'une batterie au plomb 12V sur un écran LCD.
// ---> Affichage l'intensité et la puissance fournie à la batterie par une génératrice 12V. 
//———— Fonctionnalités utilisées ————
// Utilise un afficheur LCD alphanumérique2x20 en mode 4 bits 
//———— Circuit à réaliser ————
// Broche 12 sur la broche RS du LCD
// Broche 11 sur la broche E du LCD
// Broche 5 sur la broche D4 du LCD
// Broche 4 sur la broche D5 du LCD
// Broche 3 sur la broche D6 du LCD
// Broche 2 sur la broche D7 du LCD
// Broche 7 sur HC-12 to AT mode
// Broche 8 HC-12 TX Pin
// Broche 9 HC-12 RX Pin
//———— Réglages possibles ————
// Canal de réception (voir setup)
// Voltage : Utilisé pour le calcul de la puissance en Watt.
// MiniC : pour régler le courant minimum mesuré en dessous duquel la valeur est forcée à 0, pour prendre
// en compte le fait que la mesure du zero ne tombe jamais pile.

#include <Arduino_CRC16.h>  // Librairie à ajouter via le gestionnaire de lib de l'appli arduino
#include <LiquidCrystal.h>  // lib LCD
#include <SoftwareSerial.h> // lib Transmission série

#include "config.h"
#include "caraclcd.h"
#include "fonctions.h"

//################
//# DÉCLARATIONS #
//################

//——— Prototypes Fonctions ———//
void Jauge(void);
void Reception(void);

//——— Radio HC-12 ———//
SoftwareSerial HC12(txPin, rxPin);

//——— Écran LCD ———//
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);                  // Pins LCD en mode 4 bits 

//——— Calcul CRC ———//
Arduino_CRC16 crc16;

//######################
//# VARIABLES GLOBALES #
//######################

float tension_batterie_float;

//#########
//# SETUP #
//#########
void setup() {

  //——— Serial DEBUG ———//
  Serial.begin(9600);
  
  //——— HC12 ———//
  HC12.begin(9600);           // Serial port to HC12
  pinMode(atPin, OUTPUT);
//  digitalWrite(atPin, LOW);   // HC-12 en mode commande AT
//  delay(500);
//  HC12.print("AT+C010");      // passer sur le canal 056 (433.4Mhz + 56x400KHz)
//  delay(500);
  digitalWrite(atPin, HIGH);  // HC-12 en normal mode

  //——— LCD ———//
  lcd.begin(20,2);            // Initialise le LCD avec 20 colonnes x 2 lignes 
  delay(10);
  lcd.createChar(0, carre00); // Les 8 caractères personnalisés
  lcd.createChar(1, carre01);
  lcd.createChar(2, carre02);
  lcd.createChar(3, carre03);
  lcd.createChar(4, carre04);
  lcd.createChar(5, carre05);  
  lcd.createChar(6, crochetouvrant);
  lcd.createChar(7, crochetfermant);
  lcd.print("LCD OK");        // affiche LCD OK
  delay(2000);
  lcd.clear();
  delay(10);
} 

//#############
//# PROGRAMME #
//#############
void loop() {
  Reception();
  Jauge();
  delay(200); 
} 

//#############
//# FONCTIONS #
//#############
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

  float Courant_float;
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
  
  // Conversions utiles
        tension_batterie_float = atof(tension_batterie),3;  // char convertie en Float, avec 3 décimales
        Courant_float = atof(Courant),2;                    // char convertie en Float, avec 2 décimales
        
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
          lcd.setCursor(0,0);
          lcd.print ("Power:"); 
          lcd.print (Courant_float,2);    // float avec 2 décimales
          lcd.print ("A ");               // unité et espace
          lcd.setCursor(12,0);
          lcd.write(0b01111110);          // caractère : fleche, depuis le Standard Character Pattern du LCD
          lcd.print (" ");
          lcd.print (Courant_float*Voltage,0);
          lcd.print ("Watt ");   
    
  // Affichage LCD Batterie
          lcd.setCursor(0,1);
          lcd.print ("Batt:"); 
          lcd.print (tension_batterie_float,3); 
          lcd.print ("V ");
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


void Jauge(void) {          // Jauge de charge batterie

  uint8_t battChar[6] = {0, 0, 0, 0, 0, 0};
  uint8_t ind = 0;
 
  float NiveauBatterie = mapfloat(tension_batterie_float, 11.4, 12.73, 0, 30);  // Discrétise la valeur de la tension batterie
  uint8_t NiveauBatterieBarre = (uint8_t)NiveauBatterie;                        // conversion en entier

  if (NiveauBatterie > 30) {  // en cas de dépassement des limites haute et basse et permettre l'affichage non-erroné
    NiveauBatterieBarre = 30;
    lcd.setCursor(13, 1);
    lcd.print ("Pleine");
    delay (1000);
  }
  else if (NiveauBatterie < 0) {
    NiveauBatterieBarre = 0;
    lcd.setCursor(13, 1);
    lcd.print ("Alerte");
    delay (1000);
  }
 
//  Serial.print ("NiveauBatterieBarre = ");
//  Serial.print(NiveauBatterieBarre);
//  Serial.println("  ");
 
  // Calcul segments jauge Batterie
  for(ind = 0; ind < 6; ind++)
    battChar[ind] = CalcSegment((ind + 1), NiveauBatterieBarre);
 
  // Affichage Jauge
  lcd.setCursor(12, 1);
  lcd.write(byte(6));   // crochet ouvrant
  for(ind = 0; ind < 6; ind++)
    lcd.write(battChar[ind]); // Segment jauge Batterie 1
  lcd.write(byte(7));   // crochet fermant
}






void JaugeOLD(void) {          // Jauge de charge batterie

  uint8_t battChar1 = 0;
  uint8_t battChar2 = 0;
  uint8_t battChar3 = 0;
  uint8_t battChar4 = 0;
  uint8_t battChar5 = 0;
  uint8_t battChar6 = 0; 
  
  lcd.setCursor(12, 1);
  lcd.write(byte(6));   // crochet ouvrant
  lcd.setCursor(19, 1);
  lcd.write(byte(7));   // crochet fermant
  float NiveauBatterie = mapfloat(tension_batterie_float, 11.4, 12.73, 0, 30); // Discrétise la valeur de la tension batterie
  uint8_t NiveauBatterieBarre = (uint8_t)NiveauBatterie;                               // conversion en entier

  if (NiveauBatterie > 30) {  // en cas de dépassement des limites haute et basse et permettre l'affichage non-erroné
    NiveauBatterieBarre = 30;
    lcd.setCursor(13, 1);
    lcd.print ("Pleine"); 
    delay (1000);
  }
  else if (NiveauBatterie < 0) {
    NiveauBatterieBarre = 0; 
    lcd.setCursor(13, 1);
    lcd.print ("Alerte"); 
    delay (1000);
  }
//  Serial.print ("NiveauBatterieBarre = "); 
//  Serial.print(NiveauBatterieBarre);
//  Serial.println("  ");
  
  // Calcul segment 1 jauge Batterie
  battChar1 = CalcSegment(1, NiveauBatterieBarre);

  // Calcul segment 2 jauge Batterie
  battChar2 = CalcSegment(2, NiveauBatterieBarre);

  // Calcul segment 3 jauge Batterie
  battChar3 = CalcSegment(3, NiveauBatterieBarre);

  // Calcul segment 4 jauge Batterie
  battChar4 = CalcSegment(4, NiveauBatterieBarre);

  // Calcul segment 5 jauge Batterie
  battChar5 = CalcSegment(5, NiveauBatterieBarre);

  // Calcul segment 6 jauge Batterie
  battChar6 = CalcSegment(6, NiveauBatterieBarre);
  

  lcd.setCursor(13, 1);
  lcd.write(battChar1); // Segment jauge Batterie 1
  lcd.write(battChar2); // Segment jauge Batterie 2
  lcd.write(battChar3); // Segment jauge Batterie 3
  lcd.write(battChar4); // Segment jauge Batterie 4
  lcd.write(battChar5); // Segment jauge Batterie 5
  lcd.write(battChar6); // Segment jauge Batterie 6

}
