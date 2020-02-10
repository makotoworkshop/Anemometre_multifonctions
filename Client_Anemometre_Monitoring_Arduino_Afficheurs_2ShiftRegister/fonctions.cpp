#include <stdint.h>
#include "fonctions.h"

void flushTableau( char *ptrTab, uint8_t sizeTab)
{
  uint8_t ind = 0;

  for(ind = 0; ind < sizeTab; ind++)
    ptrTab[ind] = 0;  
}
