#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned char BYTE;

#define PACKETSIZE 512

#define NODEFINIDO 0
#define FICHERONOENCONTRADO 1
#define DISCOLLENO 3
#define OPERACIONILEGAL 4
#define FICHEROYANOEXISTE 6

/*
 * Constructores de los paquetes
 */
BYTE * WRQ(char * filename, char * mode);
BYTE * RRQ(char * filename, char * mode);
BYTE * DATAPacket(int nBloque, BYTE * datos);
BYTE * ACK(int nBloque);
BYTE * ErrorMsg(int CODIGODEERROR, char *errMsg);
/*
 * Manejadores de la información contenida en los paquetes
 */
int getPacketType(BYTE *packet);
char * getFilename(BYTE *packet);
int getPacketNumber(BYTE *packet);
int getDataLength(BYTE *packet);
int isLastPacket(BYTE *packet);
char * getDataMSG(BYTE *packet);
int getErrorCode(BYTE *packet);
char * getErrorMsg(BYTE *packet);
void printError(int errCode);
void printMSG(BYTE *packet);

/* Este struct no se usa, aunque sería la opcion mas conveniente una vez
 * comprendido el funcionamiento del protocolo.
 * En vez de esto se utiliza un array instanciado con calloc, no obstante el resultado
 * final en memoria es idéntico en ambos casos.
 *//*
typedef Union {
	struct WReQuest_RReQuest{
		BYTE[2] msgType;
		BYTE[512+2] filename_and_mode;
	}

	struct DataPacket{
		BYTE[2] msgType;
		BYTE[2] nBloque;
		BYTE[512] data;
	}

	struct ACK{
		BYTE[2] msgType;
		BYTE[2] nBloque;
	}

	struct ErrorMsg{
		BYTE[2] msgType;
		BYTE[2] errorCode;
		BYTE[512] message;
	}
} Message;
*/

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

/*
	for(int i=0; i<2 + fileLength + 1 + modeLength + 1;i++)
		printf(" %d ", header[i]);
	printf("\n");
*/
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

/*
	for(int i=0; i<dataLength+4; i++)
		printf(" %d ", header[i]);
	printf("\n");
*/
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
BYTE * ErrorMsg(int CODIGODEERROR, char *errMsg){
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
		int dataLength;
		for(dataLength = 0; packet[2+dataLength]!=0; dataLength++){
			if(packet[2+dataLength]==0)
				break;
		}
//		printf("%d\n", dataLength);

		char * filename = (char *) malloc((dataLength+1)* sizeof(char));
		for(int i = 0; i<dataLength; i++)
			filename[i] = packet[2+i];
		filename[dataLength] = '\0';
/*
		for(int i = 0; i<dataLength; i++)
			printf("%c",filename[i]);
*/
		return filename;

	} else {
		printf("Error al obtener archivo del paquete: No se puede obtener archivo de un paquete que no es WRQ o RRQ.\n");
		return NULL;
	}
}

// gets the n of packet
int getPacketNumber(BYTE *packet){
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

int getDataLength(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener la longitud de los datos: Paquete nulo.\n");
		return -1;
	}

	int packetType = getPacketType(packet);
	if(packetType == 3) {
		int dataLength;
		char buff[PACKETSIZE];
		//memset(buff, 0, PACKETSIZE);
		memcpy(buff, packet+4, PACKETSIZE);
		for(dataLength = 0; dataLength<PACKETSIZE; dataLength++){
			if(buff[dataLength]==0)
				break;
			}
			//printf("DataLength: %d\n", dataLength);
		return dataLength;
	} else {
		printf("Error al obtener la longitud de los datos: el paquete debe ser de datos.\n");
		return -1;
	}
}

int isLastPacket(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error isLastPacket: Paquete nulo.\n");
		return -1;
	}

	int packetType = getPacketType(packet);
	if(packetType == 3) {
		int dataLength = getDataLength(packet);
		if(dataLength < PACKETSIZE)
			return 1;
		else
			return 0;
	} else {
		printf("Error al obtener la longitud de los datos: el paquete debe ser de datos.\n");
		return -1;
	}

}

char * getDataMSG(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al obtener el paquete: Paquete nulo.\n");
		return NULL;
	}

	int packetType = getPacketType(packet);
	if(packetType == 3) {
		int dataLength = getDataLength(packet);
//		printf("Data length: %d\n", dataLength);

		char * data = (char *) malloc((dataLength+1)* sizeof(char));

		memcpy(data, packet+4, dataLength);

/*
		for(int i = 0; i<dataLength; i++)
			printf("packetData: %c\n",data[i]);
		printf("%s\n", data);
*/
		return data;
	} else {
		printf("Error al obtener el paquete: el paquete debe ser de datos.\n");
		return NULL;
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
//		printf("%d\n", errorMsgLength);

		char * filename = (char *) malloc((errorMsgLength+1)* sizeof(char));
		for(int i = 0; i<errorMsgLength; i++)
			filename[i] = packet[4+i];
		filename[errorMsgLength] = '\0';

/*
		for(int i = 0; i<errorMsgLength; i++)
			printf("%c",filename[i]);
*/
		return filename;

	} else {
		printf("Error al obtener el mensaje de error: No se puede obtener el mensaje de error de un paquete que no es de error.\n");
		return NULL;
	}
}

void printError(int errCode){
	switch(errCode){
		case NODEFINIDO:
			printf("Error: No definido");
			break;
		case FICHERONOENCONTRADO:
			printf("Error: Fichero no encontrado");
			break;
		case DISCOLLENO:
			printf("Error: Disco lleno");
			break;
		case OPERACIONILEGAL:
			printf("Error: Operacion ilegal de TFTP");
			break;
		case FICHEROYANOEXISTE:
			printf("Error: El fichero no existe");
			break;
		default:
			printf("Error: No definido - %d", errCode);
			break;
	}
}

void printMSG(BYTE *packet){
	if(packet == NULL)
	{
		printf("Error al imprimir el paquete: Paquete nulo.\n");
		return;
	}
	int i, packetN;

	printf("\n - Imprimiendo paquete... \n");
	int packetType = getPacketType(packet);
	printf("Tipo: %d\n", packetType);
	printf(" | %d %d | ", packet[0], packet[1]);

	switch(packetType){
		case 1:
		case 2:
			for(i = 2; i<PACKETSIZE && packet[i] != 0; i++)
				printf(" %d ", packet[i]);
			printf(" | %d | ", packet[i]);
			for(i = i+1; i<PACKETSIZE && packet[i] != 0; i++) { //modo
				printf(" %d ", packet[i]);
			}
			printf(" | %d | \n", packet[i]);
			char * filename = getFilename(packet);
			printf("Nombre de archivo: %s\n", filename);
			printf("Modo: \n\n");
			break;

		case 3:
			printf("%d %d | ", packet[2], packet[3]);
			int dataLength = getDataLength(packet);
			packetN = getPacketNumber(packet);
			for(i=0; i<dataLength; i++)
				printf(" %d ", packet[4+i]);
			printf("|\n");
			char * msg = getDataMSG(packet);
			printf("Packet number: %d\n", packetN);
			printf("Data length: %d\n", dataLength);
			printf("Package data: %s\n\n", msg);
			break;

		case 4:
			printf("%d %d |\n", packet[2], packet[3]);
			int packetN = getPacketNumber(packet);
			printf("Packet number %d\n\n", packetN);
			break;

		case 5:
			printf("%d %d |", packet[2], packet[3]);
			for(i=0; i<PACKETSIZE && packet[4+i] !=0; i++)
				printf(" %d ", packet[4+i]);
			printf("| %d |\n", packet[4+i]);
			int errorCode = getErrorCode(packet);
			char * errorMsg = getErrorMsg(packet);
			printf("Error code: %d\n", errorCode);
			printError(errorCode);
			printf("\nMsg: %s\n\n", errorMsg);
			break;
	}

}

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
