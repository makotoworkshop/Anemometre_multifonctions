#include <stdint.h>
#include "fonctions.h"

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t CalcSegment(uint8_t numSeg, int valBatt)
{
  uint8_t battChar = 0;
  
  if(valBatt >= (5 * numSeg))
    battChar = 5;
  else
  {
    if(valBatt > (5 * (numSeg -1)))
      battChar = valBatt - (5 * (numSeg -1));
    else
      battChar = 0;
  }

  return battChar;
}


void flushTableau( char *ptrTab, uint8_t sizeTab)
{
  uint8_t ind = 0;

  for(ind = 0; ind < sizeTab; ind++)
    ptrTab[ind] = 0;  
}
