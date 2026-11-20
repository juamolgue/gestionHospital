#!/bin/bash
# Script de compilacion y simulacion completa - Urgencias Hospitalarias (P0)

NUM_PACIENTES=${1:-8}

echo "=== 1. Compilando todos los modulos ==="
gcc -Wall iniciador.c -o iniciador -pthread || exit 1
gcc -Wall limpiador.c -o limpiador -pthread || exit 1
gcc -Wall paciente.c  -o paciente  -pthread || exit 1
gcc -Wall generador.c -o generador          || exit 1
gcc -Wall medico.c    -o medico    -pthread || exit 1

echo "=== 2. Inicializando recursos IPC del Kernel ==="
./limpiador > /dev/null 2>&1
./iniciador

echo "=== 3. Abriendo servicio de Urgencias (medico en background) ==="
./medico &
PID_HOSPITAL=$!
sleep 1

echo "=== 4. Lanzando oleada de $NUM_PACIENTES pacientes ==="
./generador $NUM_PACIENTES

# Damos 3 segundos extra para que terminen los ultimos pacientes en box/quirofano
sleep 3

echo "=== 5. Cerrando turno hospitalario (enviando SIGINT a PID $PID_HOSPITAL) ==="
kill -SIGINT $PID_HOSPITAL
wait $PID_HOSPITAL

echo "=== 6. Mostrando historial generado con mmap ==="
cat historial_urgencias/registro_altas.txt

echo "=== 7. Limpiando recursos IPC ==="
./limpiador
