#include "comun.h"

int main(void) {
    key_t clave_shm, clave_msg;
    int shmid, msqid;
    EstadoHospital *hospital;
    sem_t *mutex, *boxes, *quirofano;

    /* 1. Crear e inicializar semáforos con nombre (P5) */
    mutex = sem_open(SEM_MUTEX, O_CREAT, 0600, 1);
    boxes = sem_open(SEM_BOXES, O_CREAT, 0600, NUM_BOXES);
    quirofano = sem_open(SEM_QUIROFANO, O_CREAT, 0600, NUM_QUIROFANOS);

    if (mutex == SEM_FAILED || boxes == SEM_FAILED || quirofano == SEM_FAILED) {
        perror("Error al crear semáforos");
        exit(EXIT_FAILURE);
    }
    sem_close(mutex);
    sem_close(boxes);
    sem_close(quirofano);

    /* 2. Crear e inicializar Memoria Compartida (P6) */
    clave_shm = ftok(RUTA_FTOK, PROJ_SHM);
    shmid = shmget(clave_shm, sizeof(EstadoHospital), IPC_CREAT | 0660);
    if (shmid == -1) {
        perror("Error en shmget");
        exit(EXIT_FAILURE);
    }

    hospital = (EstadoHospital *)shmat(shmid, NULL, 0);
    if (hospital == (EstadoHospital *)-1) {
        perror("Error en shmat");
        exit(EXIT_FAILURE);
    }

    /* Valores iniciales del hospital */
    hospital->boxes_libres = NUM_BOXES;
    hospital->quirofanos_libres = NUM_QUIROFANOS;
    hospital->pacientes_atendidos = 0;
    hospital->pacientes_en_espera = 0;
    hospital->alerta_emergencia = 0;

    shmdt(hospital); /* Desenlazamos tras inicializar */

    /* 3. Crear Cola de Mensajes (P7) */
    clave_msg = ftok(RUTA_FTOK, PROJ_MSG);
    msqid = msgget(clave_msg, IPC_CREAT | 0660);
    if (msqid == -1) {
        perror("Error en msgget");
        exit(EXIT_FAILURE);
    }

    printf("[INICIADOR] Recursos IPC del hospital creados correctamente.\n");
    printf("            SHM ID: %d | MSG ID: %d\n", shmid, msqid);
    return 0;
}
