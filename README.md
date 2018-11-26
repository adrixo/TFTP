# TFTP
Servidor y clientes TCP

## [Trivial file transfer Protocol](https://es.wikipedia.org/wiki/TFTP):

Es un protocolo de transferencia muy simple semejante a una versión
básica de FTP. TFTP a menudo se utiliza para transferir pequeños
archivos entre ordenadores en una red, como cuando un terminal X Window
o cualquier otro cliente ligero arranca desde un servidor de red.

- Utiliza UDP (puerto 69) como protocolo de transporte.
- No puede listar contenido de los directorios.
- No existen mecanismos de autenticación o cifrado.
- Se utiliza para leer o escribir archivos de un servidor remoto.
- Soporta tres modos diferentes de transferencia, "netascii", "octet" y
"mail", de los que los dos primeros corresponden a los modos "ascii" e
"imagen" (binario) del protocolo FTP.

## Protocolo

### Mensajes

#### Tipo

**tipo - descripción**

1 - Leer

2 - Escribir

3 - Datos

4 - Asentimiento

5 - Error

#### Formato

- Lectura/Escritura
```
 2 byte | caracteres     | 1 byte | caracteres | 1 byte
_______________________________________________________
01 ó 02 | nombre fichero | 0      | modo       | 0
```
Modo puede ser: netascii, octet ó mail
Utilizamos octet

- Mensajes de datos
```
 2 byte | 2 byte     | N bytes
______________________________
 03     | n Bloque   | Datos
```

- Mensajes de Asentimiento
```
 2 byte | 2 byte   
___________________
 04     | n Bloque
```

- Mensajes de error
```
 2 byte | 2 byte       | caracteres       | 1 byte
_______________________________________________________
 05     | codigo error | mensaje de error | 0
```

**error - significado**

0 - No definido
1 - Fichero no encontrado
3 - Disco lleno
4 - Operacion ilegal de TFTP
6 - El fichero ya no existe
