/* CRC16.ino
 * 
 * This sketch demonstrates how to use the Arduino_CRC32
 * library to calculate a CRC-32 checksum.
 * 
 * Alexander Entinger
 */

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include <Arduino_CRC16.h>

/**************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************/

Arduino_CRC16 crc16;

/**************************************************************************************
 * SETUP/LOOP
 **************************************************************************************/

void setup()
{
  Serial.begin(9600);

  unsigned long const start = millis();
  for(unsigned long now = millis(); !Serial && ((now - start) < 5000); now = millis()) { };

  char const str[] = "Hello CRC16 ;)";

  uint16_t const crc16_res = crc16.calc((uint8_t const *)str, strlen(str));

  Serial.print("CRC16(\"");
  Serial.print(str);
  Serial.print("\") = 0x");
  Serial.println(crc16_res, HEX);
}

void loop()
{
  
}
