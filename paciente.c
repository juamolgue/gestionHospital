#include "comun.h"

/* Variable global volátil para manejar señales de forma segura (P2) */
volatile sig_atomic_t estado_paciente = 0; 
/* 0 = esperando, 1 = atendido (SIGUSR1), 2 = cansado de esperar (SIGALRM) */

void manejador_senales(int sig) {
    if (sig == SIGUSR1) {
        estado_paciente = 1;
    } else if (sig == SIGALRM) {
        estado_paciente = 2;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <gravedad (1=Critico, 2=Urgente, 3=Leve)> <sintoma>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int gravedad = atoi(argv[1]);
    if (gravedad < PRIO_CRITICO || gravedad > PRIO_LEVE) {
        gravedad = PRIO_LEVE;
    }

    /* 1. Armar señales SIGUSR1 (atendido) y SIGALRM (timeout de espera) (P2) */
    struct sigaction act;
    act.sa_handler = manejador_senales;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1, &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    /* 2. Abrir recursos IPC existentes (sin O_CREAT) (P5, P6, P7) */
    sem_t *mutex = sem_open(SEM_MUTEX, 0);
    if (mutex == SEM_FAILED) {
        perror("[PACIENTE] Error abriendo semáforo (¿ejecutaste ./iniciador?)");
        exit(EXIT_FAILURE);
    }

    key_t clave_shm = ftok(RUTA_FTOK, PROJ_SHM);
    int shmid = shmget(clave_shm, sizeof(EstadoHospital), 0);
    EstadoHospital *hospital = (EstadoHospital *)shmat(shmid, NULL, 0);

    key_t clave_msg = ftok(RUTA_FTOK, PROJ_MSG);
    int msqid = msgget(clave_msg, 0);

    /* 3. Actualizar sala de espera en Memoria Compartida (Exclusión Mutua) */
    sem_wait(mutex);
    hospital->pacientes_en_espera++;
    int esperando_ahora = hospital->pacientes_en_espera;
    sem_post(mutex);

    /* 4. Enviar ficha de triaje a la Cola de Mensajes (P7) */
    MensajePaciente msg;
    msg.mtype = gravedad;
    msg.pid_paciente = getpid();
    msg.gravedad = gravedad;
    strncpy(msg.sintoma, argv[2], MAX_TEXTO - 1);
    msg.sintoma[MAX_TEXTO - 1] = '\0';

    if (msgsnd(msqid, &msg, sizeof(MensajePaciente) - sizeof(long), 0) == -1) {
        perror("[PACIENTE] Error enviando ficha a triaje");
        exit(EXIT_FAILURE);
    }

    printf("[PACIENTE %d] Llega a urgencias (Gravedad: %d, Sintoma: '%s'). En sala: %d\n",
           getpid(), gravedad, msg.sintoma, esperando_ahora);

    /* 5. Esperar atención médica o timeout de 20 segundos (P2) */
    alarm(20);
    while (estado_paciente == 0) {
        pause(); /* Se suspende hasta recibir SIGUSR1 o SIGALRM */
    }
    alarm(0); /* Cancelar alarma pendiente si fue atendido */

    if (estado_paciente == 1) {
        printf("[PACIENTE %d] Ha sido atendido y recibe el alta. ¡Gracias!\n", getpid());
    } else {
        printf("[PACIENTE %d] ¡Tiempo agotado! Se marcha sin ser atendido.\n", getpid());
        sem_wait(mutex);
        if (hospital->pacientes_en_espera > 0) {
            hospital->pacientes_en_espera--;
        }
        sem_post(mutex);
    }

    /* 6. Limpieza local de descriptores del proceso */
    shmdt(hospital);
    sem_close(mutex);
    return 0;
}
