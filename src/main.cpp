#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <iostream>

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
std::array<float, 2>  affineTransform(float ax, float bx, float ay, float by, float cx, float cy, float x, float y) {
	float new_x = ax * x + bx * y + cx;
	float new_y = ay * x + by * y + cy;
	return {new_x, new_y};
}

// PARAMTERY DO GENERACJI FRAKTALA
int n = 4; // liczba transformacji
// wszystkie parametry transformacji
// dla paproci Barnsleya
float ax[4] = {0.24, 0.22, 0.14, 0.8};
float bx[4] = {-0.007, -0.33, -0.36, 0.1};
float ay[4] = {0.007, 0.36, -0.38, -0.1};
float by[4] = {0.015, 0.1, -0.1, 0.8};
float cx[4] = {0, 0.54, 1.4, 1.6};
float cy[4] = {0, 0, 0, 0};

int N = 10; // liczba iteracji
	   
// zbiór punktów
std::vector<float> x, y;
int N_points_sqrt = 100; // pierwiastek kwadratowy liczby punktów 

// dodajemy punkty do zbioru
void setupSet() {
	// równomiernie rozłożone w kwadracie o boku 1	
	float sideLength = 1000.0f;
	float spacing = sideLength / (float) N_points_sqrt;
	for (int i = 0; i < N_points_sqrt; i++) {
		for (int j = 0; j < N_points_sqrt; j++) {
			x.push_back(spacing*(float) i);
			y.push_back(spacing*(float) j);
		}
	}
}

void generateFractal() {
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N_points_sqrt*N_points_sqrt; j++) {
			int nt = std::rand() % 4;
			std::array<float, 2> new_xy = affineTransform(ax[nt], bx[nt], ay[nt], by[nt], cx[nt], cy[nt], x.at(j), y.at(j));
			x.at(j)=new_xy[0];
			y.at(j)=new_xy[1];
		}
	}
}

float zoom = 100.0f;

void draw() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer); 

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); 
	for (int i = 0; i < x.size(); i++) {
		SDL_RenderDrawPoint(renderer, x.at(i) * zoom + WIDTH / 2, y.at(i) * zoom + HEIGHT / 2);
	}	

	SDL_RenderPresent(renderer); 
}

void loop() {
	while (running) {
		handleEvents();
		draw();
	}
}

int main(int argc, char* argv[]) {
	if (!init()) {
		return 1;
	}
	setupSet();
	generateFractal();
	loop();
	close();
	return 0;
}
