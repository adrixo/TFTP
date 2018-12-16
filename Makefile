kCC = gcc
CFLAGS =
#Solaris
#LIBS = -lsocket -lnsl
#Linux
#LIBS =

PROGS = servidor clientcp clientudp

all: ${PROGS}

servidor: servidor.o
	${CC} ${CFLAGS} -o $@ servidor.o ${LIBS}

servidor2: servidor2.o
	${CC} ${CFLAGS} -o $@ servidor2.o ${LIBS}

servidorSimplificado: servidorSimplificado.o
	${CC} ${CFLAGS} -o $@ servidorSimplificado.o ${LIBS}

clientcp: clientcp.o
	${CC} ${CFLAGS} -o $@ clientcp.o ${LIBS}

clientudp: clientudp.o
	${CC} ${CFLAGS} -o $@ clientudp.o ${LIBS}

cliente: cliente.o
	${CC} ${CFLAGS} -o $@ cliente.o ${LIBS}

cliente2: cliente2.o
	${CC} ${CFLAGS} -o $@ cliente2.o ${LIBS}

clean:
	rm *.o ${PROGS}
