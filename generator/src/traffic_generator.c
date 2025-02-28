#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define ROADS 4

typedef struct {
    int vehicle_id;
    char road_id;
    int lane;
    int speed;
    int rect_w;
    int rect_h;
    char targetRoad;
    int targetLane;
} Vehicle;

void send_data(int socket_fd, Vehicle *data) {
    if (send(socket_fd, data, sizeof(*data), 0) < 0) {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
    printf("Data sent to client: Vehicle ID: %d on Road %c Lane %d -> Target %c Lane %d\n",
           data->vehicle_id, data->road_id, data->lane, data->targetRoad, data->targetLane);
}

int create_socket() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sock_fd;
}

void set_socket_options(int sock_fd) {
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
}

void bind_socket(int sock_fd, struct sockaddr_in *address) {
    if (bind(sock_fd, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
}

void listen_for_connections(int sock_fd) {
    if (listen(sock_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
}

int accept_connection(int sock_fd, struct sockaddr_in *address) {
    int addrlen = sizeof(*address);
    int new_socket = accept(sock_fd, (struct sockaddr *)address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    return new_socket;
}

char getRandomRoad() {
    char roads[] = {'A', 'B', 'C', 'D'};
    return roads[rand() % ROADS];
}

Vehicle generate_vehicle() {
    static int vehicle_counter = 0;
    Vehicle v;
    v.vehicle_id = ++vehicle_counter;
    v.road_id = getRandomRoad();
    v.lane = (rand() % 2) + 2;  // Lane 2 or 3
    v.speed = 2;
    v.rect_w = 20;
    v.rect_h = 20;

    if (v.lane == 2) {
        if (v.road_id == 'A') v.targetRoad = 'B';
        else if (v.road_id == 'B') v.targetRoad = 'A';
        else if (v.road_id == 'C') v.targetRoad = 'D';
        else if (v.road_id == 'D') v.targetRoad = 'C';
    } else if (v.lane == 3) {
        if (v.road_id == 'A') v.targetRoad = 'C';
        else if (v.road_id == 'B') v.targetRoad = 'D';
        else if (v.road_id == 'C') v.targetRoad = 'B';
        else if (v.road_id == 'D') v.targetRoad = 'A';
        v.targetLane = 1;
    }

    return v;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    srand(time(NULL));

    server_fd = create_socket();
    set_socket_options(server_fd);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind_socket(server_fd, &address);
    printf("Server listening on port %d\n", PORT);
    listen_for_connections(server_fd);

    int new_socket = accept_connection(server_fd, &address);
    printf("Client connected! Waiting to send vehicle data...\n");

    while (1) {
        Vehicle vehicle = generate_vehicle();
        send_data(new_socket, &vehicle);
        sleep(rand() % 3 + 1); // Sleep 1-3 seconds
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
