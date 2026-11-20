# gestionHospital

**Sistema de Triaje y Quirófanos en Urgencias de un Hospital**

El sistema modela el funcionamiento de un servicio de urgencias hospitalarias donde múltiples pacientes (procesos independientes) llegan con distintos niveles de gravedad y compiten por recursos limitados (boxes de consulta y quirófanos) gestionados por un equipo médico multihilo.

---

## Arquitectura del Sistema

```mermaid
flowchart LR
    G[generador.c<br/>fork + execl] -->|crea| P[paciente.c<br/>alarm + pause]
    P -->|msgsnd mtype 1..3| Q[(Cola de Mensajes<br/>Triaje por Prioridad)]
    Q -->|msgrcv IPC_NOWAIT| M[medico.c<br/>Hilos pthread]
    P <-->|sem_wait / sem_post| S[(Semáforos +<br/>Memoria Compartida)]
    M <-->|sem_wait / sem_post| S
    M -->|kill SIGUSR1| P
    M -->|mmap + ftruncate| F[(historial_urgencias/<br/>registro_altas.txt)]
