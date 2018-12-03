#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void addToLog(char * stringDescriptivo, char * hostname, char * ip, char * protocol, int port)
{
	FILE *fd;
	long timevar;
	time (&timevar);

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("Add to log: file error.", stderr); return;}
	fprintf(fd, "%s >> Hostname: %s ip: %s protocol: %s Port: %d date: %s\n",stringDescriptivo, hostname, ip, protocol, port, (char *)ctime(&timevar));

	fclose(fd);
}
