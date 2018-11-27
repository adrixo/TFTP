#include <stdio.h>
#include <stdlib.h>
#include "tftp_messageHandler.c"

void main(void)
{
  char * h;

  h = WRQ("123456x8", "octet");
  //printf("%d\n", getPacketType(h));
  //printf("%s\n", getFilename(h));
  printMSG(h);

  h = DATAPacket(256, "123456789");
  //printf("%d\n", getPacketType(h));
  //printf("%s\n", getDataMSG(h));
  printMSG(h);

  h = ACK(3242);
  //printf("%d\n", getPacketType(h));
  printMSG(h);

  h = ErrorMsg(1, "Ya no funciona");
  printMSG(h);
  //printf("%d\n", getPacketType(h));
  //printf("%s\n", getErrorMsg(h));
}
