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

//Ejemplo: sendErrorMSG(s, clientaddr_in, FICHERONOENCONTRADO, "No se ha podido encontrar");
void sendErrorMSG(int s, struct sockaddr_in clientaddr_in, int codigoError, char * errMsg){
	BYTE * msg;
	int addrlen;
	int nc;
	
	addrlen = sizeof(struct sockaddr_in);

	msg = ErrorMsg(codigoError, errMsg);
	nc = sendto (s, msg, BUFFERSIZE,0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}

	return;
}
