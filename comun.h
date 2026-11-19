#ifndef COMUN_H
#define COMUN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>

/* Parámetros de configuración del hospital */
#define NUM_BOXES 3          /* Camas de atención simultánea */
#define NUM_QUIROFANOS 1     /* Recurso crítico exclusivo */
#define MAX_TEXTO 100

/* Niveles de prioridad para mtype en la cola de mensajes (P7) */
#define PRIO_CRITICO 1       /* Máxima prioridad */
#define PRIO_URGENTE 2
#define PRIO_LEVE    3

/* Claves para ftok (P6 y P7) */
#define RUTA_FTOK "."
#define PROJ_SHM  'H'
#define PROJ_MSG  'U'

/* Nombres de los semáforos con nombre (P5) */
#define SEM_MUTEX     "/sem_hosp_mutex"
#define SEM_BOXES     "/sem_hosp_boxes"
#define SEM_QUIROFANO "/sem_hosp_quirofano"

/* Estructura de la Memoria Compartida (P6) */
typedef struct {
    int boxes_libres;
    int quirofanos_libres;
    int pacientes_atendidos;
    int pacientes_en_espera;
    int alerta_emergencia;   /* 0 = normal, 1 = catástrofe activada por señal */
} EstadoHospital;

/* Estructura para la Cola de Mensajes (P7) */
typedef struct {
    long mtype;              /* Gravedad: 1, 2 o 3 */
    pid_t pid_paciente;      /* Para poder enviarle señales si hace falta */
    int gravedad;
    char sintoma[MAX_TEXTO];
} MensajePaciente;

#endif
