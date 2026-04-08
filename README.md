# Vehicular Cloud Computing Simulation

This project implements a **Vehicular Cloud Computing (VCC)** environment using the **Midd4VCC** middleware. It simulates a dynamic ecosystem where vehicles act as mobile resource nodes and application clients (e.g., software, pedestrian nodes) offload computational tasks to the cloud.

The simulation utilizes the **Manhattan Mobility Model** to ensure realistic vehicle movement within the UFPE campus.

---

## Prerequisites

Ensure you have the following dependencies installed:

```bash
sudo apt update
sudo apt install -y build-essential cmake libpaho-mqtt-dev mosquitto mosquitto-clients
```

The MQTT Broker, e.g., Mosquitto must be running.
```bash
sudo systemctl start mosquitto
```
## Compilation

Build the entire project (Server, Vehicle, and Application) using CMake:
```bash
mkdir build && cd build
cmake ..
make
```

## Execution Guide

To run a complete simulation, start the components in the following order:

### 1. Start the Middleware Server (Orchestrator)
```bash
./Midd4VCServer
```

### 2. tart Vehicle Nodes (Resource Providers)
- With Automatically Generated ID:
If you omit the ID, the system automatically generates one in the format v[SPEED]_[RANDOM_NUMBER].
```bash
# Generates an ID like v30_452, v30_891, etc.
./Vehicle 30
```

- With Custom ID:
```bash
./Vehicle 40 Car_01
```

- Static Mode (Car parking at CIn):
```bash
./Vehicle 0 Car_01
```
### 3. Start the Application Client (Task Offloader)
Simulates a user (e.g., a pedestrian at CIn) requesting computational processing.
```bash
./Application
```