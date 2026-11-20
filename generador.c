#include "comun.h"
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <numero_de_pacientes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_pacientes = atoi(argv[1]);
    const char *sintomas[] = {
        "Dolor toracico", "Fractura brazo", "Fiebre alta",
        "Corte profundo", "Mareos leves", "Alergia aguda"
    };
    int total_sintomas = 6;

    srand(time(NULL));
    printf("[GENERADOR] Iniciando simulacion con %d pacientes...\n", num_pacientes);

    for (int i = 0; i < num_pacientes; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("Error en fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            /* Proceso hijo: muta a ./paciente usando execl (P1) */
            char str_gravedad[10];
            int grav = (rand() % 3) + 1; /* Aleatorio entre 1 y 3 */
            snprintf(str_gravedad, sizeof(str_gravedad), "%d", grav);

            const char *sintoma = sintomas[rand() % total_sintomas];

            execl("./paciente", "paciente", str_gravedad, sintoma, NULL);
            perror("Error en execl de paciente");
            exit(EXIT_FAILURE);
        }

        /* Pequeña pausa de 1 segundo entre llegadas */
        sleep(1);
    }

    /* El proceso padre espera a que todos los hijos terminen (P1) */
    for (int i = 0; i < num_pacientes; i++) {
        wait(NULL);
    }

    printf("[GENERADOR] Todos los pacientes de la oleada han finalizado.\n");
    return 0;
}
