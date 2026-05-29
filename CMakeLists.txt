cmake_minimum_required(VERSION 3.10)
project(VehicularCloud C)

# 1. Configurações de Compilação
set(CMAKE_C_STANDARD 99)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -g")

# 2. Localizar Dependências Externas
# Paho MQTT (Necessária para o mqtt_adapter.c)
find_library(MQTT_LIB paho-mqtt3c)
# DL Lib (Necessária para o dlopen no job_catalog.c)
find_library(DL_LIB dl)

# 3. Definir Diretórios de Include
# Isso resolve o erro de "job_catalog.h not found" pois o CMake avisará ao GCC onde procurar
include_directories(
    vehicle
    middleware/distribution
    middleware/infrastructure
    middleware/services
    # middleware/specifics
)

# 4. Agrupar os arquivos do Middleware (O "Cérebro")
set(MIDDLEWARE_SOURCES
    middleware/distribution/midd4vc_client.c
    middleware/distribution/midd4vc_job_codec.c
    middleware/infrastructure/mqtt_adapter.c
    # middleware/specifics/job_catalog.c
)

# 5. Criar o Executável do Veículo
# add_executable(vehicle_node 
#     vehicle/mobile.c 
#     ${MIDDLEWARE_SOURCES}
# )

add_executable(Vehicle 
    vehicle/vehicle.c 
    ${MIDDLEWARE_SOURCES}
)

# 6. Linkar as Bibliotecas
# target_link_libraries(vehicle_node 
#     ${MQTT_LIB} 
#     ${DL_LIB} 
#     m  # math library para cálculos de lat/lon
# )

target_link_libraries(Vehicle 
    ${MQTT_LIB} 
    ${DL_LIB} 
    m  # math library para cálculos de lat/lon
)

# 7. Criar o Executável do Cliente (Simulador de Requisições)
# add_executable(client_app 
#     client/client.c
#     ${MIDDLEWARE_SOURCES}
# )

add_executable(Application 
    client/application.c
    ${MIDDLEWARE_SOURCES}
)

target_link_libraries(Application
    ${MQTT_LIB}
    ${DL_LIB}
    m
)
# target_link_libraries(client_app paho-mqtt3c)
# target_link_libraries(Application paho-mqtt3c)
# target_link_libraries(vehicle_node paho-mqtt3c)
# target_link_libraries(Vehicle paho-mqtt3c)
