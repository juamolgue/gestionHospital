#include "comun.h"

int main(void) {
    key_t clave_shm, clave_msg;
    int shmid, msqid;

    /* 1. Borrar semáforos con nombre (P5) */
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_BOXES);
    sem_unlink(SEM_QUIROFANO);

    /* 2. Borrar Memoria Compartida (P6) */
    clave_shm = ftok(RUTA_FTOK, PROJ_SHM);
    shmid = shmget(clave_shm, sizeof(EstadoHospital), 0);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }

    /* 3. Borrar Cola de Mensajes (P7) */
    clave_msg = ftok(RUTA_FTOK, PROJ_MSG);
    msqid = msgget(clave_msg, 0);
    if (msqid != -1) {
        msgctl(msqid, IPC_RMID, NULL);
    }

    printf("[LIMPIADOR] Todos los recursos IPC han sido eliminados.\n");
    return 0;
}
