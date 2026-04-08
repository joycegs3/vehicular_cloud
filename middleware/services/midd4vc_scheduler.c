#include "midd4vc_scheduler.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define MAX_LOAD 5  

// Converte graus para radianos
static double to_rad(double degree) {
    return degree * (M_PI / 180.0);
}

static double haversine(double lat1, double lon1, double lat2, double lon2) {
    double dLat = to_rad(lat2 - lat1);
    double dLon = to_rad(lon2 - lon1);

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(to_rad(lat1)) * cos(to_rad(lat2)) *
               sin(dLon / 2) * sin(dLon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return 6371000.0 * c; // Retorna a distância em METROS (raio da Terra = 6371km)
}

#define MAX_REACH_DISTANCE 500.0 // Raio de 500 metros para considerar "nuvem local"

vehicle_t* strategy_proximity(vehicle_t* vehicles, int vehicle_count, double lat, double lon, scheduler_ctx_t* ctx) {
    vehicle_t *best = NULL;
    double min_dist = 1e18;

    for (int i = 0; i < vehicle_count; i++) {
        if (!vehicles[i].is_active || vehicles[i].active_jobs >= MAX_LOAD) continue;

        // Cálculo profissional de distância GPS em metros
        double d = haversine(lat, lon, vehicles[i].latitude, vehicles[i].longitude);

        // Se estiver dentro do alcance e for o mais próximo até agora
        if (d < MAX_REACH_DISTANCE && d < min_dist) {
            min_dist = d;
            best = &vehicles[i];
        }
    }

    if (best) {
        printf("[STRATEGY] Proximidade: %s selecionado (Distancia: %.2fm)\n", best->vehicle_id, min_dist);
    }
    
    return best;
}

vehicle_t* strategy_round_robin(vehicle_t* vehicles, int vehicle_count, double lat, double lon, scheduler_ctx_t* ctx) {
    // static int rr_idx = 0;
    if (vehicle_count <= 0) return NULL;
    if (!ctx) return NULL;
    if (!vehicles) return NULL;

    for (int i = 0; i < vehicle_count; i++) {
        //int idx = (rr_idx++) % vehicle_count;
        //// int idx = rr_idx % vehicle_count;
        int idx = (ctx->rr_idx + i) % vehicle_count;
        //rr_idx++;
        ////rr_idx = (idx + 1) % vehicle_count;
        if (vehicles[idx].is_active && vehicles[idx].active_jobs < MAX_LOAD) {
            ctx->rr_idx = (idx + 1) % vehicle_count;
            return &vehicles[idx];
        } 
    }
    return NULL;
}

/*
vehicle_t* strategy_least_loaded(vehicle_t* vehicles, int vehicle_count, double lat, double lon, scheduler_ctx_t* ctx) {
    vehicle_t *best = NULL;
    int min_load = ctx->max_load + 1;
    int best_idx = -1;

    for (int i = 0; i < vehicle_count; i++) {

        int idx = (ctx->rr_idx + i) % vehicle_count;

        if (!vehicles[idx].is_active || vehicles[idx].active_jobs >= ctx->max_load) continue;

        if (vehicles[idx].active_jobs < min_load) {
            min_load = vehicles[idx].active_jobs;
            best = &vehicles[idx];
            best_idx = idx;
            if (min_load == 0) break;
        }
       //if (!best || vehicles[i].active_jobs < best->active_jobs) best = &vehicles[i];
    }

    if (best) {
        ctx->rr_idx = (best_idx + 1) % vehicle_count;;
    }

    return best;
}
    */

vehicle_t* strategy_least_loaded(vehicle_t* vehicles, int vehicle_count, double lat, double lon, scheduler_ctx_t* ctx) {
    if (vehicle_count <= 0) return NULL;

    int min_load = ctx->max_load + 1;
    int candidates[vehicle_count];
    int candidate_count = 0;

    // 1. Encontrar a carga mínima entre os veículos ativos
    for (int i = 0; i < vehicle_count; i++) {
        if (!vehicles[i].is_active || vehicles[i].active_jobs >= ctx->max_load) continue;

        if (vehicles[i].active_jobs < min_load) {
            min_load = vehicles[i].active_jobs;
        }
    }

    // Se nenhum veículo for encontrado abaixo do MAX_LOAD
    if (min_load > ctx->max_load) return NULL;

    // 2. Criar a lista de todos os veículos que têm essa carga mínima (o Least Loaded Resources do Python)
    for (int i = 0; i < vehicle_count; i++) {
        if (vehicles[i].is_active && vehicles[i].active_jobs == min_load) {
            candidates[candidate_count++] = i;
        }
    }

    // 3. Escolha aleatória entre os candidatos (o random.choice do Python)
    int chosen_idx = candidates[rand() % candidate_count];

    return &vehicles[chosen_idx];
}