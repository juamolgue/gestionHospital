#include "comun.h"
#include <pthread.h>
#include <sys/mman.h>
#include <sys/times.h>
#include <time.h>
#include <errno.h>

#define DIR_HISTORIAL "historial_urgencias"
#define FICHERO_ALTAS "historial_urgencias/registro_altas.txt"

/* Bandera global para finalizar el turno limpiamente al recibir SIGINT */
volatile sig_atomic_t fin_turno = 0;

/* Recursos globales compartidos entre los hilos médicos del mismo proceso (Tema 2) */
int msqid;
EstadoHospital *hospital;
sem_t *mutex, *boxes, *quirofano;

void manejador_cierre(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        fin_turno = 1;
    } else if (sig == SIGUSR2) {
        /* Alerta de emergencia masiva enviada desde consola con kill -USR2 */
        if (hospital != NULL) {
            hospital->alerta_emergencia = 1;
        }
    }
}

/* Función que proyecta el fichero con mmap() para añadir cada alta médica (P9) */
void registrar_alta_mmap(int id_medico, MensajePaciente *pac, int uso_quirofano) {
    char linea[256];
    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora); /* P8: Fecha y hora */

    int len = snprintf(linea, sizeof(linea),
        "[%02d/%02d/%04d %02d:%02d:%02d] Medico %d -> Paciente PID %d | Gravedad %d (%s) | Quirofano: %s\n",
        t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
        t->tm_hour, t->tm_min, t->tm_sec,
        id_medico, pac->pid_paciente, pac->gravedad, pac->sintoma,
        uso_quirofano ? "SI" : "NO");

    int fd = open(FICHERO_ALTAS, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("[MEDICO] Error abriendo fichero de historial");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return;
    }

    off_t tam_anterior = st.st_size;
    off_t tam_nuevo = tam_anterior + len;

    /* Ampliamos el tamaño del fichero antes de proyectarlo (P9) */
    if (ftruncate(fd, tam_nuevo) == -1) {
        perror("[MEDICO] Error en ftruncate");
        close(fd);
        return;
    }

    char *mapa = (char *)mmap(NULL, tam_nuevo, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); /* Se puede cerrar el descriptor tras mmap (P9) */

    if (mapa == MAP_FAILED) {
        perror("[MEDICO] Error en mmap");
        return;
    }

    /* Escribimos directamente en memoria proyectada y sincronizamos */
    memcpy(mapa + tam_anterior, linea, len);
    munmap(mapa, tam_nuevo);
}

/* Rutina ejecutada por cada hilo médico (P4) */
void *rutina_medico(void *arg) {
    int id_medico = *((int *)arg);
    free(arg);
    MensajePaciente msg;
    size_t tam_msg = sizeof(MensajePaciente) - sizeof(long);

    printf("[MEDICO %d] Inicia guardia en urgencias.\n", id_medico);

    while (!fin_turno) {
        /* 1. Selección por prioridad estricta en Cola de Mensajes (P7) */
        ssize_t res = msgrcv(msqid, &msg, tam_msg, PRIO_CRITICO, IPC_NOWAIT);
        if (res == -1) {
            res = msgrcv(msqid, &msg, tam_msg, PRIO_URGENTE, IPC_NOWAIT);
        }
        if (res == -1) {
            res = msgrcv(msqid, &msg, tam_msg, PRIO_LEVE, IPC_NOWAIT);
        }

        if (res == -1) {
            /* No hay pacientes esperando ahora mismo; descansamos 1s */
            sleep(1);
            continue;
        }

        /* 2. Reservar un Box de atención (Semáforo contador, P5) */
        sem_wait(boxes);

        sem_wait(mutex);
        hospital->boxes_libres--;
        if (hospital->pacientes_en_espera > 0) {
            hospital->pacientes_en_espera--;
        }
        int boxes_disp = hospital->boxes_libres;
        sem_post(mutex);

        int uso_quirofano = 0;

        /* 3. Si es paciente CRÍTICO (1), requiere además el Quirófano exclusivo */
        if (msg.gravedad == PRIO_CRITICO) {
            printf("[MEDICO %d] ¡EMERGENCIA! Solicitando quirofano para Paciente %d (%s)...\n",
                   id_medico, msg.pid_paciente, msg.sintoma);
            sem_wait(quirofano);
            uso_quirofano = 1;

            sem_wait(mutex);
            hospital->quirofanos_libres--;
            sem_post(mutex);

            printf("[MEDICO %d] Operando en quirofano a Paciente %d...\n",
                   id_medico, msg.pid_paciente);
            sleep(3); /* Simulación de cirugía */

            sem_wait(mutex);
            hospital->quirofanos_libres++;
            sem_post(mutex);

            sem_post(quirofano);
        } else {
            printf("[MEDICO %d] Atendiendo en Box (libres: %d) a Paciente %d [Gravedad %d: %s]\n",
                   id_medico, boxes_disp, msg.pid_paciente, msg.gravedad, msg.sintoma);
            sleep(2); /* Simulación de consulta */
        }

        /* 4. Notificar al proceso Paciente que ha sido atendido mediante señal (P2) */
        if (kill(msg.pid_paciente, SIGUSR1) == -1) {
            printf("[MEDICO %d] Aviso: El paciente %d ya se habia marchado por timeout.\n",
                   id_medico, msg.pid_paciente);
        } else {
            /* 5. Actualizar estadísticas y guardar historial con mmap en exclusión mutua */
            sem_wait(mutex);
            hospital->pacientes_atendidos++;
            registrar_alta_mmap(id_medico, &msg, uso_quirofano);
            sem_post(mutex);
        }

        /* 6. Liberar el Box */
        sem_wait(mutex);
        hospital->boxes_libres++;
        sem_post(mutex);
        sem_post(boxes);
    }

    printf("[MEDICO %d] Finaliza su guardia.\n", id_medico);
    pthread_exit(NULL);
}

int main(void) {
    /* Medición de tiempos de CPU al iniciar el turno (P8) */
    struct tms tms_inicio, tms_fin;
    clock_t reloj_inicio = times(&tms_inicio);
    long ticks_por_seg = sysconf(_SC_CLK_TCK);

    /* 1. Armar señales SIGINT (cierre limpio) y SIGUSR2 (emergencia) (P2) */
    struct sigaction act;
    act.sa_handler = manejador_cierre;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGUSR2, &act, NULL);

    /* 2. Crear directorio para historiales clínicos (P9) */
    mkdir(DIR_HISTORIAL, 0755);

    /* 3. Conectar a semáforos, memoria compartida y cola de mensajes (P5, P6, P7) */
    mutex = sem_open(SEM_MUTEX, 0);
    boxes = sem_open(SEM_BOXES, 0);
    quirofano = sem_open(SEM_QUIROFANO, 0);

    if (mutex == SEM_FAILED || boxes == SEM_FAILED || quirofano == SEM_FAILED) {
        perror("[HOSPITAL] Error abriendo semaforos (ejecuta ./iniciador primero)");
        exit(EXIT_FAILURE);
    }

    key_t clave_shm = ftok(RUTA_FTOK, PROJ_SHM);
    int shmid = shmget(clave_shm, sizeof(EstadoHospital), 0);
    hospital = (EstadoHospital *)shmat(shmid, NULL, 0);

    key_t clave_msg = ftok(RUTA_FTOK, PROJ_MSG);
    msqid = msgget(clave_msg, 0);

    /* 4. Lanzar tantos hilos médicos como boxes disponibles (P4) */
    pthread_t hilos[NUM_BOXES];
    for (int i = 0; i < NUM_BOXES; i++) {
        int *id = malloc(sizeof(int));
        *id = i + 1;
        if (pthread_create(&hilos[i], NULL, rutina_medico, id) != 0) {
            perror("[HOSPITAL] Error creando hilo medico");
            exit(EXIT_FAILURE);
        }
    }

    /* 5. Esperar a que todos los hilos médicos terminen tras SIGINT (P4) */
    for (int i = 0; i < NUM_BOXES; i++) {
        pthread_join(hilos[i], NULL);
    }

    /* 6. Informe final de rendimiento y contabilidad de CPU (P8) */
    clock_t reloj_fin = times(&tms_fin);
    printf("\n=== RESUMEN DEL TURNO DE URGENCIAS ===\n");
    printf("Pacientes atendidos con exito: %d\n", hospital->pacientes_atendidos);
    printf("Pacientes que quedaron en espera: %d\n", hospital->pacientes_en_espera);
    printf("Tiempo real de guardia:    %.2f s\n", (float)(reloj_fin - reloj_inicio) / ticks_por_seg);
    printf("Tiempo CPU modo usuario:   %.2f s\n", (float)(tms_fin.tms_utime - tms_inicio.tms_utime) / ticks_por_seg);
    printf("Tiempo CPU modo sistema:   %.2f s\n", (float)(tms_fin.tms_stime - tms_inicio.tms_stime) / ticks_por_seg);
    printf("Historial guardado en:     %s\n", FICHERO_ALTAS);

    /* 7. Desenlazar recursos locales */
    shmdt(hospital);
    sem_close(mutex);
    sem_close(boxes);
    sem_close(quirofano);

    return 0;
}
