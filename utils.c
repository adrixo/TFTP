#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void addToLog(char * hostname, int ip, char * protocol, int port)
{
	FILE *fd;

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("File error", stderr); return;}
	fprintf(fd, "Hostname: %s ip: %d protocol: %s Port: %d Date: d\n", hostname, ip, protocol, port);
	fclose(fd);
}
