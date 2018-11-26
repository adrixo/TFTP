/*  
 * 	S E R V I D O R
 * Programa 
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



#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024		/* Tamaño del buffer, tamaño máximo de los paquetes */
#define TAM_BUFFER 10
#define MAXHOST 128

extern int errno;
 
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);
int FIN = 0;
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

	int s_TCP, s_UDP;
	int ls_TCP;
    
	int cc;
     
	struct sigaction sa = {.sa_handler = SIG_IGN};
    
	struct sockaddr_in myaddr_in;
	struct sockaddr_in clientaddr_in;
	int addrlen;
	
	fd_set readmask;
	int numfds,s_mayor;
    
	char buffer[BUFFERSIZE];
    
	struct sigaction vec;

	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }

	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

	setpgrp();

	switch (fork()) {
	case -1:
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:
		fclose(stdin);
		fclose(stderr);

		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(1);
  		}
            
		vec.sa_handler = (void *) finalizar;
		vec.sa_flags = 0;
		if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
  			perror(" sigaction(SIGTERM)");
 			fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(1);
		}
        
		while (!FIN) {
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			if (ls_TCP > s_UDP) s_mayor=ls_TCP;
			else s_mayor=s_UDP;

			if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
				if (errno == EINTR) {
					FIN=1;
					close (ls_TCP);
					close (s_UDP);
					perror("\nFinalizando el servidor. SeÃal recibida en elect\n "); 
				}
				}
			else { 
				if (FD_ISSET(ls_TCP, &readmask)) {
		    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
		    			if (s_TCP == -1) exit(1);
		    			switch (fork()) {
						case -1:
							exit(1);
						case 0:
				    			close(ls_TCP);
							serverTCP(s_TCP, clientaddr_in);
							exit(0);
						default:
							close(s_TCP);
						}
		     		}
				if (FD_ISSET(s_UDP, &readmask)) {
					cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
					   (struct sockaddr *)&clientaddr_in, &addrlen);
					if ( cc == -1) {
					    perror(argv[0]);
					    printf("%s: recvfrom error\n", argv[0]);
					    exit (1);
					    }
					buffer[cc]='\0';
					serverUDP (s_UDP, buffer, clientaddr_in);
				}
			}
		}
		close(ls_TCP);
		close(s_UDP);
	    
		printf("\nFin de programa servidor!\n");
        
	default:
		exit(0);
	}
}

void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;	
	char buf[TAM_BUFFER];
	char hostname[MAXHOST];	

	int len, len1, status;
	struct hostent *hp;	
	long timevar;
    
	struct linger linger;
	 
	status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
	if(status){
           	
            if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
            	perror(" inet_ntop \n");
             }
    time (&timevar);
	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}
	while (len = recv(s, buf, TAM_BUFFER, 0)) {
		if (len == -1) errout(hostname); 
		while (len < TAM_BUFFER) {
			len1 = recv(s, &buf[len], TAM_BUFFER-len, 0);
			if (len1 == -1) errout(hostname);
			len += len1;
		}
		reqcnt++;
		sleep(1);
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER) errout(hostname);
	}

	close(s);

	time (&timevar);
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
}

void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
	struct in_addr reqaddr; /**/
	struct hostent *hp; /**/
	int nc, errcode;

	struct addrinfo hints, *res;

	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	
//Tratamiento del mensaje
	errcode = getaddrinfo (buffer, NULL, &hints, &res); 
	printf("%s\n", buffer);

	if (errcode != 0){
		//si no es satisfactorio
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else {
		//
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}

	freeaddrinfo(res);

//enviamos
	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}   
 }
