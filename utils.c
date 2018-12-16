#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define BUFFERSIZE	1024

void addToLog(char * stringDescriptivo, char * hostname, char * ip, char * protocol, int port)
{
	FILE *fd;
	long timevar;
	time (&timevar);

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("Add to log: file error.", stderr); return;}
	fprintf(fd, "%s Hostname: %s ip: %s Protocol: %s Puerto cliente: %d date: %s",stringDescriptivo, hostname, ip, protocol, port, (char *)ctime(&timevar));

	fclose(fd);
}

void addFileTransferInfoToLog(int type, char * fileName, char * ip)
{
	FILE *fd;
	long timevar;
	time (&timevar);

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("Add to log: file error.", stderr); return;}

	if(type==1) fprintf(fd, "--Recibiendo %s de %s\n", fileName, ip );
	if(type==2) fprintf(fd, "--Enviando %s a %s\n", fileName, ip );

	fclose(fd);
}

void addEndFileTransferInfoToLog(int type, char * fileName, char * ip, char * errmsg)
{
	FILE *fd;
	long timevar;
	time (&timevar);

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("Add to log: file error.", stderr); return;}

	if(type==1) fprintf(fd, "Completada recepcion de %s desde %s a las %s\n", fileName, ip, (char *)ctime(&timevar) );
	if(type==2) fprintf(fd, "Completado envio  de %s desde %s a las %s\n", fileName, ip, (char *)ctime(&timevar) );
	if(type==5) fprintf(fd, "Envío de %s desde %s fallido debido a: %s a las %s\n", fileName, ip, errmsg, (char *)ctime(&timevar) );

	fclose(fd);
}

void addClientFileTransferInfo(int type, int port, char * fileName, char * ip, char * errmsg, int paquete)
{
	FILE *fd;
	long timevar;
	time (&timevar);
	char filename[15];

	sprintf(filename, "%d", port);
  strcat(filename,".txt");

	fd = fopen(filename, "a");
	if(fd == NULL ) {fputs("Add to log: file error.", stderr); return;}

	if(type==0) fprintf(fd, "Inicio de transmision de %s desde %s a las %s\n",fileName, ip,(char *)ctime(&timevar));
	if(type==1) fprintf(fd, "--Recepcion del paquete %d desde %s\n", paquete, ip );
	if(type==2) fprintf(fd, "--Envio del paquete %d desde %s\n", paquete, ip );
	if(type==3) fprintf(fd, "--Completada recepcion de %s desde %s\n", fileName, ip );
	if(type==4) fprintf(fd, "Completada transmisión de %s desde %s a las %s\n",fileName, ip ,(char *)ctime(&timevar));
	if(type==5) fprintf(fd, "Envio de %s desde %s fallido debido a: %s a las %s\n", fileName, ip, errmsg, (char *)ctime(&timevar));

	fclose(fd);
}


//Ejemplo: sendErrorMSG(s, clientaddr_in, FICHERONOENCONTRADO, "No se ha podido encontrar");
void sendErrorMSG_UDP(int s, struct sockaddr_in clientaddr_in, int codigoError, char * errMsg){
	BYTE * msg;
	int addrlen;
	int nc;

	addrlen = sizeof(struct sockaddr_in);

	msg = ErrorMsg(codigoError, errMsg);
	nc = sendto (s, msg, BUFFERSIZE, 0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}

	return;
}

void sendErrorMSG_TCP(int s, int codigoError, char * errMsg){
	BYTE * msg;
	int nc;

	msg = ErrorMsg(codigoError, errMsg);
	nc = send (s, msg, BUFFERSIZE, 0);
	if ( nc == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}

	return;
}
