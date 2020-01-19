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

#include <Arduino_CRC32.h>  // Librairie à ajouter via le gestionnaire de lib de l'appli arduino
#include <LiquidCrystal.h>  // lib LCD
#include <SoftwareSerial.h> // lib Transmission série

//################
//# DÉCLARATIONS #
//################
//——— Radio HC-12 ———//
SoftwareSerial HC12(8, 9);  // HC-12 TX Pin, HC-12 RX Pin
//#define ATpin 7             // poir passer le HC-12 en AT mode
char acquis_data;
String chaine;
float Voltage=12;
float MiniC=0.08;           // courant minimum mesuré en dessous duquel la valeur est forcée à 0.
float tension_batterie_float;
float Courant_float;
//int VitesseVent;
//int rpmEolienne;
String chaineCRC32;
const char char_SPACE = 32; // charactère ESPACE ascii, le meilleur caractère pour la détection de string de sscanf !
Arduino_CRC32 crc32;

//——— Écran LCD ———//
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2; // Déclaration LCD
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);                  // Pins LCD en mode 4 bits 
byte carre00[8] = {                                         // caractères personnalisés
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111
};
byte carre01[8] = {
  B11111,
  B00000,
  B10000,
  B10000,
  B10000,
  B10000,
  B00000,
  B11111
};
byte carre02[8] = {
  B11111,
  B00000,
  B11000,
  B11000,
  B11000,
  B11000,
  B00000,
  B11111
};
byte carre03[8] = {
  B11111,
  B00000,
  B11100,
  B11100,
  B11100,
  B11100,
  B00000,
  B11111
};
byte carre04[8] = {
  B11111,
  B00000,
  B11110,
  B11110,
  B11110,
  B11110,
  B00000,
  B11111
};
byte carre05[8] = {
  B11111,
  B00000,
  B11111,
  B11111,
  B11111,
  B11111,
  B00000,
  B11111
};
byte crochetouvrant[8] = {
  B00011,
  B00010,
  B00010,
  B00010,
  B00010,
  B00010,
  B00010,
  B00011
};
byte crochetfermant[8] = {
  B11000,
  B01000,
  B01110,
  B01110,
  B01110,
  B01110,
  B01000,
  B11000
};

//——— Fonction pour pemettre le map avec variable float ———
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//#########
//# SETUP #
//#########
void setup() {
  Serial.begin(9600);         // Debug
  //——— HC12 ———//
  HC12.begin(9600);           // Serial port to HC12
//  pinMode(ATpin, OUTPUT);
//  digitalWrite(ATpin, LOW);   // HC-12 en mode commande AT
//  delay(500);
//  HC12.print("AT+C010");      // passer sur le canal 056 (433.4Mhz + 56x400KHz)
//  delay(500);
//  digitalWrite(ATpin, HIGH);  // HC-12 en normal mode

//——— LCD ———//
  lcd.begin(20,2);            // Initialise le LCD avec 20 colonnes x 2 lignes 
  delay(10);
  lcd.print("LCD OK");        // affiche LCD OK
  delay(2000);
  lcd.clear();
  delay(10);
  lcd.createChar(0, carre00); // Les 8 caractères personnalisés
  lcd.createChar(1, carre01);
  lcd.createChar(2, carre02);
  lcd.createChar(3, carre03);
  lcd.createChar(4, carre04);
  lcd.createChar(5, carre05);  
  lcd.createChar(6, crochetouvrant);
  lcd.createChar(7, crochetfermant);
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
void Reception() {
// surtout pas de delay dans cette boucle, sinon les data reçues sont erronnées.
  while (HC12.available()) {        // If HC-12 has data
    acquis_data = HC12.read();
    chaine = chaine + acquis_data;
    Serial.println (chaine);      // Attention, chaine est donc une String

/* message reçu de la forme 
33/54/12.665/0.045;
pour chaque ligne on fait :*/
    if (acquis_data == 10) {      //détection de fin de ligne : méthodes ascii meilleure !
//    if (chaine.endsWith("\n")) {      //détection de fin de ligne : méthodes String
//      Serial.println ("fin de ligne");            // debug

      char tension_batterie[7];   // chaine de 6 caractères pour stocker le texte avant le mot VOL
      char Courant[8];            // chaine de 4 caractères pour stocker le texte avant le mot AMP
      char Checksum[9];
      char VitesseVent[4];
      char rpmEolienne[4];

//  http://docs.roxen.com/pike/7.0/tutorial/strings/sscanf.xml
 sscanf(chaine.c_str(), "%s %s %s %s %s", VitesseVent, rpmEolienne, tension_batterie, Courant, Checksum);  // la chaine à parser est dans une String, avec la méthode c_str()

      tension_batterie_float = atof(tension_batterie),3;  // char convertie en Float, avec 3 décimales
      Courant_float = atof(Courant),2;                    // char convertie en Float, avec 2 décimales

// Reconstruction de la chaine
      chaineCRC32 = String(VitesseVent) + char_SPACE + String(rpmEolienne) + char_SPACE + String(atof(tension_batterie),3) + char_SPACE + String(atof(Courant),3);
      Serial.println ( "chaineCRC32 :" +chaineCRC32 );

// Calcul du Checksum
      unsigned long const start = millis();
      for(unsigned long now = millis(); !Serial && ((now - start) < 5000); now = millis()) { };
      uint32_t const crc32_res = crc32.calc((uint8_t const *)chaineCRC32.c_str(), strlen(chaineCRC32.c_str()));
      Serial.print("CRC32 = 0x");
      Serial.println(crc32_res, HEX);

// Affichage contrôle   
      Serial.print("VENT:");
      Serial.println(VitesseVent);
      Serial.print("EOLIENNE:");
      Serial.println(rpmEolienne);

      Serial.print("VOLTS: ");
      Serial.println(tension_batterie_float,3); // float avec 3 décimales
      Serial.print("AMPERES: ");
      Serial.println(Courant_float,3);

      Serial.print("Checksum: ");
      Serial.println(Checksum);
//      Serial.print("Watt: ");
//      Serial.println(Courant_float*Voltage,0); // float avec 0 décimales
      Serial.println(' ');
      
// Affichage LCD courant et Puissance
      if ( Courant_float < MiniC ){   // remise à zero forcée si valeur mesurée très petite
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

      chaine = "";    // vide la String chaine
    }
  }
}

void Jauge() {          // Jauge de charge batterie
  lcd.setCursor(12, 1);
  lcd.write(byte(6)); // crochet ouvrant
  lcd.setCursor(19, 1);
  lcd.write(byte(7));   // crochet fermant
  float NiveauBatterie = mapfloat(tension_batterie_float, 11.4, 12.73, 0, 30); // Discrétise la valeur de la tension batterie
  int NiveauBatterieBarre = (int)NiveauBatterie;                      // conversion en entier

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
  
  switch (NiveauBatterieBarre) {
    case 0:
    lcd.setCursor(13, 1);
    lcd.write(byte(0)); // carre00  
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 1:
    lcd.setCursor(13, 1);
    lcd.write(byte(1)); // carre01
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 2:
    lcd.setCursor(13, 1);
    lcd.write(byte(2)); // carre02
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 3:
    lcd.setCursor(13, 1);
    lcd.write(byte(3)); // carre03
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 4:
    lcd.setCursor(13, 1);
    lcd.write(byte(4)); // carre04
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 5:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 6:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(1)); // carre01
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 7:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(2)); // carre02
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 8:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(3)); // carre03
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 9:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(4)); // carre04
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 10:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 11:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(1)); // carre01
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 12:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(2)); // carre02
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 13:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(3)); // carre03
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 14:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(4)); // carre04
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 15:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
    case 16:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(1)); // carre01
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 17:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(2)); // carre02
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 18:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(3)); // carre03
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 19:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(4)); // carre04
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 20:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(0)); // carre00
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 21:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(1)); // carre01
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 22:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(2)); // carre02
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 23:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(3)); // carre03
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 24:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(4)); // carre04
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 25:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(0)); // carre00
      break;
     case 26:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(1)); // carre01
      break;
     case 27:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(2)); // carre02
      break;
     case 28:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(3)); // carre03
      break;
      case 29:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(4)); // carre04
      break;    
      case 30:
    lcd.setCursor(13, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(14, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(15, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(16, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(17, 1);
    lcd.write(byte(5)); // carre05
    lcd.setCursor(18, 1);
    lcd.write(byte(5)); // carre05
      break;       
    default:
      // statements
      break;
  }
}
