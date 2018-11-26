#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void addToLog(char * hostname, int ip, char * protocol, int port)
{
	FILE *fd;
	
	fd = fopen("log.txt", "a");
	if(fp == NULL ) {fputs("File error", stderr); return;}
	fprintf("Hostname: %s ip: %d protocol: %s Port: %d Date: %d\n", hostname, ip, protocol, port, date);
	fclose(fp);
}
