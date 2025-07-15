#include <SDL2/SDL.h>
#include <vector>

const int WIDTH=1280;
const int HEIGHT=720;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

bool init() {
	bool success = true;
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not init SDL: %s\n", SDL_GetError());
		success = false;
	} else {
		window = SDL_CreateWindow("grav-sim", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
		if (window == NULL) {
			printf("Could not create window: %s\n", SDL_GetError());
			success = false;
		} else {
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
			if (renderer == NULL) {
				printf("Could not create renderer: %s\n", SDL_GetError());
				success = false;
			}
		}

	}
	return success;
}

void close() {
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_Quit();
}

bool running = true;
SDL_Event event;

void handleEvents() {
    while(SDL_PollEvent(&event)) { 
        switch(event.type) { 
            case SDL_QUIT:
                running = false; 
                printf("Quitting\n");
                break;
            default:
                break;
        }
    }
}

// x'=ax*x+bx*y+cx
// y'=ay*x+by*y+cy
float[] affineTransform(float ax, float bx, float ay, float by, float cx, float cy, float x, float y) {
	float new_x = ax * x + bx * y + cx;
	float new_y = ay * x + by * y + cy;
	float result[] = {new_x, new_y};
	return result;
}

// PARAMTERY DO GENERACJI FRAKTALA
int n = 4; // liczba transformacji
// wszystkie parametry transformacji
// dla paproci Barnsleya
float ax[n] = {0.24, 0.22, 0.14, 0.8};
float bx[n] = {-0.007, -0.33, -0.36, 0.1};
float ay[n] = {0.007, 0.36, -0.38, -0.1};
float by[n] = {0.015, 0.1, -0.1, 0.8};
float cx[n] = {0, 0.54, 1.4, 1.6};
float cy[n] = {0, 0, 0, 0};

int N = 10; // liczba iteracji
	   
// zbiór punktów
std::vector<float> x, y;

void setupSet() {

}

void generateFractal() {
}

void draw() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer); 

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); 

	SDL_RenderPresent(renderer); 
}

void loop() {
	while (running) {
		handleEvents();
	}
}

int main(int argc, char* argv[]) {
	if (!init()) {
		return 1;
	}
	generateFractal();
	loop();
	close();
	return 0;
}
