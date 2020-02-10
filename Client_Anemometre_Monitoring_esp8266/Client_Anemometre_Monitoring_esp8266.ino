// Ressources :
//https://forum.arduino.cc/index.php?topic=553743.0
//https://plaisirarduino.fr/moniteur-serie/


#include <SoftwareSerial.h> // lib Transmission série
#include <ESP8266WiFi.h>    // lib ESP Wifi
#include <ESP8266WiFiMulti.h>
#include <Arduino_CRC16.h>  // Librairie à ajouter via le gestionnaire de lib de l'appli arduino

#include "config.h"
#include "fonctions.h"
#include "ESPinfluxdb.h"    // lib ESP InfluxDB

//################
//# DÉCLARATIONS #
//################
//——— Radio HC-12 ———//
SoftwareSerial HC12(txPin, rxPin);

//——— Prototypes Fonctions ———//
void Reception(void);

//——— Calcul CRC ———//
Arduino_CRC16 crc16;

//######################
//# VARIABLES GLOBALES #
//######################

float tension_batterie_float;
float Courant_float;
int VitesseVent_int;
int rpmEolienne_int;

//——— InfluxDB ———//
const char *INFLUXDB_HOST = "xxxx.org";
const uint16_t INFLUXDB_PORT = 8086;
const char *DATABASE = "xxxx";
const char *DB_USER = "xxxx";
const char *DB_PASSWORD = "xxxx";
Influxdb influxdb(INFLUXDB_HOST, INFLUXDB_PORT);  //https://github.com/projetsdiy/grafana-dashboard-influxdb-exp8266-ina219-solar-panel-monitoring/tree/master/solar_panel_monitoring
int compteur = 1;

//——— WiFi ———//
char ssid[] = "xxxx";
char password[] = "xxxx";
ESP8266WiFiMulti WiFiMulti;

//#########
//# SETUP #
//#########
void setup() {
  Serial.begin(9600); // Debug
//——— HC12 ———//
  HC12.begin(9600);           // Serial port to HC12
        
//——— WiFi ———//
  WiFiMulti.addAP(ssid, password);  // Connection au réseau WiFi
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println ("WiFi Connecté"); 
  
//——— InfluxDB ———//
  while (influxdb.opendb(DATABASE, DB_USER, DB_PASSWORD)!=DB_SUCCESS) { // Connexion à la base InfluxDB
    delay(10000);
  }
  Serial.println ("Connexion DataBase OK"); 
}

//#############
//# PROGRAMME #
//#############
void loop() {
  Reception();
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

//float Courant_float;
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
        
   // Conversions utiles pour la fonction SendDataToInfluxdbServer()
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
        Serial.println(VitesseVent_int); 
        Serial.print("EOLIENNE :");
        Serial.println(rpmEolienne_int);
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
//        if ( Courant_float < MiniC ) {   // remise à zero forcée si valeur mesurée très petite
//          Courant_float = 0;
//        }
 //       if (compteur > 6) { // envoie la data toutes les 6 secondes…
          SendDataToInfluxdbServer(); //  l'opération prend 5 secondes à transférer/écrire en base)
 //         compteur = 1;
 //       }
           
        } 
        else {
          Serial.println("");
          Serial.println("ERREUR DURANT LA RECEPTION");
          Serial.println("");
        }
      }
    }    
  }
 //       compteur++;
}


void SendDataToInfluxdbServer() { //Writing data with influxdb HTTP API: https://docs.influxdata.com/influxdb/v1.5/guides/writing_data/
                                  //Querying Data: https://docs.influxdata.com/influxdb/v1.5/query_language/
  // Comparaison du Checksum calculé au Checksum reçu
    dbMeasurement rowDATA("Data");
    rowDATA.addField("VitesseVent", VitesseVent_int);
    rowDATA.addField("RotationEolienne", rpmEolienne_int);
    rowDATA.addField("tension_batterie", tension_batterie_float);
    rowDATA.addField("Courant", Courant_float);
    Serial.println(influxdb.write(rowDATA) == DB_SUCCESS ? " - rowDATA write success" : " - Writing failed");
    influxdb.write(rowDATA);
  
    // Vide les données - Empty field object.
    rowDATA.empty();
}
