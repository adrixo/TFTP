/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.
 *
 */
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


#define VERBOSE 1

#define PUERTO 41921
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define TCP	1
#define UDP	2

extern int errno;

#include "tftp_messageHandler.c"
#include "utils.c"


void serverEnviaFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in, int tipo);
void serverRecibeFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in, int tipo);

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */

void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar()
{
  printf("Programa Terminado SIGTERM\n");
  FIN = 1;
}

void handler()
{
 printf("Alarma recibida \n");
}

int main(argc, argv)
int argc;
char *argv[];
{

  int s_TCP, s_UDP;		/* connected socket descriptor */
  int ls_TCP;				/* listen socket descriptor */

  int cc;				    /* contains the number of bytes read */

  struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
/*
  struct sigaction {
       void     (*sa_handler)(int);
       void     (*sa_sigaction)(int, siginfo_t *, void *);
       sigset_t   sa_mask;
       int        sa_flags;
       void     (*sa_restorer)(void);
   };
 */

  struct sockaddr_in myaddr_in;	/* for local socket address */
  struct sockaddr_in clientaddr_in;	/* for peer socket address */
  /*
  /usr/include/netinet/in.h
  struct sockaddr_in {
      sa_family_t     sin_family;
      in_port_t       sin_port;
      struct  in_addr sin_addr;
      char            sin_zero[8];
    }
  */
  int addrlen;

  fd_set readmask;
  int numfds,s_mayor;

  char buffer[BUFFERSIZE];	/* buffer for packets to be read into */

  struct sigaction vec,vec2;

  /* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}


	/* clear out address structures */
  memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

  addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}


		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}


	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
  }

	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
  }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror(" sigaction(SIGCHLD)");
      fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
      exit(1);
    }

		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
    vec.sa_handler = (void *) finalizar;
    vec.sa_flags = 0;
    if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
      perror(" sigaction(SIGTERM)");
      fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
      exit(1);
    }

    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
      perror(" sigaction(SIGALRM)");
      fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
      exit(1);
    }
    //if(VERBOSE) printf("Sigactions...\n");

		while (!FIN) {
      /* Meter en el conjunto de sockets los sockets UDP y TCP */
      FD_ZERO(&readmask);
      FD_SET(ls_TCP, &readmask);
      FD_SET(s_UDP, &readmask);

      /*
      Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
      el conjunto de sockets (readmask)
      */
      if (ls_TCP > s_UDP) s_mayor=ls_TCP;
      else s_mayor=s_UDP;

      numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL);

      if ( numfds  < 0) {
        if (errno == EINTR) {
          FIN=1;
          close (ls_TCP);
          close (s_UDP);
          perror("\nFinalizando el servidor. Senal recibida en elect\n ");
        }
      }
      else {

        /* Comprobamos si el socket seleccionado es el socket TCP */
        if (FD_ISSET(ls_TCP, &readmask)) {
            /* Note that addrlen is passed as a pointer
            * so that the accept call can return the
            * size of the returned address.
            */
    			/* This call will block until a new
    			 * connection arrives.  Then, it will
    			 * return the address of the connecting
    			 * peer, and a new socket descriptor, s,
    			 * for that connection.
    			 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
      			case -1:	/* Can't fork, just exit. */
      				exit(1);
      			case 0:		/* Child process comes here. */
              close(ls_TCP); /* Close the listen socket inherited from the daemon. */
      				serverTCP(s_TCP, clientaddr_in);
      				exit(0);
      			default:	/* Daemon process comes here. */
      					/* The daemon needs to remember
      					 * to close the new accept socket
      					 * after forking the child.  This
      					 * prevents the daemon from running
      					 * out of file descriptor space.  It
      					 * also means that when the server
      					 * closes the socket, that it will
      					 * allow the socket to be destroyed
      					 * since it will be the last close.
      					 */
      				close(s_TCP);
      		}
        } /* De TCP*/
        //exit(1);

        /* Comprobamos si el socket seleccionado es el socket UDP */
        if (FD_ISSET(s_UDP, &readmask)) {

    			switch (fork()) {
      			case -1:	/* Can't fork, just exit. */
      				exit(1);
      			case 0:		/* Child process comes here. */
              /* This call will block until a new
              * request arrives.  Then, it will
              * return the address of the client,
              * and a buffer containing its request.
              * BUFFERSIZE - 1 bytes are read so that
              * room is left at the end of the buffer
              * for a null character.
              */
              cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
                 (struct sockaddr *)&clientaddr_in, &addrlen);
              if ( cc == -1) {
                perror(argv[0]);
                printf("%s: recvfrom error\n", argv[0]);
                sendErrorMSG_UDP(s_UDP, clientaddr_in, NODEFINIDO, "Mensaje mal entregado");
                exit (1);
                }
              /* Make sure the message received is
              * null terminated.
              */
              buffer[cc]='\0';

              //Cambiamos el puerto para que no interfieran los paquetes entre procesos
              int newSocket;
              	/* Create the socket UDP. */
            	newSocket = socket (AF_INET, SOCK_DGRAM, 0);
            	if (newSocket == -1) {
            		perror(argv[0]);
            		printf("%s: unable to create socket on child.\n", argv[0]);
            		exit(1);
              }

              myaddr_in.sin_port = htons(0);

            	/* Bind the server's address to the socket. */
            	if (bind(newSocket, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
            		perror(argv[0]);
            		printf("%s: unable to bind address UDP\n", argv[0]);
            		exit(1);
              }
              serverUDP (newSocket, buffer, clientaddr_in);

              close(newSocket);
      				exit(0);

      			default:
              if(VERBOSE) printf("Creando petición UDP.\n");
      		}
        }
      }
		}   /* Fin del bucle infinito de atenci�n a clientes */

    /* Cerramos los sockets UDP y TCP */
    close(ls_TCP);
    close(s_UDP);

    printf("\nFin de programa servidor!\n");

	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
  int reqcnt = 0;		/* keeps count of number of requests */
  char buf[BUFFERSIZE];		/* This example uses TAM_BUFFER byte messages. */
  char hostname[MAXHOST];		/* remote host's name string */
	char *nombreFichero;
  int len, len1, status;
  struct hostent *hp;		/* pointer to host info for remote host */
  long timevar;			/* contains time returned by time() */

  struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

  status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
  if(status){
    /* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
    if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
      perror(" inet_ntop \n");
  }
  /* Log a startup message. */
  time (&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
  if(VERBOSE) printf("Startup from %s port %u at %s",
    hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
  linger.l_onoff  =1;
  linger.l_linger =1;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
    errout(hostname);
  }


  while (len = recv(s, buf, BUFFERSIZE, 0)) {
    if (len == -1) errout(hostname); /* error from recv */
	if(getPacketType(buf)==1){
    addToLog("Iniciando peticion escritura-> ", hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", ntohs(clientaddr_in.sin_port));
    nombreFichero = getFilename(buf);
    addFileTransferInfoToLog(getPacketType(buf), nombreFichero, inet_ntoa(clientaddr_in.sin_addr));
    serverRecibeFichero(s,getFilename(buf), clientaddr_in, TCP);
  }
  if(getPacketType(buf)==2){
    addToLog("Iniciando peticion lectura-> ", hostname, inet_ntoa(clientaddr_in.sin_addr), "TCP", ntohs(clientaddr_in.sin_port));
    nombreFichero = getFilename(buf);
    addFileTransferInfoToLog(getPacketType(buf), nombreFichero, inet_ntoa(clientaddr_in.sin_addr));
    serverEnviaFichero(s, getFilename(buf), clientaddr_in, TCP);
  }
  if(getPacketType(buf)==5){
    printError(getErrorCode(buf));
  }
  else {
    sendErrorMSG_TCP(s, OPERACIONILEGAL, "Operacion no permitida, solo WRQ RRQ");
  }
  		/* Increment the request count. */
    reqcnt++;
  		/* This sleep simulates the processing of the
  		 * request that a real server might do.
  		 */
  }


  close(s);

	/* Log a finishing message. */
  time (&timevar);

  if(VERBOSE) printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
// Venimos de la llamada: serverUDP (s_UDP, buffer, clientaddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
  //if(VERBOSE) printf("Starting serverUDP on socket: %d \n", s);

  int nc, status;
	int addrlen;
  char *nombreFichero;

  char hostname[MAXHOST];

  addrlen = sizeof(struct sockaddr_in);

  status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
  if(status){
    if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
      perror(" inet_ntop \n");
  }

  if(getPacketType(buffer)==1){
    inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST);
    addToLog("Iniciando peticion escritura-> ", hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", ntohs(clientaddr_in.sin_port));
    nombreFichero = getFilename(buffer);
    addFileTransferInfoToLog(getPacketType(buffer), nombreFichero, inet_ntoa(clientaddr_in.sin_addr));
    serverRecibeFichero(s,getFilename(buffer), clientaddr_in, UDP);
  }
  if(getPacketType(buffer)==2){
    addToLog("Iniciando peticion lectura-> ", hostname, inet_ntoa(clientaddr_in.sin_addr), "UDP", ntohs(clientaddr_in.sin_port));
    nombreFichero = getFilename(buffer);
    addFileTransferInfoToLog(getPacketType(buffer), nombreFichero, inet_ntoa(clientaddr_in.sin_addr));
    serverEnviaFichero(s, getFilename(buffer), clientaddr_in, UDP);
  }
  if(getPacketType(buffer)==5){
    printError(getErrorCode(buffer));
  }
  else {
    sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Operacion no permitida, solo WRQ RRQ");
  }
}


void serverEnviaFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in, int tipo)
{
  int i, packetNumber=0,fin=0;
  int cc;				    /* contains the number of bytes read */

  int reenviar = 0;
  int retries = 0;

  int datosAEnviar = PACKETSIZE;

  char * datosFichero;
  char * ultimosDatosFichero;
  char * packet;

  char asentimiento[4];

  int addrlen;
  int tamanno;
  int numPaquetes;
  int restoPaquete;

  FILE * fichero;

  addrlen = sizeof(struct sockaddr_in);

  char rutaFichero[25] = "ficherosTFTPserver/";
  strcat(rutaFichero,Nombrefichero);


  if(VERBOSE) printf("Enviando fichero %s...\n", Nombrefichero);
  fichero = fopen(rutaFichero,"rb");
  if(fichero==NULL){
    if(VERBOSE) printf("No se ha encontrado el fichero.\n");
    if(tipo==UDP) sendErrorMSG_UDP(s, clientaddr_in, FICHERONOENCONTRADO, "No se ha encontrado el fichero");
		if(tipo==TCP) sendErrorMSG_TCP(s, FICHERONOENCONTRADO, "No se ha encontrado el fichero");
    addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "No se ha enconrtado el fichero");
    return;
  }

  fseek(fichero, 0L, SEEK_END );
  tamanno=ftell(fichero);
  numPaquetes=tamanno/PACKETSIZE;
  restoPaquete=tamanno%PACKETSIZE;

  rewind(fichero);
  datosFichero = calloc(PACKETSIZE,sizeof(char));

  if(restoPaquete!=0)
    ultimosDatosFichero = calloc(restoPaquete,sizeof(char));
  else
    ultimosDatosFichero = calloc(1,sizeof(char));

  if(datosFichero==NULL || ultimosDatosFichero==NULL){
    if(VERBOSE) printf("Error al hacer el calloc.\n");
    if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al hacer el calloc");
		if(tipo==TCP)	sendErrorMSG_TCP(s, NODEFINIDO, "Error al hacer el calloc");
    addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Error no definido durante la transmision (calloc)");
    fclose (fichero);
    return;
  }

  while(fin!=2){
    if(reenviar == 0){
      packetNumber++;

      if(packetNumber<=numPaquetes){
        fread(datosFichero, PACKETSIZE,1,fichero);
        packet = DATAPacket(packetNumber,datosFichero);
      }
      else {
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

    if(tipo==UDP)	sendto (s, packet, 4+datosAEnviar,0, (struct sockaddr *)&clientaddr_in, addrlen);
		if(tipo==TCP)	send (s, packet, 4+datosAEnviar,0);

    if(tipo==UDP)	alarm(TIMEOUT*2);
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
			if(tipo==TCP){
				if(VERBOSE) printf("Error al recibir un mensaje\n");
        sendErrorMSG_TCP(s, NODEFINIDO, "Error al recibir un mensaje\n");
			}
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Error al recibir un mensaje\n");
      fclose (fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }
    if(tipo==UDP) alarm(0);

    //Para comprobar el timeout
    //if(packetNumber == 5){ return;}

    if(getPacketType(asentimiento)==5){
      //printErrorMsg(asentimiento);
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), getErrorMsg(asentimiento));
      fclose(fichero);
      return;
    }

    if(getPacketType(asentimiento)!=4){
      if(VERBOSE) printf("Se esperaba ack");
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Se esperaba ack");
      if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Se esperaba ack");
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Se esperaba ack");
      fclose(fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }

    if(VERBOSE) printf("ACK paquete: %d, packetNumber: %d\n", getPacketNumber(asentimiento),packetNumber);
    if(getPacketNumber(asentimiento)!=packetNumber){
      if(VERBOSE) printf("Numero asentimiento incorrecto");
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Numero asentimiento incorrecto");
      if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Numero asentimiento incorrecto");
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Numero asentimiento incorrecto");
      fclose(fichero);
      free(datosFichero);
      free(ultimosDatosFichero);
      return;
    }
    if(fin==1)
    fin=2;
  }

  if(VERBOSE) printf("Envio concluido\n");
  addEndFileTransferInfoToLog(2, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "nada");

  fclose (fichero);
  free(datosFichero);
  free(ultimosDatosFichero);

  return;
}



void serverRecibeFichero(int s, char * Nombrefichero, struct sockaddr_in clientaddr_in, int tipo)
{
  int packetNumber=0,fin=0;
  int cc;				    /* contains the number of bytes read */
  int retries = 0;
  int reenviar = 0;

  char * packet;
  char parteFichero[PACKETSIZE+4];

  FILE * fichero;

  int addrlen;
  addrlen = sizeof(struct sockaddr_in);

  char rutaFichero[25] = "ficherosTFTPserver/";
  strcat(rutaFichero,Nombrefichero);

  if(VERBOSE) printf("Recibiendo fichero %s...\n", Nombrefichero);
  fichero = fopen(rutaFichero,"rb");
  if(fichero!=NULL){
    if(VERBOSE) printf("Error: El fichero ya existe.\n");
    if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, FICHEROYANOEXISTE, "El fichero ya existe");
		if(tipo==TCP)	sendErrorMSG_TCP(s, FICHEROYANOEXISTE, "El fichero ya existe");
    addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "El fichero ya existe");
    fclose(fichero);
    return;
  }

  fichero = fopen(rutaFichero,"wb");
  packet = ACK(0);

  if(tipo==UDP)	sendto (s, packet, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);
	if(tipo==TCP)	send (s, packet, 4,0);

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
			if(tipo==TCP){
      	if (errno == EINTR) {
  				/* Alarm went off and aborted the receive.
  				 * Need to retry the request if we have
  				 * not already exceeded the retry limit.
  				 */
        	if(VERBOSE) printf("Timeout vencido: No se recibio paquete.\n");
          if(retries<5){
            sendto (s, packet, 4,0, (struct sockaddr *)&clientaddr_in, addrlen); //reenviamos el ultimo ack
            reenviar = 1;
            retries++;
            if(VERBOSE) printf("Intento n %d.\n", retries);
            continue;
          }
        	else
        	 sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Timeout: No se recibio paquete\n");
      	}
      	else {
        	if(VERBOSE) printf("Error al recibir un mensaje\n");
        	if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, NODEFINIDO, "Error al recibir un mensaje\n");
      	}
			 }
			if(tipo==TCP){
				if(VERBOSE) printf("Error al recibir un mensaje\n");
        sendErrorMSG_TCP(s, NODEFINIDO, "Error al recibir un mensaje\n");
			}
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Error al recibir un mensaje\n");
      fclose(fichero);
      return;
    }
    alarm(0);

    //Para comprobar el timeout
    //if(packetNumber==5) { sleep(7); return;}

    if(getPacketType(parteFichero)==5){
     // printErrorMsg(parteFichero);
     addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), getErrorMsg(parteFichero));
      fclose(fichero);
      return;
    }

    if(VERBOSE) printf("Recibiendo paquete %d...\n", packetNumber);

    if(getPacketType(parteFichero)!=3){
      if(VERBOSE) printf("Se esperaba paquete: %d\n",getPacketType(parteFichero));
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Se esperaba paquete");
			if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Se esperaba paquete");
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Error en la transmision, no se recibio correctamente un paquete de datos");
      fclose(fichero);
      return;
    }

    if(getPacketNumber(parteFichero)!=packetNumber){
      if(VERBOSE) printf("Numero asentimiento incorrecto: %d\n",getPacketNumber(parteFichero));
      if(tipo==UDP)	sendErrorMSG_UDP(s, clientaddr_in, OPERACIONILEGAL, "Numero asentimiento incorrecto");
			if(tipo==TCP)	sendErrorMSG_TCP(s, OPERACIONILEGAL, "Numero asentimiento incorrecto");
      addEndFileTransferInfoToLog(5, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "Numero asentimiento incorrecto");
      fclose(fichero);
      return;
    }

    fwrite(getDataMSG(parteFichero, cc-4), cc-4, 1, fichero);
    packet = ACK(packetNumber);
    if(tipo==UDP)	sendto (s, packet, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);
		if(tipo==TCP)	send (s, packet, 4,0);

    if(cc-4<PACKETSIZE)
      fin=2;
  }
  if(VERBOSE) printf("Fichero recibido.\n");
  addEndFileTransferInfoToLog(1, Nombrefichero,  inet_ntoa(clientaddr_in.sin_addr), "nada");

  fclose (fichero);
  return;
}
