#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <stdbool.h>

#define PORT 8080

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

static int northSouthGreen = 0; // initial state
static int eastWestGreen = 1;

typedef struct{
    SDL_Rect rect;
    int vehicle_id;
    char road_id;
    int lane;
    int speed;
    char targetRoad;
    int targetLane; 
} Vehicle;

void drawVehicle(SDL_Renderer *renderer, Vehicle *vehicle) {

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);  // Red vehicle
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

void DrawLaneMarking(SDL_Renderer *renderer){
    SDL_Color laneMarking = {247, 233, 23, 255};
    SDL_Color laneMarkingRed = {247, 0, 0, 255};
    
    SDL_SetRenderDrawColor(renderer, laneMarking.r, laneMarking.g, laneMarking.b, laneMarking.a);
    // Horizontal road markings
    DrawDashedLine(renderer, 0, 250, 150, 250, 10);
    DrawDashedLine(renderer, 0, 350, 150, 350, 10);
    DrawDashedLine(renderer, 450, 250, 600, 250, 10);
    DrawDashedLine(renderer, 450, 350, 600, 350, 10);

    SDL_SetRenderDrawColor(renderer, laneMarkingRed.r, laneMarkingRed.g, laneMarkingRed.b, laneMarkingRed.a);
    // Split middle lane markings - horizontal
    DrawDashedLine(renderer, 600, 300, 450, 300, 0);  // Middle divider
    DrawDashedLine(renderer, 0, 300, 150, 300, 0);  // Middle divider

    SDL_SetRenderDrawColor(renderer, laneMarking.r, laneMarking.g, laneMarking.b, laneMarking.a);
    // Vertical road markings
    DrawDashedLine(renderer, 250, 0, 250, 150, 10);
    DrawDashedLine(renderer, 350, 0, 350, 150, 10);
    DrawDashedLine(renderer, 250, 450, 250, 600, 10);
    DrawDashedLine(renderer, 350, 450, 350, 600, 10);

    SDL_SetRenderDrawColor(renderer, laneMarkingRed.r, laneMarkingRed.g, laneMarkingRed.b, laneMarkingRed.a);
    // Split middle lane markings - vertical
    DrawDashedLine(renderer, 300, 0, 300, 150, 0);  // Middle divider
    DrawDashedLine(renderer, 300, 600, 300, 450, 0);    // Middle divider
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

void TrafficLightState(SDL_Renderer *renderer, int northSouthGreen, int eastWestGreen) {
    // Vertical lights control North-South traffic
    DrawTrafficLight(renderer, 175, 255, northSouthGreen, "vertical"); // North-South left lane
    DrawTrafficLight(renderer, 395, 255, northSouthGreen, "vertical"); // North-South right lane
    // Horizontal lights control East-West traffic
    DrawTrafficLight(renderer, 255, 175, eastWestGreen, "horizontal"); // East-West upper lane
    DrawTrafficLight(renderer, 255, 395, eastWestGreen, "horizontal"); // East-West lower lane
}

void DrawBackground(SDL_Renderer *renderer) {
    // Set background color (green for grass)
    SDL_SetRenderDrawColor(renderer, 34, 139, 34, 255);
    SDL_RenderClear(renderer);

    // Set road color (gray)
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);

    // Horizontal road
    SDL_Rect horizontalRoad = {0, 150, 600, 300}; 
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Vertical road
    SDL_Rect verticalRoad = {150, 0, 300, 600}; 
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Dashed lines for lane markings (yellow)
    DrawLaneMarking(renderer);
    // Traffic lights
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
  /* Vehicles Stopping Logic */
  int shouldStop = 0;  
  int stopX = vehicle->rect.x;  
  int stopY = vehicle->rect.y;  

  //For lane 2 only 
  if(vehicle->lane == 2 ){
    if(vehicle->road_id == 'A' && northSouthGreen){
      stopY = 150 - 20;
      if(vehicle->rect.y >= stopY){
        shouldStop = 1;
      }
    }

    if(vehicle->road_id == 'B' && northSouthGreen ){
      stopY=450;
      if(vehicle->rect.y <= stopY){
        shouldStop = 1; 
      }
    }

    if(vehicle->road_id == 'D' && eastWestGreen){
      stopX=150-20; 
      if(vehicle->rect.x >= stopX){
        shouldStop = 1;
      }
    }

    if(vehicle->road_id == 'C' && eastWestGreen){
      stopX= 450; 
      if(vehicle->rect.x <= stopX){
        shouldStop = 1;
      }
    }
  }

  if(shouldStop){
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

int main() {
    // Socket related code commented out during the development of UI elements
    // int sock = create_socket();

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


    // connect_to_server(sock, "127.0.0.1");

Vehicle vehicle1 = {
    {0, 0, 20, 20},  // Temporary position (updated below)
    1, 'D', 3, 2,    // ID=1, starts at road 'D', lane 3, speed=3
    'A', 1          // Target is road 'A', lane 1
    };
    getLaneCenter(vehicle1.road_id, vehicle1.lane, &vehicle1.rect.x, &vehicle1.rect.y);

    Vehicle vehicle2 = {{0, 0, 20, 20}, 2, 'A', 3, 2, 'C', 1};
    getLaneCenter(vehicle2.road_id, vehicle2.lane, &vehicle2.rect.x, &vehicle2.rect.y);

    Vehicle vehicle3 = {{0, 0, 20, 20}, 3, 'C', 3, 2, 'B', 1};
    getLaneCenter(vehicle3.road_id, vehicle3.lane, &vehicle3.rect.x, &vehicle3.rect.y);

    Vehicle vehicle4 = {{0, 0, 20, 20}, 4, 'B', 3, 2, 'D', 1};
    getLaneCenter(vehicle4.road_id, vehicle4.lane, &vehicle4.rect.x, &vehicle4.rect.y);

    Vehicle vehicle5 = {{0, 0, 20, 20}, 5, 'D', 2, 2, 'C', 2};
    getLaneCenter(vehicle5.road_id, vehicle5.lane, &vehicle5.rect.x, &vehicle5.rect.y);

    Vehicle vehicle6 = {{0, 0, 20, 20}, 6, 'C', 2, 2, 'D', 2};
    getLaneCenter(vehicle6.road_id, vehicle6.lane, &vehicle6.rect.x, &vehicle6.rect.y);

    Vehicle vehicle7 = {{0, 0, 20, 20}, 7, 'A', 2, 2, 'B', 2};
    getLaneCenter(vehicle7.road_id, vehicle7.lane, &vehicle7.rect.x, &vehicle7.rect.y);

    Vehicle vehicle8 = {{0, 0, 20, 20}, 8, 'B', 2, 2, 'A', 2};
    getLaneCenter(vehicle8.road_id, vehicle8.lane, &vehicle8.rect.x, &vehicle8.rect.y);

    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN)) {
                running = 0;
            }
        }
        moveVehicle(&vehicle1);
        moveVehicle(&vehicle2);
        moveVehicle(&vehicle3);
        moveVehicle(&vehicle4);
        moveVehicle(&vehicle5);
        moveVehicle(&vehicle6);
        moveVehicle(&vehicle7);
        moveVehicle(&vehicle8);

        DrawBackground(renderer);
        
        TrafficLightState(renderer, northSouthGreen, eastWestGreen);

        drawVehicle(renderer, &vehicle1);
        drawVehicle(renderer, &vehicle2);
        drawVehicle(renderer, &vehicle3);
        drawVehicle(renderer, &vehicle4);
        drawVehicle(renderer, &vehicle5);
        drawVehicle(renderer, &vehicle6);
        drawVehicle(renderer, &vehicle7);
        drawVehicle(renderer, &vehicle8);
        SDL_RenderPresent(renderer);
        SDL_Delay(30);

    }

    // receive_data(sock);

    // Close socket
    // close(sock);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
