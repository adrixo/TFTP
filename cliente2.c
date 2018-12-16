/*
 *			C L I E N T U D P
 *
 *	This is an example program that demonstrates the use of
 *	sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 *	This program will request the internet address of a target
 *	host by name from the serving host.  The serving host
 *	will return the requested internet address as a response,
 *	and will return an address of all ones if it does not recognize
 *	the host name.
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int errno;

#define VERBOSE 1

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 41921
#define MAXHOST 512
#define TAM_BUFFER 10
#define TCP	1
#define UDP	2


#include "tftp_messageHandler.c"
#include "utils.c"


void clienteEnviaFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in, int tipo);
void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in, int tipo);

/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
 printf("Alarma recibida \n");
}

int FIN = 0;             /* Para el cierre ordenado */
void finalizar()
{
  printf("Programa Terminado SIGTERM\n");
  FIN = 1;
}

/*
 *			M A I N
 *
 *	This routine is the client which requests service from the remote
 *	"example server".  It will send a message to the remote nameserver
 *	requesting the internet address corresponding to a given hostname.
 *	The server will look up the name, and return its internet address.
 *	The returned address will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as the first parameter to the command.  The second parameter should
 *	be the the name of the target host for which the internet address
 *	is sought.
 */
int main(argc, argv)
int argc;
char *argv[];
{
	int i, errcode;
	int retry = RETRIES;		/* holds the retry count */
  int s;				/* socket descriptor */
  long timevar;                       /* contains time returned by time() */
  struct sockaddr_in myaddr_in;	/* for local socket address */
  struct sockaddr_in servaddr_in;	/* for server socket address */
  struct in_addr reqaddr;		/* for returned internet address */
  int	addrlen, n_retry;
  struct sigaction vec;
  char hostname[MAXHOST];
  struct addrinfo hints, *res;
  int tcp = 0, udp = 0;
  int j;
  char buf[TAM_BUFFER];

	if (argc != 5) {
		fprintf(stderr, "Usage:  %s <nameserver> <protocol> <mode> <filename>\n", argv[0]);
		exit(1);
	}

  if(argv[3][0] != 'l' )
    if(argv[3][0] != 'e'){
		  fprintf(stderr, "Mode should be <l> or <e>\n");
		    exit(1);
	  }

  if( strcmp(argv[2], "tcp"))
    if( strcmp(argv[2], "udp")){
      fprintf(stderr, "Protocol should be <tcp> or <udp>\n");
        exit(1);
    }

  if(!strcmp(argv[2], "tcp")){
    tcp = 1;
  }

  if(!strcmp(argv[2], "udp")){
    udp = 1;
  }

  /* Create the socket. */
	if(udp) s = socket (AF_INET, SOCK_DGRAM, 0);
  if(tcp) s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}

    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

//UDP
  if(udp){
  			/* Bind socket to some local address so that the
  		 * server can send the reply back.  A port number
  		 * of zero will be used so that the system will
  		 * assign any available port number.  An address
  		 * of INADDR_ANY will be used so we do not have to
  		 * look up the internet address of the local host.
  		 */
  	myaddr_in.sin_family = AF_INET;
  	myaddr_in.sin_port = 0;
  	myaddr_in.sin_addr.s_addr = INADDR_ANY;
  	if (bind(s, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
  		perror(argv[0]);
  		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
  		exit(1);
  	}
  }

		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
  addrlen = sizeof(struct sockaddr_in);
  if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
    exit(1);
  }

            /* Print out a startup message for the user. */
  time(&timevar);
            /* The port number must be converted first to host byte
             * order before printing.  On most hosts, this is not
             * necessary, but the ntohs() call is included here so
             * that this program could easily be ported to a host
             * that does require it.
             */
  printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));


	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
		/* Get the host information for the server's hostname that the
		 * user passed in.
		 */
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
 	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
  errcode = getaddrinfo (argv[1], NULL, &hints, &res);
  if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
	  fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
		argv[0], argv[1]);
	  exit(1);
  }
  else {
			/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
  freeaddrinfo(res);

  /* puerto del servidor en orden de red*/
  servaddr_in.sin_port = htons(PUERTO);

  if(tcp){
    		/* Try to connect to the remote server at the address
    		 * which was just built into peeraddr.
    		 */
    	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
    		perror(argv[0]);
    		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
    		exit(1);
    	}
  }

    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
  vec.sa_handler = (void *) finalizar;
  vec.sa_flags = 0;
  if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
    perror(" sigaction(SIGTERM)");
    fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
    exit(1);
  }

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
  vec.sa_handler = (void *) handler;
  vec.sa_flags = 0;
  if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
    perror(" sigaction(SIGALRM)");
    fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
    exit(1);
  }

  if(udp){
    n_retry=RETRIES;
    BYTE *msg;
    /* Iniciamos la peticion: */
    //printf("->%c\n", argv[2][0]);
    if(argv[3][0] == 'l'){
      clientUDPRecibeFichero(s, argv[4], "octec", servaddr_in, UDP);
    }
    if(argv[3][0] == 'e'){
      clienteEnviaFichero(s, argv[4], "octec", servaddr_in, UDP);
    }

  }

  else if (tcp){
		if(argv[3][0] == 'l'){
      clientUDPRecibeFichero(s, argv[4], "octec", servaddr_in, TCP);
    }
    if(argv[3][0] == 'e'){
      clienteEnviaFichero(s, argv[4], "octec", servaddr_in, TCP);
    }
  }

      /* Print message indicating completion of task. */
  	time(&timevar);
  	printf("All done at %s", (char *)ctime(&timevar));
  //}
}


void clienteEnviaFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in, int tipo)
{
  int i,packetNumber=0,fin=0;
  int cc;				    /* contains the number of bytes read */
  int retries = 0;
  int reenviar = 0;

  char * datosFichero;
  char * ultimosDatosFichero;
  char * packet;
  char asentimiento[4];
  int datosAEnviar = PACKETSIZE;
  int addrlen;
  int tamanno;
  int numPaquetes;
  int restoPaquete;

  FILE * fichero;

//obtenemos el numero de puerto, en un archivo con ese nombre meteremos el progreso de ejecución del programa
  struct sockaddr_in myaddr;
  int len;
  len = sizeof(len);
  if (getsockname(s, (struct sockaddr *)&myaddr, &len) == -1)
      perror("getsockname");
  /*else
      printf("port number %d\n", ntohs(myaddr.sin_port));*/
  int portNumber = ntohs(myaddr.sin_port);

  addrlen = sizeof(struct sockaddr_in);
  char rutaFichero[25] = "ficherosTFTPcliente/";
  strcat(rutaFichero,Nombrefichero);

  if(VERBOSE)  printf("Enviando fichero %s...\n", Nombrefichero);
  addClientFileTransferInfo(0, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", 0);

  fichero = fopen(rutaFichero,"rb");
  if(fichero==NULL){
    if(VERBOSE)  printf("No se ha encontrado el fichero %s\n", Nombrefichero);
    if(tipo==UDP) sendErrorMSG_UDP(s, clientaddr_in, FICHERONOENCONTRADO, "No se ha encontrado el fichero");
		if(tipo==TCP) sendErrorMSG_TCP(s, FICHERONOENCONTRADO, "No se ha encontrado el fichero");
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "No se ha encontrado el fichero", 0);
    return;
  }

  packet = WRQ(Nombrefichero, mode);
  if(tipo==UDP)	sendto (s, packet, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);
	if(tipo==TCP)	send (s, packet, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0);

  if(tipo==UDP)	alarm(TIMEOUT);
  if(tipo==UDP)	cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
	if(tipo==TCP)	cc = recv (s, asentimiento, 4,0);
  if(cc == -1){
		if(tipo==UDP){
    	if (errno == EINTR) {
        /* Alarm went off and aborted the receive.
         * Need to retry the request if we have
         * not already exceeded the retry limit.
         */
    	  if(VERBOSE) printf("Timeout vencido: No se recibio ACK para empezar a enviar fichero.\n");
    	  sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Timeout: No se recibio ACK\n");
        addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Timeout: No se recibio ACK", 0);
    	}
    	else {
    	  if(VERBOSE) printf("Error al recibir un mensaje\n");
    	  sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al recibir un mensaje\n");
    	}
		}
		if(tipo==TCP)	sendErrorMSG_TCP(s, NODEFINIDO, "Error al recibir un mensaje\n");
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Error al recibir un mensaje", 0);
    fclose (fichero);
    return;
  }
  alarm(0);

  if(getPacketType(asentimiento)==5){
    printErrorMsg(asentimiento);
    fclose(fichero);
    return;
  }

  if(getPacketType(asentimiento)==5){
    printErrorMsg(asentimiento);
    fclose(fichero);
    return;
  }

  if(getPacketType(asentimiento)!=4){
    if(VERBOSE) printf("Se esperaba ack\n");
    if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Se esperaba ack");
		if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Se esperaba ack");
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Se esperaba ack", 0);
    fclose(fichero);
    return;
  }

  if(VERBOSE) printf("ACK paquete: %d\n", getPacketNumber(asentimiento));
  if(getPacketNumber(asentimiento)!=packetNumber){
    if(VERBOSE) printf("Numero asentimiento incorrecto");
    if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Numero asentimiento incorrecto");
		if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Numero asentimiento incorrecto");
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Numero asentimiento incorrecto", 0);
    fclose(fichero);
    return;
  }

  fseek(fichero, 0L, SEEK_END );
  tamanno=ftell(fichero);
  numPaquetes=tamanno/512;
  restoPaquete=tamanno%512;

  rewind(fichero);
  datosFichero = calloc(512,sizeof(char));
  if(restoPaquete!=0)
    ultimosDatosFichero = calloc(restoPaquete,sizeof(char));
  else
    ultimosDatosFichero = calloc(1,sizeof(char));


  if(datosFichero==NULL || ultimosDatosFichero==NULL){
    if(VERBOSE) printf("Error al hacer el calloc.\n");
    if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al hacer el calloc");
		if(tipo==TCP)	sendErrorMSG_TCP(s, NODEFINIDO, "Error al hacer el calloc");
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Error al hacer el calloc", 0);
    fclose (fichero);
    return;
  }

  while(fin!=2){

    if(reenviar == 0)
    {
      packetNumber++;
    	if(packetNumber<=numPaquetes){
        fread(datosFichero, 512,1,fichero);
        packet = DATAPacket(packetNumber,datosFichero);
      }

      else{
        fin=1;
        if(restoPaquete!=0){
          fread(ultimosDatosFichero, restoPaquete,1,fichero);
  				datosAEnviar = restoPaquete;
  			}
        else{
  				ultimosDatosFichero[0]=0;
  				datosAEnviar = 1;
  			}
        packet = DATAPacket(packetNumber,ultimosDatosFichero);
      }
      if(VERBOSE) printf("Enviando paquete %d...\n", packetNumber);
    }
    if(reenviar == 1){
      reenviar = 0;
      if(VERBOSE) printf("Reenviando paquete %d...\n", packetNumber);
    }

    addClientFileTransferInfo(2, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", packetNumber);

    if(tipo==UDP)	sendto (s, packet, 4+datosAEnviar,0, (struct sockaddr *)&clientaddr_in, addrlen);
		if(tipo==TCP)	send (s, packet, 4+datosAEnviar,0);

    if(tipo==UDP)	alarm(TIMEOUT);
		if(tipo==UDP)	cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
		if(tipo==TCP)	cc = recv (s, asentimiento, 4,0);
    if(cc == -1){
			if(tipo==UDP){
     	 if (errno == EINTR) {
  				/* Alarm went off and aborted the receive.
  				 * Need to retry the request if we have
  				 * not already exceeded the retry limit.
  				 */
      	  if(VERBOSE) printf("Timeout vencido: No se recibio ACK.\n");
          addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Timeout vencido: No se recibio ACK", 0);
          if(retries<5){
            reenviar = 1;
            retries++;
            if(VERBOSE) printf("Intento n %d.\n", retries);
            continue;
          }
        	else
            sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Timeout: No se recibio ACK\n");
     	 }
      	else {
      	  if(VERBOSE) printf("Error al recibir un mensaje\n");
      	  sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al recibir un mensaje\n");
     	 }
			}
			if(tipo==TCP) sendErrorMSG_TCP(s, NODEFINIDO, "Error al recibir un mensaje\n");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Error al recibir un mensaje", 0);

      fclose (fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }
    if(tipo==UDP) alarm(0);

    if(getPacketType(asentimiento)==5){
      printErrorMsg(asentimiento);
      fclose(fichero);
      return;
    }

    if(getPacketType(asentimiento)!=4){
      if(VERBOSE) printf("Se esperaba ack\n");
      if(tipo==UDP) sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Se esperaba ack");
			if(tipo==TCP) sendErrorMSG_TCP(s, OPERACIONILEGAL, "Se esperaba ack");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Se esperaba ack", 0);
      fclose(fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }

    if(VERBOSE) printf("ACK paquete: %d\n", getPacketNumber(asentimiento));
    if(getPacketNumber(asentimiento)!=packetNumber){
      if(VERBOSE) printf("Numero asentimiento incorrecto\n");
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Numero asentimiento incorrecto");
			if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Numero asentimiento incorrecto");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Numero asentimiento incorrecto", 0);
      fclose(fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }

    if(fin==1)
      fin=2;
  }

  if(VERBOSE) printf("Envio concluido\n");
  addClientFileTransferInfo(4, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", 0);

  fclose (fichero);
  free(datosFichero);
  free(ultimosDatosFichero);
  return;
}


void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in, int tipo)
{
  int packetNumber=0,fin=0;
  int cc;				  /* contains the number of bytes read */
  int reenviar = 0;

  int retries = 0;

  char * packet;
  char parteFichero[PACKETSIZE+4];

  FILE * fichero;

  int addrlen;
  addrlen = sizeof(struct sockaddr_in);

  //obtenemos el numero de puerto, en un archivo con ese nombre meteremos el progreso de ejecución del programa
  struct sockaddr_in myaddr;
  int len;
  len = sizeof(len);
  if (getsockname(s, (struct sockaddr *)&myaddr, &len) == -1)
      perror("getsockname");
  /*else
      printf("port number %d\n", ntohs(myaddr.sin_port));*/
  int portNumber = ntohs(myaddr.sin_port);

  char rutaFichero[25] = "ficherosTFTPcliente/";
  strcat(rutaFichero,Nombrefichero);

  if(VERBOSE)  printf("Recibiendo fichero %s...\n", Nombrefichero);
  addClientFileTransferInfo(0, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", 0);

    fichero = fopen(rutaFichero,"rb");
  if(fichero!=NULL){
    if(VERBOSE)  printf("El fichero %s ya existe\n", Nombrefichero);
    addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "El fichero ya existe", 0);
    fclose(fichero);
    return;
  }

  fichero = fopen(rutaFichero,"wb");
  packet = RRQ(Nombrefichero, mode);

  if(tipo==UDP)	sendto (s, packet, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);
	if(tipo==TCP)	send (s, packet, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0);

  while(fin!=2){
    if(reenviar == 0){
      packetNumber++;
    }
    if(reenviar == 1){
      reenviar = 0;
    }

    if(tipo==UDP)	alarm(TIMEOUT);
    if(tipo==UDP)	cc = recvfrom (s, parteFichero, PACKETSIZE+4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
		if(tipo==TCP)	cc = recv (s, parteFichero, PACKETSIZE+4,0);
    if(cc == -1){
			if(tipo==UDP){
      	if (errno == EINTR) {
  				/* Alarm went off and aborted the receive.
  				 * Need to retry the request if we have
  				 * not already exceeded the retry limit.
  				 */
        	if(VERBOSE) printf("Timeout vencido: No se recibio paquete.\n");
          addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Timeout vencido: No se recibio paquete", 0);
          if(retries<5){
            sendto (s, packet, 4,0, (struct sockaddr *)&clientaddr_in, addrlen); //reenviamos el ultimo ack
            reenviar = 1;
            retries++;
            if(VERBOSE) printf("Intento n %d.\n", retries);
            continue;
          }
        	else
            sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Timeouts vencidos: No se recibio paquete\n");
            addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Timeouts vencidos: No se recibio paquete", 0);
      	}
      	else {
        	if(VERBOSE) printf("Error al recibir un mensaje\n");
        	sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al recibir un mensaje\n");
      	}
			}
			if(tipo==TCP)	sendErrorMSG_TCP(s, NODEFINIDO, "Error al recibir un mensaje\n");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Error al recibir un mensaje", 0);
      fclose (fichero);
      return;
    }
    if(tipo==UDP)	alarm(0);

    if(getPacketType(parteFichero)==5){
      printErrorMsg(parteFichero);
      fclose(fichero);
      return;
    }
    if(VERBOSE) printf("Recibiendo paquete %d...\n", packetNumber);

    //Para probar alarm()
    //if(packetNumber == 5) return;

    if(getPacketType(parteFichero)!=3){
      if(VERBOSE) printf("Se esperaba paquete: %d\n",getPacketType(parteFichero));
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Se esperaba paquete");
			if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Se esperaba paquete");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Se esperaba paquete", 0);
      fclose(fichero);
      return;
    }

    if(getPacketNumber(parteFichero)!=packetNumber){
      if(VERBOSE) printf("Numero asentimiento incorrecto: %d\n",getPacketNumber(parteFichero));
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Numero asentimiento incorrecto");
			if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Numero asentimiento incorrecto");
      addClientFileTransferInfo(5, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "Numero asentimiento incorrecto", 0);
      fclose(fichero);
      return;
    }

    fwrite(getDataMSG(parteFichero, cc-4), cc-4, 1,fichero);

    addClientFileTransferInfo(1, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", packetNumber);
    packet = ACK(packetNumber);
    if(tipo==UDP)	sendto (s, packet, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);
		if(tipo==TCP)	send (s, packet, 4,0);
    if(cc-4<512)
    fin=2;
  }
  if(VERBOSE) printf("Fichero recibido.\n");
  addClientFileTransferInfo(4, portNumber, Nombrefichero, inet_ntoa(clientaddr_in.sin_addr), "nada", 0);
  fclose(fichero);

  return;

}
