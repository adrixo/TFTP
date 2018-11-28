#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "tftp_messageHandler.c"

#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_DATOS 512


void serverUDPEnviarFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in);
void serverUDPRecibeFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in);
void clientUDPEnviaFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in);
void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in);

int main(argc, argv)
int argc;
char *argv[];
{
//SERVIDOR
//  recv(mensaje);
//	if(mensaje.leer) serverUDPEnviaFichero(int s, mensaje.Nombrefichero, struct sockaddr_in clientaddr_in);
//	if(mensaje.escribir) serverUDPRecibeFichero(int s, mensaje.Nombrefichero, struct sockaddr_in clientaddr_in);
//CLIENTE
//  lee(argumentos);
//	if(argumento==leer) clientUDPRecibeFichero(int s, mensaje.Nombrefichero,  char * mode, struct sockaddr_in clientaddr_in);
//	if(argumento==escribir) clientUDPEnviaFichero(int s, char * Nombrefichero,  char * mode, struct sockaddr_in clientaddr_in);
	return 0;
}


void serverUDPEnviaFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in)
{
  int i,n=0,fin=0; 
	int cc;				    /* contains the number of bytes read */
	char * datos;
	char * datosFichero; 
	char * UltimosdatosFichero; 
  char * h;
  char asentimiento[4];
	int addrlen;  
	int tamanno;
	int numPaquetes;
	int restoPaquete;
  FILE * fichero;
  addrlen = sizeof(struct sockaddr_in);

  fichero = fopen(Nombrefichero,"r");
  if(fichero==NULL){
		h = ErrorMsg(1, "No se ha encontrado el fichero el fichero");
		sendto (s, &h, 2+2+strlen("No se ha encontrado el fichero el fichero")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("No se ha encontrado el fichero el fichero\n");
		return;
	}

	fseek(fichero, 0L, SEEK_END );
	tamanno=ftell(fichero);
	numPaquetes=tamanno/512;
	restoPaquete=tamanno%512;

  rewind(fichero);
  datos = malloc(sizeof(char));
	datosFichero = malloc(sizeof(char)*512);
	if(restoPaquete!=0) UltimosdatosFichero = malloc(sizeof(char)*restoPaquete);
	else UltimosdatosFichero = malloc(sizeof(char)*1);

  if(datosFichero==NULL){
		h = ErrorMsg(0, "Error al hacer el malloc");
		sendto (s, &h, 2+2+strlen("Error al hacer el malloc")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
 		printf("Error al hacer el malloc\n");
  	return;
	}
  while(fin!=2){
		n++;
  	if(n<=numPaquetes){
			fread(datosFichero, 512,1,fichero);	
			h = DATAPacket(n,datosFichero);
		}	
  	else{
			fin=1;
			if(restoPaquete!=0) fread(UltimosdatosFichero, restoPaquete,1,fichero);		
			else UltimosdatosFichero[0]=0;
			h = DATAPacket(n,UltimosdatosFichero);
		}
  
		sendto (s, &h, 2+2+getDataLength(h),0, (struct sockaddr *)&clientaddr_in, addrlen);	
		cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
	  if(cc == -1){
		  h = ErrorMsg(0, "Error al recibir un mensaje");
			sendto (s, &h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		  }
    if(getPacketType(asentimiento)!=4 && getPacketNumber(asentimiento)!=n){
		  h = ErrorMsg(0, "Numero asentimiento/paquete incorrecto");
			sendto (s, &h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		  }
		if(fin==1) fin=2;
  }
  fclose (fichero);
}



void serverUDPRecibeFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in)
{
  int n=0,fin=0; 
	int cc;				    /* contains the number of bytes read */
  char * h;
  char parteFichero[TAM_DATOS];
	FILE * fichero;
	int addrlen;  
  addrlen = sizeof(struct sockaddr_in);

	fichero = fopen(Nombrefichero,"r");
  if(fichero!=NULL){
		h = ErrorMsg(6, "El fichero ya existe");
		sendto (s, &h, 2+2+strlen("El fichero ya existe")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("El fichero ya existe\n");
		return;
	}
  fichero = fopen(Nombrefichero,"w");
	h = ACK(0);
	sendto (s, &h, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);	

  while(fin==2){
		n++;
		cc = recvfrom (s, parteFichero, TAM_DATOS,0,(struct sockaddr *)&clientaddr_in, &addrlen);

	  if(cc == -1){
		  h = ErrorMsg(0, "Error al recibir un mensaje");
			sendto (s, &h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		  }
		if(getPacketType(parteFichero)!=3 && getPacketNumber(parteFichero)!=n){
		  h = ErrorMsg(0, "Numero asentimiento/paquete incorrecto");
			sendto (s, &h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		}
 
		fwrite(parteFichero, sizeof(char), TAM_DATOS, fichero ); 
    h = ACK(n);
	  sendto (s, &h, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);	

    if(getDataLength(parteFichero)<512) fin=2;
  }
  fclose (fichero);
}



void clientUDPEnviaFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in)
{
  int i,n=0,fin=0; 
	int cc;				    /* contains the number of bytes read */
	char * datosFichero;
	char * UltimosdatosFichero;  
  char * h;
  char asentimiento[4];
	int addrlen;  
	int tamanno;
	int numPaquetes;
	int restoPaquete;
  FILE * fichero;
  addrlen = sizeof(struct sockaddr_in);

  fichero = fopen(Nombrefichero,"r");
  if(fichero==NULL){
		h = ErrorMsg(1, "No se ha encontrado el fichero el fichero");
		sendto (s, &h, 2+2+strlen("No se ha encontrado el fichero el fichero")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("No se ha encontrado el fichero el fichero\n");
		return;
	}

	h = WRQ(Nombrefichero, mode);
	sendto (s, &h, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
	cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);

	if(cc == -1){
		h = ErrorMsg(0, "Error al recibir un mensaje");
		sendto (s, &h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		return;
	}
  if(getPacketType(asentimiento)!=4 && getPacketNumber(asentimiento)!=n){
		h = ErrorMsg(0, "Numero asentimiento/paquete incorrecto");
		sendto (s, &h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		return;
	}	

	fseek(fichero, 0L, SEEK_END );
	tamanno=ftell(fichero);
	numPaquetes=tamanno/512;
	restoPaquete=tamanno%512;

  rewind(fichero);
	datosFichero = malloc(sizeof(char)*512);
	if(restoPaquete!=0) UltimosdatosFichero = malloc(sizeof(char)*restoPaquete);
	else UltimosdatosFichero = malloc(sizeof(char)*1);
 
  if(datosFichero==NULL){
		h = ErrorMsg(0, "Error al hacer el malloc");
	  sendto (s, &h, 2+2+strlen("Error al hacer el malloc")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
 		printf("Error al hacer el malloc\n");
  	return;
	}
  while(fin!=2){
		n++;

  	if(n<=numPaquetes){
			fread(datosFichero, 512,1,fichero);	
			h = DATAPacket(n,datosFichero);
		}	
  	else{
			fin=1;
			if(restoPaquete!=0) fread(UltimosdatosFichero, restoPaquete,1,fichero);		
			else UltimosdatosFichero[0]=0;
			h = DATAPacket(n,UltimosdatosFichero);
		}

	  sendto (s, &h, 2+2+getDataLength(h),0, (struct sockaddr *)&clientaddr_in, addrlen);	
		cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
	  if(cc == -1){
		  h = ErrorMsg(0, "Error al recibir un mensaje");
			sendto (s, &h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		  }
    if(getPacketType(asentimiento)!=4 && getPacketNumber(asentimiento)!=n){
		  h = ErrorMsg(0, "Numero asentimiento/paquete incorrecto");
			sendto (s, &h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		 }
		if(fin==1) fin=2;
  }
  fclose (fichero);
}



void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in)
{
	int n=0,fin=0; 
	int cc;				    /* contains the number of bytes read */
  char * h;
	char asentimiento[4];
  char parteFichero[TAM_DATOS];
	FILE * fichero;
	int addrlen;  
  addrlen = sizeof(struct sockaddr_in);

	fichero = fopen(Nombrefichero,"r");
  if(fichero!=NULL){
		h = ErrorMsg(6, "El fichero ya existe");
		sendto (s, &h, 2+2+strlen("El fichero ya existe")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("El fichero ya existe\n");
		return;
	}
  fichero = fopen(Nombrefichero,"w");

  h = RRQ(Nombrefichero, mode);
	sendto (s, &h, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	

  while(fin==2){
		n++;
		cc = recvfrom (s, parteFichero, TAM_DATOS,0,(struct sockaddr *)&clientaddr_in, &addrlen);

	  if(cc == -1){
		  h = ErrorMsg(0, "Error al recibir un mensaje");
			sendto (s, &h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		  }
		if(getPacketType(parteFichero)!=3 && getPacketNumber(parteFichero)!=n){
		  h = ErrorMsg(0, "Numero asentimiento/paquete incorrecto");
			sendto (s, &h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			return;
		}
 
		fwrite(parteFichero, sizeof(char), TAM_DATOS, fichero ); 
    h = ACK(n);
	  sendto (s, &h, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);	

    if(getDataLength(parteFichero)<512) fin=2;
  }
  fclose (fichero);
}

