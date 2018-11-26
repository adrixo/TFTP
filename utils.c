#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char BYTE;

#define NODEFINIDO 0
#define FICHERONOENCONTRADO 1
#define DISCOLLENO 3
#define OPERACIONILEGAL 4
#define FICHEROYANOEXISTE 6

/*
 *                    Write request
 * Returns this byte chain:
 * 2 byte | caracteres     | 1 byte | caracteres | 1 byte
 * _______________________________________________________
 * 01     | nombre fichero | 0      | modo       | 0
 */
BYTE * WRQ(char * filename, char * mode){
	int fileLength = strlen(filename);
	int modeLength = strlen(mode);

	BYTE * header = (BYTE *) calloc( 2 + fileLength + 1 + modeLength + 1, sizeof(BYTE));

// msg type
	header[0] = 0;
	header[1] = 1;

//file name
	for(int i=0; i<fileLength;i++){
		header[2+i] = filename[i];
	}

//0 byte
	header[2+fileLength] = 0;

//mode
	for(int i=0; i<modeLength;i++){
		header[2+fileLength+1+i] = mode[i];
	}

//0 byte
	header[2 + fileLength + 1 + modeLength] = 0;

	for(int i=0; i<2 + fileLength + 1 + modeLength + 1;i++)
		printf(" %d ", header[i]);
	printf("\n");

	return header;
}

/*
 *                      Read request
 * Returns this byte chain:
 * 2 byte | caracteres     | 1 byte | caracteres | 1 byte
 * _______________________________________________________
 *  02    | nombre fichero | 0      | modo       | 0
 */
BYTE * RRQ(char * filename, char * mode){
	int fileLength = strlen(filename);
	int modeLength = strlen(mode);

	BYTE * header = (BYTE *) calloc( 2 + fileLength + 1 + modeLength + 1, sizeof(BYTE));

// msg type
	header[0] = 0;
	header[1] = 2;

//file name
	for(int i=0; i<fileLength;i++){
		header[2+i] = filename[i];
	}

//0 byte
	header[2+fileLength] = 0;

//mode
	for(int i=0; i<modeLength;i++){
		header[2+fileLength+1+i] = mode[i];
	}

//0 byte
	header[2 + fileLength + 1 + modeLength] = 0;
/*
	for(int i=0; i<2 + fileLength + 1 + modeLength + 1;i++)
		printf(" %d ", header[i]);
	printf("\n");
*/
	return header;
}

/*
 *                      DATA packet
 * Returns this byte chain:
 *  2 byte | 2 byte     | N bytes
 * ______________________________
 *  03     | n Bloque   | Datos
 */
BYTE * DATAPacket(int nBloque, BYTE * datos){
	int dataLength = strlen(datos);

	BYTE * header = (BYTE *) calloc( 2+2+dataLength, sizeof(BYTE));

// msg type
	header[0] = 0;
	header[1] = 3;

//nBloque
	if(nBloque >= 256*256){
		printf("N de bloque no puede ser introducido en 2 bytes.\n");
		return NULL;
	}
	header[2] = nBloque/256;
	header[3] = nBloque%256;

//N bytes
	for(int i = 0; i<dataLength; i++)
		header[4+i] = datos[i];


	for(int i=0; i<dataLength+4; i++)
		printf(" %d ", header[i]);
	printf("\n");

	return header;
}


/*
 *                      ACK
 * Returns this byte chain:
 *  2 byte | 2 byte
 * ___________________
 *  04     | n Bloque
 */
BYTE * ACK(int nBloque){

	BYTE * header = (BYTE *) calloc( 2+2, sizeof(BYTE));

// msg type
	header[0] = 0;
	header[1] = 4;

//nBloque
	if(nBloque >= 256*256){
		printf("N de bloque no puede ser introducido en 2 bytes.\n");
		return NULL;
	}
	header[2] = nBloque/256;
	header[3] = nBloque%256;

/*
	for(int i=0; i<4; i++)
		printf(" %d ", header[i]);
	printf("\n");
*/
	return header;
}


/*
 *                    Mensajes de error
 * Returns this byte chain:
 *  2 byte | 2 byte       | caracteres       | 1 byte
 * _______________________________________________________
 *  05     | codigo error | mensaje de error | 0
 */
BYTE * ERRORMSG(int CODIGODEERROR, char *errMsg){
	int errMsgLength = strlen(errMsg);

	BYTE * header = (BYTE *) calloc( 2+2+errMsgLength+1, sizeof(BYTE));

// msg type
	header[0] = 0;
	header[1] = 5;

// error code
	header[2] = CODIGODEERROR/256;
	header[3] = CODIGODEERROR%256;

// Error msg
	for(int i = 0; i<errMsgLength; i++)
		header[4+i] = errMsg[i];

// 1 byte
	header[4+errMsgLength] = 0;

/*
	for(int i=0; i<4+errMsgLength; i++)
		printf(" %d ", header[i]);
	printf("\n");
*/
	return header;
}

// Returns packet type
int getPacketType(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener tipo de paquete: Paquete nulo.\n");
		return -1;
	}
	int packetType = packet[0]*256+packet[1];
	return packetType;
}

//Returns file contained on a WRQ or RRQ packet
char * getFilename(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener el nombre de archivo del paquete: Paquete nulo.\n");
		return NULL;
	}

	int packetType = getPacketType(packet);
	if(packetType == 1 || packetType == 2)
	{
		int filenameLength;
		for(filenameLength = 0; packet[2+filenameLength]!=0; filenameLength++){
			if(packet[2+filenameLength]==0)
				break;
		}
		printf("%d\n", filenameLength);

		char * filename = (char *) malloc((filenameLength+1)* sizeof(char));
		for(int i = 0; i<filenameLength; i++)
			filename[i] = packet[2+i];
		filename[filenameLength] = '\0';
/*
		for(int i = 0; i<filenameLength; i++)
			printf("%c",filename[i]);
*/
		return filename;

	} else {
		printf("Error al obtener archivo del paquete: No se puede obtener archivo de un paquete que no es WRQ o RRQ.\n");
		return NULL;
	}
}

// gets the n of packet
int packetNumber(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener el numero del paquete: Paquete nulo.\n");
		return -1;
	}

	int packetType = getPacketType(packet);
	if(packetType == 3 || packetType == 4)
	{
		int nPacket = packet[2]*256+packet[3];
		return nPacket;
	} else {
		printf("Error al obtener el numero de paquete: el paquete debe ser de datos o de asentimiento.\n");
		return -1;
	}
}

// gets the n of packet
int getErrorCode(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener codigo de error del paquete: Paquete nulo.\n");
		return 1;
	}

	int packetType = getPacketType(packet);
	if(packetType == 5)
	{
		int err = packet[2]*256+packet[3];
		return err;
	} else {
		printf("Error al obtener el tipo de error: el paquete debe ser de error.\n");
		return 1;
	}
}

// Returns the error msg
char * getErrorMsg(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener el mensaje de error del paquete: Paquete nulo.\n");
		return NULL;
	}

	int packetType = getPacketType(packet);
	if(packetType == 5)
	{
		int errorMsgLength;
		for(errorMsgLength = 0; packet[4+errorMsgLength]!=0; errorMsgLength++){
			if(packet[4+errorMsgLength]==0)
				break;
		}
		printf("%d\n", errorMsgLength);

		char * filename = (char *) malloc((errorMsgLength+1)* sizeof(char));
		for(int i = 0; i<errorMsgLength; i++)
			filename[i] = packet[4+i];
		filename[errorMsgLength] = '\0';

		for(int i = 0; i<errorMsgLength; i++)
			printf("%c",filename[i]);

		return filename;

	} else {
		printf("Error al obtener el mensaje de error: No se puede obtener el mensaje de error de un paquete que no es de error.\n");
		return NULL;
	}
}

void printError(int errCode){
	switch(errCode){
		case NODEFINIDO:
			printf("Error: No definido\n");
			break;
		case FICHERONOENCONTRADO:
			printf("Error: Fichero no encontrado\n");
			break;
		case DISCOLLENO:
			printf("Error: Disco lleno\n");
			break;
		case OPERACIONILEGAL:
			printf("Error: Operacion ilegal de TFTP\n");
			break;
		case FICHEROYANOEXISTE:
			printf("Error: El fichero no existe\n");
			break;
		default:
			printf("Error: No definido - %d\n", errCode);
			break;
	}
}

// gets the n of packet
int plantilla(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener archivo del paquete: Paquete nulo.\n");
		return 1;
	}

	int packetType = getPacketType(packet);
	if(packetType == 3 || packetType == 4)
	{
	} else {
		printf("Error al el numero de paquete: el paquete debe ser de datos o de asentimiento.\n");
		return 1;
	}
}

void addToLog(char * hostname, int ip, char * protocol, int port)
{
	FILE *fd;

	fd = fopen("log.txt", "a");
	if(fd == NULL ) {fputs("File error", stderr); return;}
	fprintf(fd, "Hostname: %s ip: %d protocol: %s Port: %d Date: d\n", hostname, ip, protocol, port);
	fclose(fd);
}
