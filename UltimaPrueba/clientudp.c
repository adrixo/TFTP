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
#include "tftp_messageHandler.c"



extern int errno;

#define ADDRNOTFOUND	0xffffffff	/* value returned for unknown host */
#define RETRIES	5		/* number of times to retry before givin up */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define PUERTO 17278
#define TIMEOUT 6
#define MAXHOST 512
/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void clientUDPEnviaFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in);
void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in);

void handler()
{
 printf("Alarma recibida \n");
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

	if (argc != 3) {
		fprintf(stderr, "Usage:  %s <nameserver> <target>\n", argv[0]);
		exit(1);
	}
	
		/* Create the socket. */
	s = socket (AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	
    /* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	
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

   /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
    vec.sa_handler = (void *) handler;
    vec.sa_flags = 0;
    if ( sigaction(SIGALRM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGALRM)");
            fprintf(stderr,"%s: unable to register the SIGALRM signal\n", argv[0]);
            exit(1);
        }
	
    n_retry=RETRIES;
    
	clientUDPEnviaFichero(s, argv[2], "octec", servaddr_in);
	exit(0);
	//while (n_retry > 0) {
		/* Send the request to the nameserver. */
				
     /*   if (sendto (s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in,
				sizeof(struct sockaddr_in)) == -1) {
        		perror(argv[0]);
        		fprintf(stderr, "%s: unable to send request\n", argv[0]);
        		exit(1);
        	}
		/* Set up a timeout so I don't hang in case the packet
		 * gets lost.  After all, UDP does not guarantee
		 * delivery.
		 */
	  //  alarm(TIMEOUT);
		/* Wait for the reply to come in. */
    /*    if (recvfrom (s, &reqaddr, sizeof(struct in_addr), 0,
						(struct sockaddr *)&servaddr_in, &addrlen) == -1) {
    		if (errno == EINTR) {
    				/* Alarm went off and aborted the receive.
    				 * Need to retry the request if we have
    				 * not already exceeded the retry limit.
    				 */
 		/*         printf("attempt %d (retries %d).\n", n_retry, RETRIES);
  	 		     n_retry--; 
                    } 
            else  {
				printf("Unable to get response from");
				exit(1); 
                }
              } 
        else {
            alarm(0);
            /* Print out response. */
       /*     if (reqaddr.s_addr == ADDRNOTFOUND) 
               printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
            else {
                /* inet_ntop para interoperatividad con IPv6 */
     /*           if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
                   perror(" inet_ntop \n");
                printf("Address for %s is %s\n", argv[2], hostname);
                }	
            break;	
            }
  }*/

    if (n_retry == 0) {
       printf("Unable to get response from");
       printf(" %s after %d attempts.\n", argv[1], RETRIES);
       }
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
	char rutaFichero[25] = "ficherosTFTPclient/"; 
	strcat(rutaFichero,Nombrefichero);

  fichero = fopen(rutaFichero,"r");
  if(fichero==NULL){
		h = ErrorMsg(FICHERONOENCONTRADO, "No se ha encontrado el fichero el fichero");
		sendto (s, h, 2+2+strlen("No se ha encontrado el fichero el fichero")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("No se ha encontrado el fichero el fichero\n");
		return;
	}

	h = WRQ(Nombrefichero, mode);
	sendto (s, h, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
	cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
	printMSG(asentimiento);

	if(cc == -1){
		h = ErrorMsg(NODEFINIDO, "Error al recibir un mensaje");
		sendto (s, h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		fclose(fichero);
		return;
	}
  if(getPacketType(asentimiento)!=4 || getPacketNumber(asentimiento)!=n){
		h = ErrorMsg(NODEFINIDO, "Numero asentimiento/paquete incorrecto");
		sendto (s, h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		fclose(fichero);
		return;
	}	

	fseek(fichero, 0L, SEEK_END );
	tamanno=ftell(fichero);
	numPaquetes=tamanno/512;
	restoPaquete=tamanno%512;

  rewind(fichero);
	datosFichero = calloc(512,sizeof(char));
	if(restoPaquete!=0) UltimosdatosFichero = calloc(restoPaquete,sizeof(char));
	else UltimosdatosFichero = calloc(1,sizeof(char));
 
  if(datosFichero==NULL || UltimosdatosFichero==NULL){
		h = ErrorMsg(NODEFINIDO, "Error al hacer el calloc");
	  sendto (s, h, 2+2+strlen("Error al hacer el calloc")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
 		printf("Error al hacer el calloc\n");
		fclose(fichero);
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

	  sendto (s, h, 2+2+512,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		cc = recvfrom (s, asentimiento, 4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
		printMSG(asentimiento);
	  if(cc == -1){
		  h = ErrorMsg(NODEFINIDO, "Error al recibir un mensaje");
			sendto (s, h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			fclose(fichero);
			free(datosFichero);
			free(UltimosdatosFichero);
			return;
		  }
    if(getPacketType(asentimiento)!=4 || getPacketNumber(asentimiento)!=n){
		  h = ErrorMsg(NODEFINIDO, "Numero asentimiento/paquete incorrecto");
			sendto (s, h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
			fclose(fichero);
			free(datosFichero);
			free(UltimosdatosFichero);
			return;
		 }
		if(fin==1) fin=2;
  }
  fclose (fichero);
//	free(datosFichero);
//	free(UltimosdatosFichero);
	return;
}



void clientUDPRecibeFichero(int s, char * Nombrefichero, char * mode, struct sockaddr_in clientaddr_in)
{
	int n=0,fin=0; 
	int cc;				    /* contains the number of bytes read */
  char * h;
  char parteFichero[PACKETSIZE+4];
	FILE * fichero;
	int addrlen;  
  addrlen = sizeof(struct sockaddr_in);
	char rutaFichero[25] = "ficherosTFTPclient/"; 
	strcat(rutaFichero,Nombrefichero);

	fichero = fopen(rutaFichero,"r");
  if(fichero!=NULL){
		h = ErrorMsg(FICHEROYANOEXISTE, "El fichero ya existe");
		sendto (s, h, 2+2+strlen("El fichero ya existe")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
		printf("El fichero ya existe\n");
		return;
	}

  fichero = fopen(rutaFichero,"w");
  h = RRQ(Nombrefichero, mode);

  sendto (s, h, 2+strlen(Nombrefichero)+1+strlen(mode)+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	

  while(fin!=2){
		n++;
		cc = recvfrom (s, parteFichero, PACKETSIZE+4,0,(struct sockaddr *)&clientaddr_in, &addrlen);
		printMSG(parteFichero);

	  if(cc == -1){
		  h = ErrorMsg(NODEFINIDO, "Error al recibir un mensaje");
			sendto (s, h, 2+2+strlen("Error al recibir un mensaje")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
  		fclose (fichero); 
			return;
		  }
		if(getPacketType(parteFichero)!=3 || getPacketNumber(parteFichero)!=n){
		  h = ErrorMsg(NODEFINIDO, "Numero asentimiento/paquete incorrecto");
			sendto (s, h, 2+2+strlen("Numero asentimiento/paquete incorrecto")+1,0, (struct sockaddr *)&clientaddr_in, addrlen);	
  		fclose (fichero);
			return;
		}

		fwrite(getDataMSG(parteFichero), getDataLength(parteFichero), 1, fichero ); 
		
    h = ACK(n);
	  sendto (s, h, 4,0, (struct sockaddr *)&clientaddr_in, addrlen);	
    if(getDataLength(parteFichero)<512) fin=2;
	}
  fclose (fichero);

	return;
}



