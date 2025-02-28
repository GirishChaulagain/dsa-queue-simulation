#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>

#define PORT 8080
#define MAX_VEHICLES 100
const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

static int udGreen = 0; // initial state
static int rlGreen = 1;

typedef struct{
    SDL_Rect rect;
    int vehicle_id;
    char road_id;
    int lane;
    int speed;
    char targetRoad;
    int targetLane; 
} Vehicle;

typedef struct {
    Vehicle *vehicles[MAX_VEHICLES];
    int front;
    int rear;
    int size;
} VehicleQueue;


    VehicleQueue queue;

void initQueue(VehicleQueue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

int isQueueFull(VehicleQueue *q) {
    return q->size >= MAX_VEHICLES;
}

int isQueueEmpty(VehicleQueue *q) {
    return q->size == 0;
}

void enqueue(VehicleQueue *q, Vehicle *v) {
    if (isQueueFull(q)) {
        printf("Queue is full! Cannot enqueue vehicle %d\n", v->vehicle_id);
        free(v);
        return;
    }
    q->rear = (q->rear + 1) % MAX_VEHICLES;
    q->vehicles[q->rear] = v;
    q->size++;
    printf("Enqueued vehicle %d on Road %c Lane %d\n", v->vehicle_id, v->road_id, v->lane);
}

Vehicle* dequeue(VehicleQueue *q) {
    if (isQueueEmpty(q)) {
        printf("Queue is empty!\n");
        return NULL;
    }
    Vehicle *v = q->vehicles[q->front];
    q->front = (q->front + 1) % MAX_VEHICLES;
    q->size--;
    return v;
}

int create_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Set socket to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    return sock;
}

void connect_to_server(int sock) {
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
    }
}

void drawVehicle(SDL_Renderer *renderer, Vehicle *vehicle) {
    // Change: Car color from red (255, 0, 0, 255) to blue (0, 0, 255, 255)
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    printf("Drawing vehicle %d at (%d, %d) with size (%d, %d)\n", 
           vehicle->vehicle_id, vehicle->rect.x, vehicle->rect.y, vehicle->rect.w, vehicle->rect.h);
    SDL_RenderFillRect(renderer, &vehicle->rect);
}

int InitializeSDL(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }
    return 0;
}

SDL_Window* CreateWindow(const char *title, int width, int height) {
    SDL_Window *window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    }
    return window;
}

SDL_Renderer* CreateRenderer(SDL_Window *window) {
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    }
    return renderer;
}

void DrawDashedLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2, int dashLength) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    int steps = (abs(dx) > abs(dy)) ? abs(dx) : abs(dy);

    float xIncrement = (float)dx / steps;
    float yIncrement = (float)dy / steps;

    float x = x1;
    float y = y1;

    if (dashLength == 0) {
        // If dashLength is 0, draw a continuous line
        for (int i = 0; i <= steps; i++) {
            SDL_RenderDrawPoint(renderer, (int)x, (int)y);
            x += xIncrement;
            y += yIncrement;
        }
    } else {
        // Draw a dashed line
        for (int i = 0; i <= steps; i++) {
            if ((i / dashLength) % 2 < 1) {
                SDL_RenderDrawPoint(renderer, (int)x, (int)y);
            }
            x += xIncrement;
            y += yIncrement;
        }
    }
}

void DrawLaneMarking(SDL_Renderer *renderer) {
    SDL_Color laneMarking = {247, 233, 23, 255}; // Yellow (unchanged for dashed lines)
    // Change: Use yellow for all lines, including middle dividers (previously red)
    SDL_Color laneMarkingYellow = {247, 233, 23, 255}; // Was laneMarkingRed

    SDL_SetRenderDrawColor(renderer, laneMarking.r, laneMarking.g, laneMarking.b, laneMarking.a);
    DrawDashedLine(renderer, 0, 250, 150, 250, 10);
    DrawDashedLine(renderer, 0, 350, 150, 350, 10);
    DrawDashedLine(renderer, 450, 250, 600, 250, 10);
    DrawDashedLine(renderer, 450, 350, 600, 350, 10);

    // Change: Middle dividers now yellow instead of red
    SDL_SetRenderDrawColor(renderer, laneMarkingYellow.r, laneMarkingYellow.g, laneMarkingYellow.b, laneMarkingYellow.a);
    DrawDashedLine(renderer, 600, 300, 450, 300, 0);
    DrawDashedLine(renderer, 0, 300, 150, 300, 0);

    SDL_SetRenderDrawColor(renderer, laneMarking.r, laneMarking.g, laneMarking.b, laneMarking.a);
    DrawDashedLine(renderer, 250, 0, 250, 150, 10);
    DrawDashedLine(renderer, 350, 0, 350, 150, 10);
    DrawDashedLine(renderer, 250, 450, 250, 600, 10);
    DrawDashedLine(renderer, 350, 450, 350, 600, 10);

    SDL_SetRenderDrawColor(renderer, laneMarkingYellow.r, laneMarkingYellow.g, laneMarkingYellow.b, laneMarkingYellow.a);
    DrawDashedLine(renderer, 300, 0, 300, 150, 0);
    DrawDashedLine(renderer, 300, 600, 300, 450, 0);
}

void DrawTrafficLight(SDL_Renderer *renderer, int XPos, int YPos, int isGreen, char *orientation) {

    const int width = 30;
    const int height = 90;

    if (isGreen) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }

    SDL_Rect trafficLightRect;

    if (strcmp(orientation, "vertical") == 0) {
        trafficLightRect = (SDL_Rect){XPos, YPos, width, height}; // Horizontal orientation
    } else if (strcmp(orientation, "horizontal") == 0) {
        trafficLightRect = (SDL_Rect){XPos, YPos, height, width}; // Vertical orientation
    } else {
        printf("Invalid orientation: %s\n", orientation);
        return;
    }

    SDL_RenderFillRect(renderer, &trafficLightRect);
}

void TrafficLightState(SDL_Renderer *renderer, int udGreen, int rlGreen) {
    // Vertical lights control North-South traffic
    DrawTrafficLight(renderer, 175, 255, udGreen, "vertical"); // North-South left lane
    DrawTrafficLight(renderer, 395, 255, udGreen, "vertical"); // North-South right lane
    // Horizontal lights control East-West traffic
    DrawTrafficLight(renderer, 255, 175, rlGreen, "horizontal"); // East-West upper lane
    DrawTrafficLight(renderer, 255, 395, rlGreen, "horizontal"); // East-West lower lane
}

void DrawBackground(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255); // Grass (unchanged)
    SDL_RenderClear(renderer);

    // Change: Road color from dark grey (50, 50, 50) to lighter grey (150, 150, 150)
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);

    SDL_Rect horizontalRoad = {0, 150, 600, 300};
    SDL_RenderFillRect(renderer, &horizontalRoad);
    SDL_Rect verticalRoad = {150, 0, 300, 600};
    SDL_RenderFillRect(renderer, &verticalRoad);
    DrawLaneMarking(renderer);
}



/*void receive_data(int sock) {*/
/*    Vehicle received_data;*/
/**/
/*    while (1) {*/
/*        ssize_t bytes_received = recv(sock, &received_data, sizeof(received_data), 0);*/
/**/
/*        if (bytes_received == 0) {*/
/*            printf("Server disconnected.\n");*/
/*            break;*/
/*        } else if (bytes_received < 0) {*/
/*            perror("Receive failed");*/
/*            break;*/
/*        }*/
/**/
/*        printf("Received Data: Vehicle ID %d on Road %c Lane %cL%d\n",*/
/*               received_data.vehicle_id, received_data.road_id, received_data.road_id, received_data.lane);*/
/**/
/*        sleep(1);*/
/*    }*/
/*}*/

    static SDL_Window *window = NULL;
    static SDL_Renderer *renderer = NULL;

    typedef struct{ 
        int x_start, x_end;
        int y_start, y_end;
    } LanePosition;

LanePosition lanePositions[4][3] = {
    // A road lanes (North to South) (A1, A2, A3)
    // A2 is split into two - leftmost (outgoing), rightmost (incoming)
    { {150, 250, -30, -30}, {270, 300, -30, -30}, {350, 450, -30, -30} },

    // B road lanes (South to North) (B1, B2, B3)
    // B2 is split into two - leftmost (incoming), rightmost (outgoing)
    { {350, 450, 630, 630}, {300, 330, 630, 630}, {150, 250, 630, 630} },

    // C road lanes (East to West) (C1, C2, C3)
    // C2 is split into two - uppermost (outgoing), lowermost (incoming)
    { {630, 630, 150, 250}, {630, 630, 270, 300}, {630, 630, 350, 450} },

    // D road lanes (West to East) (D1, D2, D3)
    // D2 is split into two - uppermost (incoming), lowermost (outgoing)
    { {-30, -30, 350, 450}, {-30, -30, 300, 330}, {-30, -30, 150, 250} }
};

void getLaneCenter(char road, int lane, int *x, int *y) {
    int roadIndex = road - 'A';  // Convert 'A'-'D' to index 0-3
    int laneIndex = lane - 1;    // Convert 1-3 to index 0-2

    int middleLaneOffset = 0;
    if (lane == 2) {
        if (road == 'A') {
            middleLaneOffset = -15; // Move left for outgoing
        } else if (road == 'B') {
            middleLaneOffset = 15;  // Move right for incoming
        } else if (road == 'D') {
            middleLaneOffset = 15;  // Move down for outgoing
        } else if (road == 'C') {
            middleLaneOffset = -15; // Move up for incoming
        }
    }

    if (road == 'A' || road == 'B') {
        *x = ((lanePositions[roadIndex][laneIndex].x_start + lanePositions[roadIndex][laneIndex].x_end) / 2) + middleLaneOffset;
        *y = (road == 'A') ? -30 : SCREEN_HEIGHT + 10;
    } else {
        *x = (road == 'C') ? SCREEN_WIDTH + 10 : -30;
        *y = ((lanePositions[roadIndex][laneIndex].y_start + lanePositions[roadIndex][laneIndex].y_end) / 2) + middleLaneOffset;
    }

    printf("Road: %c, Lane: %d, X: %d, Y: %d, Offset: %d\n", road, lane, *x, *y, middleLaneOffset);
}

void moveVehicle(Vehicle *vehicle) {
    int targetX, targetY;
    getLaneCenter(vehicle->targetRoad, vehicle->targetLane, &targetX, &targetY);

    if (vehicle->targetLane == 1) {
        if (!((vehicle->road_id == 'D' && vehicle->lane == 3 && vehicle->targetRoad == 'A') ||
              (vehicle->road_id == 'A' && vehicle->lane == 3 && vehicle->targetRoad == 'C') ||
              (vehicle->road_id == 'C' && vehicle->lane == 3 && vehicle->targetRoad == 'B') ||
              (vehicle->road_id == 'B' && vehicle->lane == 3 && vehicle->targetRoad == 'D'))) {
            printf("Vehicle %d is not allowed to move to Lane 1! Stopping movement.\n", vehicle->vehicle_id);
            return;
        }
    }

    if (vehicle->targetLane == 2) {
        if (!((vehicle->road_id == 'A' && vehicle->lane == 2 && vehicle->targetRoad == 'B') ||
              (vehicle->road_id == 'A' && vehicle->lane == 2 && vehicle->targetRoad == 'C') ||
              (vehicle->road_id == 'C' && vehicle->lane == 2 && vehicle->targetRoad == 'A') ||
              (vehicle->road_id == 'C' && vehicle->lane == 2 && vehicle->targetRoad == 'D') ||
              (vehicle->road_id == 'B' && vehicle->lane == 2 && vehicle->targetRoad == 'A') ||
              (vehicle->road_id == 'B' && vehicle->lane == 2 && vehicle->targetRoad == 'D') ||
              (vehicle->road_id == 'D' && vehicle->lane == 2 && vehicle->targetRoad == 'C') ||
              (vehicle->road_id == 'D' && vehicle->lane == 2 && vehicle->targetRoad == 'B'))) {
            printf("Vehicle %d is not allowed to move to Lane 2! Stopping movement.\n", vehicle->vehicle_id);
            return;
        }
    }
   /*Vehicle Stopping Logic */
  int shouldStop = 0;  
  int stopX = vehicle->rect.x;  
  int stopY = vehicle->rect.y;  

  //For lane 2 only  

  //For lane 2 only 
  if (vehicle->lane == 2) {
    if (vehicle->road_id == 'A' && udGreen) {
      stopY = 150 - 20;
      if (vehicle->rect.y == stopY) {
        shouldStop = 1;
      } else {
        shouldStop =0;
      }
    }

    if (vehicle->road_id == 'B' && udGreen) {
      stopY = 450;
      if (vehicle->rect.y == stopY) {
        shouldStop = 1;
      }else{
        shouldStop =0; 
      }
    }

    if (vehicle->road_id == 'D' && rlGreen) {
      stopX = 150 - 20;
      if (vehicle->rect.x == stopX) {
        shouldStop = 1;
      }else{
        shouldStop=0;
      }
    }

    if (vehicle->road_id == 'C' && rlGreen) {
      stopX = 450;
      if (vehicle->rect.x == stopX) {
        shouldStop = 1;
      }else{
        shouldStop = 0;
      }
    }
  }

    if (shouldStop) {
        vehicle->rect.x = stopX;
        vehicle->rect.y = stopY;
        printf("Vehicle %d stopped at (%d, %d) due to red light\n",
               vehicle->vehicle_id, vehicle->rect.x, vehicle->rect.y);
        return;
    }
    int reachedX = (abs(vehicle->rect.x - targetX) <= vehicle->speed);
    int reachedY = (abs(vehicle->rect.y - targetY) <= vehicle->speed);

    // Prioritize movement direction based on road layout
    if ((vehicle->road_id == 'A' && vehicle->targetRoad == 'C') || 
        (vehicle->road_id == 'B' && vehicle->targetRoad == 'D')) {
        // Move Y first
        if (!reachedY) {
            vehicle->rect.y += (vehicle->rect.y < targetY) ? vehicle->speed : -vehicle->speed;
        } else if (!reachedX) {
            vehicle->rect.x += (vehicle->rect.x < targetX) ? vehicle->speed : -vehicle->speed;
        }
    } else {
        // Move X first
        if (!reachedX) {
            vehicle->rect.x += (vehicle->rect.x < targetX) ? vehicle->speed : -vehicle->speed;
        } else if (!reachedY) {
            vehicle->rect.y += (vehicle->rect.y < targetY) ? vehicle->speed : -vehicle->speed;
        }
    }

    // Snap to target position
    if (reachedX) vehicle->rect.x = targetX;
    if (reachedY) vehicle->rect.y = targetY;

    if (reachedX && reachedY) {
        vehicle->road_id = vehicle->targetRoad;
        vehicle->lane = vehicle->targetLane;
    }
    // Debugging Output
    printf("Vehicle %d Position: (%d, %d) Target: (%d, %d)\n", 
            vehicle->vehicle_id, vehicle->rect.x, vehicle->rect.y, targetX, targetY);
}

Uint32 lastSwitchTime = 0;

void updateTrafficLights() {
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastSwitchTime > 8555) {
        udGreen = !udGreen;
        rlGreen = !rlGreen;
        lastSwitchTime = currentTime;
        printf("Traffic Light Changed! North-South: %d, East-West: %d\n", udGreen, rlGreen);
    }
}

void receive_data(int sock) {
    Vehicle received_data;
    ssize_t bytes_received = recv(sock, &received_data, sizeof(received_data), MSG_DONTWAIT);
    if (bytes_received > 0) {
        Vehicle *v = (Vehicle *)malloc(sizeof(Vehicle));
        v->vehicle_id = received_data.vehicle_id;
        v->road_id = received_data.road_id;
        v->lane = received_data.lane;
        v->speed = received_data.speed;
        v->rect.w = 20;  // Hardcoded as in original
        v->rect.h = 20;  // Hardcoded as in original
        v->targetRoad = received_data.targetRoad;
        v->targetLane = received_data.targetLane;
        getLaneCenter(v->road_id, v->lane, &v->rect.x, &v->rect.y);
    } else if (bytes_received == 0) {
        printf("Server disconnected\n");
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("Receive failed");
    }
}


int main() {
    // Socket related code commented out during the development of UI elements
     int sock = create_socket();

    if (InitializeSDL() < 0) {
        return 1;
    }
    window = CreateWindow("Traffic Simulator", SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!window) {   
        return 1;
    }
    renderer = CreateRenderer(window);
    if (!renderer) {
        return 1;
    }
    VehicleQueue queue;
    initQueue(&queue);

     connect_to_server(sock);

    /*Vehicle *v1 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v1->vehicle_id = 1;*/
    /*v1->road_id = 'C';*/
    /*v1->lane = 2;*/
    /*v1->speed = 2;*/
    /*v1->rect.w = 20;*/
    /*v1->rect.h = 20;*/
    /*v1->targetRoad = 'A';*/
    /*v1->targetLane = 2;*/
    /*getLaneCenter(v1->road_id, v1->lane, &v1->rect.x, &v1->rect.y);*/
    /*enqueue(&queue, v1);*/
    /**/
    /*Vehicle *v2 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v2->vehicle_id = 2;*/
    /*v2->road_id = 'D';*/
    /*v2->lane = 2;*/
    /*v2->speed = 2;*/
    /*v2->rect.w = 20;*/
    /*v2->rect.h = 20;*/
    /*v2->targetRoad = 'B';*/
    /*v2->targetLane = 2;*/
    /*getLaneCenter(v2->road_id, v2->lane, &v2->rect.x, &v2->rect.y);*/
    /*enqueue(&queue, v2);*/
    /**/
    /*Vehicle *v3 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v3->vehicle_id = 3;*/
    /*v3->road_id = 'A';*/
    /*v3->lane = 2;*/
    /*v3->speed = 2;*/
    /*v3->rect.w = 20;*/
    /*v3->rect.h = 20;*/
    /*v3->targetRoad = 'D';*/
    /*v3->targetLane = 2;*/
    /*getLaneCenter(v3->road_id, v3->lane, &v3->rect.x, &v3->rect.y);*/
    /*enqueue(&queue, v3);*/
    /**/
    /**/
    /*Vehicle *v5 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v5->vehicle_id = 5;*/
    /*v5->road_id = 'D';*/
    /*v5->lane = 3;*/
    /*v5->speed = 2;*/
    /*v5->rect.w = 20;*/
    /*v5->rect.h = 20;*/
    /*v5->targetRoad = 'A';*/
    /*v5->targetLane = 1;*/
    /*getLaneCenter(v5->road_id, v5->lane, &v5->rect.x, &v5->rect.y);*/
    /*enqueue(&queue, v5);*/
    /**/
    /*Vehicle *v6 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v6->vehicle_id = 6;*/
    /*v6->road_id = 'C';*/
    /*v6->lane = 3;*/
    /*v6->speed = 2;*/
    /*v6->rect.w = 20;*/
    /*v6->rect.h = 20;*/
    /*v6->targetRoad = 'B';*/
    /*v6->targetLane = 1;*/
    /*getLaneCenter(v6->road_id, v6->lane, &v6->rect.x, &v6->rect.y);*/
    /*enqueue(&queue, v6);*/
    /**/
    /*Vehicle *v7 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v7->vehicle_id = 7;*/
    /*v7->road_id = 'A';*/
    /*v7->lane = 3;*/
    /*v7->speed = 2;*/
    /*v7->rect.w = 20;*/
    /*v7->rect.h = 20;*/
    /*v7->targetRoad = 'C';*/
    /*v7->targetLane = 1;*/
    /*getLaneCenter(v7->road_id, v7->lane, &v7->rect.x, &v7->rect.y);*/
    /*enqueue(&queue, v7);*/
    /**/
    /*Vehicle *v8 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v8->vehicle_id = 8;*/
    /*v8->road_id = 'B';*/
    /*v8->lane = 3;*/
    /*v8->speed = 2;*/
    /*v8->rect.w = 20;*/
    /*v8->rect.h = 20;*/
    /*v8->targetRoad = 'D';*/
    /*v8->targetLane = 1;*/
    /*getLaneCenter(v8->road_id, v8->lane, &v8->rect.x, &v8->rect.y);*/
    /*enqueue(&queue, v8);*/
    /**/
    /*Vehicle *v9 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v9->vehicle_id = 9;*/
    /*v9->road_id = 'A';*/
    /*v9->lane = 2;*/
    /*v9->speed = 2;*/
    /*v9->rect.w = 20;*/
    /*v9->rect.h = 20;*/
    /*v9->targetRoad = 'B';*/
    /*v9->targetLane = 2;*/
    /*getLaneCenter(v9->road_id, v9->lane, &v9->rect.x, &v9->rect.y);*/
    /*enqueue(&queue, v9);*/
    /**/
    /*Vehicle *v10 = (Vehicle *)malloc(sizeof(Vehicle));*/
    /*v10->vehicle_id = 10;*/
    /*v10->road_id = 'B';*/
    /*v10->lane = 2;*/
    /*v10->speed = 2;*/
    /*v10->rect.w = 20;*/
    /*v10->rect.h = 20;*/
    /*v10->targetRoad = 'A';*/
    /*v10->targetLane = 2;*/
    /*getLaneCenter(v10->road_id, v10->lane, &v10->rect.x, &v10->rect.y);*/
    /*enqueue(&queue, v10);*/

    Vehicle *active_vehicles[MAX_VEHICLES] = {0};
    int num_active_vehicles = 0;
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN)) {
                running = 0;
            }
        }

        receive_data(sock);

        while (!isQueueEmpty(&queue) && num_active_vehicles < MAX_VEHICLES) {
            Vehicle *v = dequeue(&queue);
            active_vehicles[num_active_vehicles++] = v;
        }

        updateTrafficLights();

for (int i = 0; i < num_active_vehicles; i++) {
            if (active_vehicles[i]) {
                moveVehicle(active_vehicles[i]);
                int targetX, targetY;
                getLaneCenter(active_vehicles[i]->targetRoad, active_vehicles[i]->targetLane, &targetX, &targetY);
                if (abs(active_vehicles[i]->rect.x - targetX) <= active_vehicles[i]->speed && 
                    abs(active_vehicles[i]->rect.y - targetY) <= active_vehicles[i]->speed) {
                    printf("Vehicle %d reached target and is removed.\n", active_vehicles[i]->vehicle_id);
                    free(active_vehicles[i]);
                    active_vehicles[i] = NULL;
                }
            }
        }
        int write_idx = 0;
        for (int i = 0; i < num_active_vehicles; i++) {
            if (active_vehicles[i] != NULL) {
                active_vehicles[write_idx++] = active_vehicles[i];
            }
        }
        num_active_vehicles = write_idx;
        DrawBackground(renderer);

        TrafficLightState(renderer, udGreen, rlGreen);




        printf("Rendering %d active vehicles\n", num_active_vehicles);
        for (int i = 0; i < num_active_vehicles; i++) {
            if (active_vehicles[i]) {
                drawVehicle(renderer, active_vehicles[i]);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(30);

    }


    for (int i = 0; i < num_active_vehicles; i++) {
        if (active_vehicles[i]) free(active_vehicles[i]);
    }
    // receive_data(sock);

    // Close socket
     close(sock);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
