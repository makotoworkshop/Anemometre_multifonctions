/* Arduino Anemometer and Tachometer Éolienne
IN : capteur à effet Hall Anémomètre est connecté à la pin 2 = int0
IN : capteur à effet Hall Éolienne est connecté à la pin 3 = int1  
OUT : HC-12 Radio Default Mode 9600bps
*/

#include <Arduino_CRC32.h>  // Librairie à ajouter via le gestionnaire de lib de l'appli arduino
#include <SoftwareSerial.h>
#include <avr/sleep.h>
/****************/
/* DÉCLARATIONS */
/****************/
// Partie ANEMO
SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin
unsigned long rpmVent = 0;
unsigned long rpmEol = 0;
unsigned long vitVentKMH = 0;
unsigned long vitEolRPM = 0;
unsigned long dateDernierChangementVent = 0;
unsigned long dateDernierChangementEol = 0;
unsigned long dateDernierChangementRPM = 0;
unsigned long dateDernierChangementKMH = 0;
unsigned long vitVentCourante = 0;
unsigned long vitVentDernierChangement = 0;  
float intervalleKMH = 0;
float intervalleRPM = 0;
const char char_LF = 10; // charactère saut de ligne ascii
const char char_SPACE = 32; // charactère ESPACE ascii, le meilleur caractère pour la détection de string de sscanf !
String chaine;
String message;
int rotaAnemo = 0;
int rotaEol = 0;
#define ATpin 7 // used to switch HC-12 to AT mode

// Partie MONITORING
float Courant=0;
float SupplyVoltage=12;
const int Voie_0=0; // declaration constante de broche analogique
int mesure_brute=0; // Variable pour acquisition résultat brut de conversion analogique numérique
float mesuref=0.0;  // Variable pour calcul résultat décimal de conversion analogique numérique
float tension=0.0;  // Variable tension mesurée
float tension_batterie=0.0; // Variable tension batterie calculée
float tension_regulateur=8925.0;  // Variable tension réelle aux bornes du régulateur -9V (en mV)

int PIN_ACS712 = A1;
double Voltage = 0;
double Current = 0;
char MessageTensionBatterie[] = " VOL / "; // message to be sent; '\n' is a forced terminator char
char MessageCourant[] = " AMP"; // message to be sent; '\n' is a forced terminator char
Arduino_CRC32 crc32;

/*********/
/* SETUP */
/*********/  
void setup() {
  HC12.begin(9600);               // Serial port to HC12
  // debug uniquement, penser à commenter toutes les lignes «Serial…» pour éviter les erreurs de valeur (calcul trop long pour l'interruption)
  Serial.begin(9600);             // Serial port to computer
  // Pin capteurs
  attachInterrupt(digitalPinToInterrupt(2), rpm_vent, FALLING);  // le capteur à effet Hall Anémomètre est connecté à la pin 2 = int0
  attachInterrupt(digitalPinToInterrupt(3), rpm_eol, FALLING);  // le capteur à effet Hall Éolienne est connecté à la pin 3 = int1  
  pinMode(PIN_ACS712, INPUT);
  
  pinMode(ATpin, OUTPUT);
  digitalWrite(ATpin, LOW); // Set HC-12 into AT Command mode
  delay(500);
  HC12.print("AT+C010");  // passer sur le canal 036 (433.4Mhz + 36x400KHz)
  delay(500);
  digitalWrite(ATpin, HIGH); // HC-12 en normal mode
}

/*************/
/* PROGRAMME */
/*************/
void loop() {
  MesureCourant();
  MesureBrute();
  TensionMesuree();
  TensionBatterie();
//  for (int i = 0; i < 10; i++) {
//    Courant = Courant+0.01;
//    if (Courant >= 12){
//      Courant = 0;
//    }
//  }
  RemiseZeroVitVentKMHnew2 ();
  RemiseZeroVitEolRPMnew2 ();

// Construction de la chaine
  chaine =  String(vitVentKMH)+ char_SPACE + String(vitEolRPM) + char_SPACE + String(tension_batterie,3) + char_SPACE + String(Current,3);  // construction du message
  Serial.println ( "chaine String :" +chaine );
  // Message de la forme suivante : 49 338 11.405 -12.500

// Calcul du Checksum
  unsigned long const start = millis();
  for(unsigned long now = millis(); !Serial && ((now - start) < 5000); now = millis()) { };
  uint32_t const crc32_res = crc32.calc((uint8_t const *)chaine.c_str(), strlen(chaine.c_str()));
  Serial.print("CRC32 = 0x");
  Serial.println(crc32_res, HEX);
  
// Checksum joint au message
  message = chaine+ char_SPACE + String(crc32_res, HEX) + char_SPACE + char_LF;
  Serial.println ( "message :" +message ); 
   
// send radio data
//  HC12.print(chaine);
  HC12.print(message);
  // Message de la forme suivante : 49 338 11.405 -12.500 3cfe1c9
  
  delay(100);
}

/*************/
/* FONCTIONS */
/*************/

void rpm_vent()   // appelée par l'interruption, Anémomètre vitesse du vent.
{ 
  unsigned long dateCourante = millis();
  intervalleKMH = (dateCourante - dateDernierChangementVent);
//  Serial.print ( "intervalle en s : " );
//  Serial.println (intervalleKMH/1000); // affiche l'intervalle de temps entre deux passages
  if (intervalleKMH != 0)  // attention si intervalle = 0, division par zero -> erreur
  {
    rpmVent = 60 / (intervalleKMH /1000);  
  }
  vitVentKMH = ( rpmVent + 6.174 ) / 8.367;
  Serial.print ( "vitVentKMH : " );
  Serial.println ( vitVentKMH ); // affiche les rpm  
  Serial.println ( "" );
  dateDernierChangementVent = dateCourante;
  rotaAnemo = 1;

}

void rpm_eol()    // appelée par l'interruption, Tachymétre rotation éolienne.
{
  unsigned long dateCourante = millis();
  intervalleRPM = (dateCourante - dateDernierChangementEol);
//  Serial.print ( "intervalle en s : " );
//  Serial.println (intervalleRPM/1000); // affiche l'intervalle de temps entre deux passages
  if (intervalleRPM != 0)  // attention si intervalle = 0, division par zero -> erreur
  {  
    vitEolRPM = 60 / (intervalleRPM /1000);
  }
  Serial.print ( "rpm : " );
  Serial.println ( vitEolRPM ); // affiche les rpm  
  Serial.println ( "" );
  dateDernierChangementEol = dateCourante;
  rotaEol = 1;
}


void RemiseZeroVitVentKMH ()
{
  unsigned long dateCouranteKMH = millis();
  if (rotaAnemo == 1 ){   // teste si l'Anémomètre tourne
 //   Serial.println ( "Anémo tourne ");
    rotaAnemo = 0;
  }
  else    // Si ça ne tourne plus (valeur plus mise à jour)
  {  
    float dureeKMH = (dateCouranteKMH - dateDernierChangementKMH);
//    if (dureeKMH > 10000) // Si ça ne tourne plus depuis 10 secondes
    if (dureeKMH > 16000) // Si ça ne tourne plus depuis 16 secondes, délai un peu augmenté vis à vis du mode veille.
    {
//      Serial.print ( "dureeKMH : " );
//      Serial.println ( dureeKMH ); // affiche les rpm  
      vitVentKMH = 0;  // Remsise à zero !
      dateDernierChangementKMH = dateCouranteKMH;   
    }
  }
}

void RemiseZeroVitVentKMHnew2 ()
{
  unsigned long dateCouranteKMH = millis();
  if (rotaAnemo == 1 ){   // teste si l'Anémomètre tourne
 //   Serial.println ( "Anémo tourne ");
    float dureeKMH = (dateCouranteKMH - dateDernierChangementKMH);
    if (dureeKMH > 10000) {
      rotaAnemo = 0;
      dateDernierChangementKMH = dateCouranteKMH; 
    }
  }
  else    // Si ça ne tourne plus (valeur plus mise à jour)
  {  
    float dureeKMH = (dateCouranteKMH - dateDernierChangementKMH);
    if (dureeKMH > 6000) // Si ça ne tourne plus depuis 16 secondes, délai un peu augmenté vis à vis du mode veille.
    {
 //     Serial.print ( "dureeKMH : " );
  //    Serial.println ( dureeKMH ); // affiche les rpm  
      vitVentKMH = 0;  // Remsise à zero !
      dateDernierChangementKMH = dateCouranteKMH;   
    }
  }
}

void RemiseZeroVitEolRPMnew2 ()
{
  unsigned long dateCouranteRPM = millis();
  if (rotaEol == 1 ){ // teste si l'éolienne tourne
//    Serial.println ( "Éolienne tourne ");
    float dureeRPM = (dateCouranteRPM - dateDernierChangementRPM);
    if (dureeRPM > 10000) {
      rotaEol = 0;
      dateDernierChangementRPM = dateCouranteRPM;
    }
  }
  else    // Si ça ne tourne plus (valeur plus mise à jour)
  {  
    float dureeRPM = (dateCouranteRPM - dateDernierChangementRPM);
    if (dureeRPM > 6000) // Si ça ne tourne plus depuis 65 secondes (soit moins de 1 rpm), changé pour 8 sec en charge
    {
//      Serial.print ( "dureeRPM : " );
//      Serial.println ( dureeRPM ); // affiche les rpm  
      vitEolRPM = 0;  // Remsise à zero !
      dateDernierChangementRPM = dateCouranteRPM;    
    }
  }
}

void RemiseZeroVitVentKMHnew ()
{
  unsigned long dateCouranteKMH = millis();
  float dureeKMH = (dateCouranteKMH - dateDernierChangementKMH);
    vitVentCourante = vitVentKMH;
 //   Serial.print ( "vitVentCourante : " );
 //   Serial.println ( vitVentCourante ); 

  if ((dureeKMH > 3000) && (vitVentCourante == vitVentKMH)) {

   // Serial.println ( "Anémo tourne ? ");

//    if (vitVentCourante == vitVentKMH) {
//      
//      
//      Serial.print ( "dureeKMH : " );
//      Serial.println ( dureeKMH ); // affiche les rpm  
//      vitVentKMH = 0;  // Remsise à zero !
//      dateDernierChangementKMH = dateCouranteKMH;   
//        
//  // vitVentDernierChangement = vitVentCourante;          //   vitVentCourante = vitVentKMH;
//      vitVentCourante = 0;          //   vitVentCourante = vitVentKMH;
//
//    }

  }

//        Serial.print ( "vitVentDernierChangement : " );
//        Serial.println ( vitVentDernierChangement ); 
 //       Serial.print ( "vitVentCourante : " );
 //       Serial.println ( vitVentCourante ); 
}


void RemiseZeroVitEolRPM ()
{
  unsigned long dateCouranteRPM = millis();
  if (rotaEol == 1 ){ // teste si l'éolienne tourne
//    Serial.println ( "Éolienne tourne ");
    rotaEol = 0;
  }
  else    // Si ça ne tourne plus (valeur plus mise à jour)
  {  
    float dureeRPM = (dateCouranteRPM - dateDernierChangementRPM);
    if (dureeRPM > 8000) // Si ça ne tourne plus depuis 65 secondes (soit moins de 1 rpm), changé pour 8 sec en charge
    {
//      Serial.print ( "dureeRPM : " );
//      Serial.println ( dureeRPM ); // affiche les rpm  
      vitEolRPM = 0;  // Remsise à zero !
      dateDernierChangementRPM = dateCouranteRPM;    
    }
  }
}



// Partie MONITORING

void MesureCourant() {
//  Serial.print("Courant : ");
//  Serial.print(Courant);
//  Serial.print(" A | Puissance : ");
//  Serial.print(Courant*SupplyVoltage);
//  Serial.println(" Watt");

// Voltage is Sensed 1000 Times for precision
for(int i = 0; i < 1000; i++) { // ça ralentis tout, mais c'est indispensable !
Voltage = (Voltage + (0.004882812 * analogRead(PIN_ACS712))); // (5 V / 1024 (Analog) = 0.0049) which converter Measured analog input voltage to 5 V Range
delay(1);
}
Voltage = Voltage /1000;

Current = (Voltage -2.5)/ 0.100; // Sensed voltage is converter to current (0.100 pour modèle 20A)

//Serial.print("\n Voltage Sensed (V) = "); // shows the measured voltage
//Serial.print(Voltage,3); // the ‘2’ after voltage allows you to display 2 digits after decimal point
//Serial.print("\t Current (A) = "); // shows the voltage measured
//Serial.println(Current,3); // the ‘2’ after voltage allows you to display 2 digits after decimal point



}

void MesureBrute() {
//-------- mesure brute --------
  mesure_brute=analogRead(Voie_0);
//  Serial.print ("Valeur brute = "); 
//  Serial.print (mesure_brute); 
//  Serial.println (" "); // espace de propreté
}

void TensionMesuree() {
//---------- tension mesurée ---------
  mesuref=float(mesure_brute)*5000.0/1024.0;
  tension=mesuref/1000.0; // en Volts
//  Serial.print ("Tension = "); 
//  Serial.print(tension,3);  // float avec 2 décimales 
//  Serial.println(" V ");  // unité et espace de propreté
}

void TensionBatterie() {
//---------- tension batterie ---------
  tension_batterie=mesuref+tension_regulateur;
  tension_batterie=tension_batterie/1000.0; // en Volts
//  lcd.setCursor(0,1) ; // positionne le curseur à l'endroit voulu (colonne, ligne)
//  lcd.print ("Batt:"); 
//  lcd.print (tension_batterie,2); // float avec 2 décimales
//  lcd.print ("V "); // unité et espace de propreté
//  Serial.print ("Batterie = "); 
//  Serial.print(tension_batterie,3); // float avec 2 décimales 
//  Serial.println(" V ");
//  Serial.println("  ");
}
