#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <chrono>
#include <stdio.h>

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
// góra, dół, prawo, lewo
bool keysDown[] = {false, false, false, false};

void handleEvents() {
	while(SDL_PollEvent(&event)) { 
		switch(event.type) { 
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_UP:
						keysDown[0] = true;
						break;
					case SDLK_DOWN:
						keysDown[1] = true;
						break;
					case SDLK_RIGHT:
						keysDown[2] = true;
						break;
					case SDLK_LEFT:
						keysDown[3] = true;
						break;
					default:
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					case SDLK_UP:
						keysDown[0] = false;
						break;
					case SDLK_DOWN:
						keysDown[1] = false;
						break;
					case SDLK_RIGHT:
						keysDown[2] = false;
						break;
					case SDLK_LEFT:
						keysDown[3] = false;
						break;
					default:
						break;
				}
				break;
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

int N = 100; // liczba iteracji

// zbiór punktów
std::vector<float> x, y;
int N_points_sqrt = 500; // pierwiastek kwadratowy liczby punktów 

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

float zoom = 1000.0f;
float cam_x = 0;
float cam_y = 0;
float cam_v = 5.0f;
float cam_vx = 0;
float cam_vy = 0;

void updateCamera() {
	if (keysDown[0]) 
		cam_vy = -cam_v;
	else if (keysDown[1]) 
		cam_vy = cam_v;
	else
		cam_vy = 0;
	if (keysDown[2]) 
		cam_vx = cam_v;
	else if (keysDown[3]) 
		cam_vx = -cam_v;
	else
		cam_vx = 0;

	cam_x += cam_vx;
	cam_y += cam_vy;
}

void draw() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer); 

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); 
	for (int i = 0; i < x.size(); i++) {
		float current_x = x.at(i) * zoom + WIDTH / 2 - cam_x;
		float current_y = y.at(i) * zoom + HEIGHT / 2 - cam_y;
		if (current_x <= WIDTH && current_x >= 0)
			if (current_y <= HEIGHT && current_y >= 0)
				SDL_RenderDrawPoint(renderer, current_x, current_y);
	}	

	SDL_RenderPresent(renderer); 
}

int FPS = 60; // docelowa liczba klatek na sekundę
double deltaTime = 1000000000.0d / (float) FPS;
double delta = 0;
int frames= 0;

void loop() {
	auto lastTime = std::chrono::system_clock::now();
	auto lastTimeFrames = std::chrono::system_clock::now();
	while (running) {
		auto now = std::chrono::system_clock::now();
		auto duration = now - lastTime;
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);	
		float timeElapsed = ns.count();
		delta += timeElapsed / deltaTime;
		lastTime = now;

		if (delta >= 1) {
			updateCamera();
			draw();
			delta--;
			frames++;
		}

		handleEvents();

		// co sekundę wypisz szybkość programu w klatkach na sekundę
		auto nowFrames = std::chrono::system_clock::now();
		auto durationFrames = nowFrames - lastTimeFrames;
		auto nsFrames = std::chrono::duration_cast<std::chrono::nanoseconds>(durationFrames);	
		float timeElapsedFrames = nsFrames.count();
		if (timeElapsedFrames >= 1000000000) {
			printf("FPS: %d\n", frames);
			frames = 0;
			lastTimeFrames = nowFrames;
		}

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
