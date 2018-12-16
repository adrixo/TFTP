#!/bin/bash
pkill servidor
./servidor
./cliente localhost tcp e fichero1.txt &
./cliente localhost tcp l fichero2.txt &
./cliente localhost tcp e fichero3.txt &
./cliente localhost udp e fichero4.txt &
./cliente localhost udp l fichero5.txt &
./cliente localhost udp e fichero6.txt &
