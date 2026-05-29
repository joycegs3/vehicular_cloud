#define _POSIX_C_SOURCE 199309L
#define _XOPEN_SOURCE 500
#include "../../middleware/distribution/midd4vc_client.h"
#include "../../middleware/distribution/midd4vc_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_PENDING_JOBS 64
#define SLEEP_INTERVAL 5

typedef struct {
    char task_id[64];
    int arg_val;
    int active;
} task_tracker_t;

static task_tracker_t trackers[MAX_PENDING_JOBS];

typedef struct {
    char id[64];
} application_state_t;

static application_state_t app;

void save_job_context(const char* jid, int val) {
    for (int i = 0; i < MAX_PENDING_JOBS; i++) {
        if (!trackers[i].active) {
            strncpy(trackers[i].task_id, jid, 63);
            trackers[i].arg_val = val;
            trackers[i].active = 1;
            return;
        }
    }
}

int get_and_clear_arg(const char* jid) {
    for (int i = 0; i < MAX_PENDING_JOBS; i++) {
        if (trackers[i].active && strcmp(trackers[i].task_id, jid) == 0) {
            trackers[i].active = 0; // Libera o slot
            return trackers[i].arg_val;
        }
    }
    return -1; // Não encontrado
}

void on_result_received(const char* job_id, midd4vc_job_status_t status, int result) {
    int original_arg = get_and_clear_arg(job_id);

    if (status == JOB_DONE) {
        printf("[CLIENT] Task %s Finished. Args: %d, Result: %d\n", job_id, original_arg, result);
    } else {
        printf("[CLIENT] Task %s FAILED | Arg: %d\n", job_id, original_arg);
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL) + getpid());

    // Inicializa trackers
    memset(trackers, 0, sizeof(trackers));

    if (argc >= 2) {
        strncpy(app.id, argv[1], 63);
    } else {
        snprintf(app.id, sizeof(app.id), "app_%03d", rand() % 1000);
    }

    midd4vc_client_t *client = midd4vc_create(app.id, ROLE_CLIENT);
    midd4vc_set_job_result_handler(client, on_result_received);
    midd4vc_start(client);

    printf("[CLIENT] %s online. Sistema de virtualização. Tracker ativado.\n", app.id);
    
    int task_count = 0;
    while (1) {
        char jid[64];
        snprintf(jid, sizeof(jid), "task_%d_%d", getpid(), task_count++);

        // Exemplo: Cálculo de fatorial (valor entre 1 e 10)
        int val = (rand() % 10) + 1;

        save_job_context(jid, val);
        
        // Coordenadas de exemplo (Recife)
        double lat = -8.055278;
        double lon = -34.951111;

        printf("[CLIENT] Submitting %s (val: %d)...\n", jid, val);

        container_spec_t spec;
        memset(&spec, 0, sizeof(container_spec_t));
        
        // 1. Specs de imagem e hardware do container
        strncpy(spec.image, "frolvlad/alpine-gcc", sizeof(spec.image)); // Verificar opção de imagem mais leve com compilador C
        spec.cpu_quota = 50000;         // 0.5 CPU
        spec.memory_limit = 104857600;  // 100 MB de RAM

        // 2. Montagem do comando
        snprintf(spec.command, sizeof(spec.command), "sh -c 'gcc /workspace/job.c -o /run_fact && /run_fact %d'", val);

        // 3. Código a ser executado dentro do container (Fatorial)
        strcpy(spec.code, 
            "#include <stdio.h>\n"
            "#include <stdlib.h>\n"
            "int main(int argc, char **argv) {\n"
            "    if(argc<2) return 1;\n"
            "    int n = atoi(argv[1]);\n"
            "    int fact = 1;\n"
            "    for(int i = 1; i <= n; i++) {\n"
            "        fact *= i;\n"
            "    }\n"
            "    printf(\"%d\", fact);\n" // Imprime apenas o número para o popen capturar
            "    return 0;\n"
            "}\n"
        );

        midd4vc_submit_job(client, jid, "Virtualization", "Container_RUN", lat, lon, &val, 1, &spec);

        // Intervalo de 200ms entre envios para um fluxo constante (ajustável)
        sleep(SLEEP_INTERVAL); 
    }
    return 0;
}