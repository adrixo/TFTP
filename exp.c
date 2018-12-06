
main:
  int s_TCP, s_UDP - sockets descriptor
  int ls_TCP - listen socket descriptor

  int cc - number of bytes read

  struct sigaction sa = {.sa_handler = SIG_IGN};
        struct sigaction {
             void     (*sa_handler)(int);
             void     (*sa_sigaction)(int, siginfo_t *, void *);
             sigset_t   sa_mask;
             int        sa_flags;
             void     (*sa_restorer)(void);
         };

  struct sockaddr_in myaddr_in;
  struct sockaddr_in clientaddr_in;
        /usr/include/netinet/in.h
        struct sockaddr_in {
            sa_family_t     sin_family;   AF_INET
            in_port_t       sin_port;     Port number
            struct  in_addr sin_addr;     IP address
            char            sin_zero[8];
        }

        in_addr
          es un struct union en el que se puede meter/extraer la ip de distintos modos

  int addrlen;

  fd_set readmask;
  int numfds,s_mayor;

  char buffer[BUFFERSIZE];	/* buffer for packets to be read into */

  struct sigaction vec;


1. create the listen socket:
  ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
    man socket
      Domain: selects the protocol family
      Type: specifies the communication semantics
        -SOCK_STREAM Provides sequenced, reliable, two-way, connection-based
        byte streams.  An out-of-band data transmission mechanism may be supported.
      Protocol: particular protocol to be used with the sock

2. introducimos los datos en myaddr_in

3. bind the listen address to the socket
  bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in));
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
      enlaza sockfd con addr
      addrlen especifica el tamaño en bytes de addr

4. listen
  listen(ls_TCP, 5)
    int listen(int sockfd, int backlog);
      marca el sockfd como un socket pasivo, uno que se usa para aceptar peticiones de conexion usando accept
      backlog define la longitud maxima a la que la cola de conexiones pendientes puede crecer. //en otro caso recibe ECONNREFUSED

//UDP, llamada a socket y a bind
5.
  s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
  bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in))

//inicialization Complete


6. llamada a setpgrp para que el daemon no se asocie a nuestra terminal
  setpgrp()

7. Fork the daemon and
  fork()
  //nos quedamos con el hijo
    7.1 close stdin and stderr
    7.2 registrar sigcld a sig_ign
    7.3 registramos sigterm

8. Dentro del fork se inicia un while hasta que FIN (global) se pone a 0
  por ejemplo con la funcion finalizar asociada al handler de la señal SIGTERM

9. Meter en el conjunto de sockets los sockets udp y tcp
  FD_ZERO(&readmask); //Clears a set
  FD_SET(ls_TCP, &readmask); //Add a given file descriptor
  FD_SET(s_UDP, &readmask);

10.
  numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL);

11. se comprueba si se ha seleccionado TCP o UDP
  if (FD_ISSET(ls_TCP, &readmask)) {
  if (FD_ISSET(s_UDP, &readmask)) {

12. Caso TCP
  12.1 Se llama a una funion bloqueante hasta que una nueva conexión viene
    s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
  12.2 se hace fork, el hijo se encarga de resolver la peticion:
    serverTCP(s_TCP, clientaddr_in);

13. Caso UDP
  13.1 cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
     (struct sockaddr *)&clientaddr_in, &addrlen);
  13.2
    buffer[cc]='\0';
    serverUDP (s_UDP, buffer, clientaddr_in);


14. cerramos ls_tcp y s_udp
  close(ls_TCP);
  close(s_UDP);





SERVERUDP, s_udp, buffer, clientaddr_in (rellenado en recvfrom)
1. rellenamos
  struct in_addr reqaddr;	/* for requested host's address */
  struct hostent *hp;		/* pointer to host info for requested host */
  int nc, errcode;

  struct addrinfo hints, *res;

  int addrlen;

  addrlen = sizeof(struct sockaddr_in);
