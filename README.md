<h1>gestionHospital</h1>
<h3>Sistema de Triaje y Quirófanos en Urgencias de un Hospital</h1>
<p>Proyecto que consiste en diseñar el sistema de un hospital, gestionando la cola de pacientes por prioridad y asignando médicos según dicha prioridad, teniendo en cuenta el número de camas y quirófanos disponibles, además de su nivel de ocupación en el momento de llegada de los pacientes.</p>
<p>A continuación, se detalla una breve descripción de cada fichero introducido.</p>
<p><strong>comun.h</strong>: Este fichero debemos de incluirlo en el resto de archivos. Define las claves IPC, los nombres de los semáforos, la estructura de la memoria compartida y el formato de los mensajes de triaje.</p>
<p><strong>iniciador.c</strong>: Como su nombre indica, este programa se ejecuta una vez al principio al principio de las pruebas. Crea e inicializa los semáforos, reserva y mapea la memoria compartida y crea la cola de mensajes.</p>
<p><strong>limpiador.c</strong>: Programa que marca la memoria y cola para borrado y desenlaza los semáforos. Fundamental para que no se queden recursos bloqueados en Linux entre prueba y prueba.</p>
<p><strong>paciente.c</strong>: Este programa representa a un único paciente que llega a urgencias, envía su ficha de traje y espera a que lo atiendan. Si pasa un tiempo determinado sin ser atendido, este se marcha del hospital sin ser atendido, dejando un hueco libre para el siguiente paciente.</p>
<p><strong>generador.c</strong>: Este programa crea múltiples procesos paciente independientes en paralelo con síntomas y gravedades aleatorias, y espera a que todos finalicen. Simula una oleada en urgencias.</p>
