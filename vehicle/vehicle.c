#define _POSIX_C_SOURCE 199309L
#define _XOPEN_SOURCE 500

#include "../../middleware/distribution/midd4vc_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// Constante para conversão aproximada de metros para graus (Rigor de movimento local)
#define METERS_PER_DEGREE 111111.0

// Limites Geográficos (Campus UFPE)
#define UFPE_MIN_LAT -8.057868
#define UFPE_MAX_LAT -8.046450
#define UFPE_MIN_LON -34.955076
#define UFPE_MAX_LON -34.945806

typedef struct {
    char id[64];
    double lat;
    double lon;
    double v_base_delta; 
    int is_mobile;
} mobile_state_t;

static mobile_state_t vehicle_node;

typedef struct {
    double lat;
    double lon;
    char name[32]; // Opcional, para logs
} waypoint_t;

static waypoint_t campus_map[] = {
    {-8.055278, -34.951111, "CIn"},               // Centro de Informática
    {-8.054450, -34.951150, "Cruzamento CCEN"},   // Saída do CIn para o CCEN
    {-8.052600, -34.949300, "Reitoria"},          // Reitoria da UFPE
    {-8.049400, -34.945500, "Medicina/HC"},      // Área de Medicina / Hospital das Clínicas
    {-8.051500, -34.943800, "Área de Saúde"},     // Saída pela via lateral do HC
    {-8.054100, -34.947800, "Biblioteca Central"},// Passando pela BC
    {-8.056100, -34.950200, "NIATE/CTG"},         // Área de Engenharia
    {-8.055278, -34.951111, "CIn (Retorno)"}      // Fecha o ciclo
};
#define MAP_SIZE (sizeof(campus_map) / sizeof(waypoint_t))

static int target_idx = 0;

/**
 * Converte velocidade (km/h) para deslocamento em graus
 * Baseado na premissa de atualização a cada 1 segundo (sleep(1))
 */
double kmh_to_lat_delta(double kmh) {
    double metros_por_segundo = kmh / 3.6;
    return metros_por_segundo / METERS_PER_DEGREE;
}

/**
 * Handler disparado ao receber um Job do Middleware
 */
void on_job_received(midd4vc_client_t *c, const midd4vc_job_t *job) {
    printf("\n>>> [%s] JOB RECEIVED: ID %s em (%.6f, %.6f)\n", 
            vehicle_node.id, job->job_id, vehicle_node.lat, vehicle_node.lon);

    // Execução da tarefa lógica
    int result = midd4vc_execute_job_internal(c, job);
    
    if (result >= 0) {
        printf("<<< [%s] Job %s concluído. Enviando resultado: %d\n", 
                vehicle_node.id, job->job_id, result);
        midd4vc_send_job_success(c, job->client_id, job->job_id, result);
    } else {
        printf("!!! [%s] Erro na execução do Job %s\n", vehicle_node.id, job->job_id);
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL) + getpid());

    double velocidade_kmh = (argc >= 2) ? atof(argv[1]) : 0.0;
    vehicle_node.is_mobile = (velocidade_kmh > 0.1);

    // 1. Definição do ID
    if (argc >= 3) {
        strncpy(vehicle_node.id, argv[2], 63);
    } else {
        snprintf(vehicle_node.id, sizeof(vehicle_node.id), "veh_%03d", rand() % 1000, (int)velocidade_kmh);
    }

    // 2. Configuração de Velocidade e Mobilidade
    vehicle_node.v_base_delta = kmh_to_lat_delta(velocidade_kmh);
    
    // 3. Posição Inicial
    if (argc >= 5) {
        vehicle_node.lat = atof(argv[3]);
        vehicle_node.lon = atof(argv[4]);
    } else {
        vehicle_node.lat = campus_map[0].lat; 
        vehicle_node.lon = campus_map[0].lon;
        printf("[%s] Usando coordenadas padrão (CIn).\n", vehicle_node.id);
    }
    
    // Inicialização do Middleware
    midd4vc_client_t *v_node = midd4vc_create(vehicle_node.id, ROLE_VEHICLE);
    midd4vc_set_job_handler(v_node, on_job_received);
    midd4vc_start(v_node);

    printf("[%s] Online | Modo: %s | Vel: %.1f km/h\n",
        vehicle_node.id, vehicle_node.is_mobile ? "MOBILE" : "STATIONARY", velocidade_kmh);

    while (1) {
        if (vehicle_node.is_mobile) {
            waypoint_t target = campus_map[target_idx];
            
            double d_lat = target.lat - vehicle_node.lat;
            double d_lon = target.lon - vehicle_node.lon;
            double dist = sqrt(d_lat * d_lat + d_lon * d_lon);

            // Se chegou ao ponto (tolerância baseada na velocidade), mira o próximo
            if (dist < (vehicle_node.v_base_delta * 1.1)) {
                target_idx = (target_idx + 1) % MAP_SIZE;
            } else {
                // Avança em direção ao alvo
                vehicle_node.lat += (d_lat / dist) * vehicle_node.v_base_delta;
                vehicle_node.lon += (d_lon / dist) * vehicle_node.v_base_delta;
            }
        }

        // Reporte de status via JSON para o Orquestrador/Scheduler
        char status_json[256];
        snprintf(status_json, sizeof(status_json), 
                "{\"id\":\"%s\",\"latitude\":%.6f,\"longitude\":%.6f,\"speed\":%.1f}", 
                vehicle_node.id, vehicle_node.lat, vehicle_node.lon, velocidade_kmh);
        
        midd4vc_register(v_node, status_json);

        sleep(1); 
    }
    
    return 0;
}