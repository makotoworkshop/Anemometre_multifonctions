Arduino_CRC16
=============

This Arduino library provides a simple interface to perform checksum calculations utilizing the CRC-16 algorithm. The C code for the CRC-16 algorithm was generated using [PyCRC](https://pycrc.org) with the predefined [crc-16 model](https://pycrc.org/models.html#crc-32).

## Usage

```C++
#include <Arduino_CRC16.h>
/* ... */
Arduino_CRC16 crc16;
/* ... */
char const str[] = "Hello CRC16 ;)";
uint16_t const crc16_res = crc16.calc((uint8_t const *)str, strlen(str));
```
