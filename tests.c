#include <stdio.h>
#include <stdlib.h>
#include "utils.c"

void main(void)
{
  char * h;
  printf("%d\n", sizeof(char));
  h = WRQ("123456x8", "octet");
  printf("%d\n", getPacketType(h));
  printf("s%s\n", getFilename(h));

  h = DATAPacket(256, "123456789");
  printf("%d\n", getPacketType(h));

  h = ACK(3242);
  printf("%d\n", getPacketType(h));

  h = ERRORMSG(1, "Ya no funciona");
  printf("%d\n", getPacketType(h));
  getErrorMsg(h);
}
