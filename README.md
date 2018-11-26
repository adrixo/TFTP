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

