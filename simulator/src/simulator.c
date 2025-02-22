#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>

#define PORT 8080

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

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

    for (int i = 0; i <= steps; i++) {
        if ((i / dashLength) % 2 < 1) {
            SDL_RenderDrawPoint(renderer, (int)x, (int)y);
        }
        x += xIncrement;
        y += yIncrement;
    }
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
    SDL_SetRenderDrawColor(renderer, 247, 233, 23, 255);
    
    DrawDashedLine(renderer, 0, 250, 150, 250, 10);
    DrawDashedLine(renderer, 0, 350, 150, 350, 10);

    DrawDashedLine(renderer, 450, 250, 600, 250, 10);
    DrawDashedLine(renderer, 450, 350, 600, 350, 10);

    DrawDashedLine(renderer, 250, 0, 250, 150, 10);
    DrawDashedLine(renderer, 350, 0, 350, 150, 10);

    DrawDashedLine(renderer, 250, 450, 250, 600, 10);
    DrawDashedLine(renderer, 350, 450, 350, 600, 10);

    DrawTrafficLight(renderer, 175, 255, 0, "vertical");
    DrawTrafficLight(renderer, 395, 255, 1, "vertical");
    DrawTrafficLight(renderer, 255, 175, 0, "horizontal");
    DrawTrafficLight(renderer, 255, 395, 1, "horizontal");
 
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
    // A road lanes (A1, A2, A3)
    { {150, 250, 0, 150}, {250, 350, 0, 150}, {350, 450, 0, 150} }, //check y_end imp!!!
    
    // B road lanes (B1, B2, B3)
    {{350, 450, 450, 600}, {250, 350, 450, 600}, {150, 250, 450, 600}},
    
    // C road lanes (C1, C2, C3)
    { {450, 600, 150, 250}, {450, 600, 250, 350}, {450, 600, 350, 450} },
    
    // D road lanes (D1, D2, D3)
    {  {0, 150, 350, 450}, {0, 150, 250, 350},{0, 150, 150, 250} }
};
   
void getLaneCenter(char road, int lane, int *x, int *y) {
    int roadIndex = road - 'A';  // Convert 'A'-'D' to index 0-3
    int laneIndex = lane - 1;    // Convert 1-3 to index 0-2

    *x = (lanePositions[roadIndex][laneIndex].x_start + lanePositions[roadIndex][laneIndex].x_end) / 2 - 20 / 2;
    *y = (lanePositions[roadIndex][laneIndex].y_start + lanePositions[roadIndex][laneIndex].y_end) / 2 - 20 / 2;

    // Debugging output
    printf("Vehicle starting at Road %c, Lane %d → Center (%d, %d)\n", road, lane, *x, *y);
}

void moveVehicle(Vehicle *vehicle) {
    int targetX, targetY;
    getLaneCenter(vehicle->targetRoad, vehicle->targetLane, &targetX, &targetY);

    int reachedX = (abs(vehicle->rect.x - targetX) <= vehicle->speed);
    int reachedY = (abs(vehicle->rect.y - targetY) <= vehicle->speed);

    // 🌟 Fix: Decide movement priority based on road direction
    if ((vehicle->road_id == 'A' && vehicle->targetRoad == 'C') || 
        (vehicle->road_id == 'B' && vehicle->targetRoad == 'D')) {
        // Move Y first (for A3 → C1 and B3 → D1)
        if (!reachedY) {
            if (vehicle->rect.y < targetY) vehicle->rect.y += vehicle->speed;  // Move down
            else vehicle->rect.y -= vehicle->speed;  // Move up
        } else if (!reachedX) {
            if (vehicle->rect.x < targetX) vehicle->rect.x += vehicle->speed;  // Move right
            else vehicle->rect.x -= vehicle->speed;  // Move left
        }
    } else {
        // Default: Move X first (for D3 → A1 and C3 → B1)
        if (!reachedX) {
            if (vehicle->rect.x < targetX) vehicle->rect.x += vehicle->speed;  // Move right
            else vehicle->rect.x -= vehicle->speed;  // Move left
        } else if (!reachedY) {
            if (vehicle->rect.y < targetY) vehicle->rect.y += vehicle->speed;  // Move down
            else vehicle->rect.y -= vehicle->speed;  // Move up
        }
    }
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

    Vehicle vehicle2 = {
    {0, 0, 20, 20},  // Temporary values (updated below)
    2, 'A', 3, 2,  // ID=2, starts at road 'A', lane 3
    'C', 1                     // Target is road 'C', lane 1
    };

    // Set correct initial position (centered in A3)
    getLaneCenter(vehicle2.road_id, vehicle2.lane, &vehicle2.rect.x, &vehicle2.rect.y);

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
        DrawBackground(renderer);

        drawVehicle(renderer, &vehicle1);
        drawVehicle(renderer, &vehicle2);

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
