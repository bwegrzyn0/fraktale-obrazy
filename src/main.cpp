#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <chrono>
#include <stdio.h>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

const int WIDTH=1920;
const int HEIGHT=1080;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture * texture;
Uint32 * pixels = new Uint32[WIDTH * HEIGHT]; // piksele na ekranie

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
	if (success) {
		texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
		ImGui_ImplSDLRenderer2_Init(renderer);
	}
	return success;
}

void close() {
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
	delete[] pixels;
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	SDL_Quit();
}

bool running = true;
SDL_Event event;
// góra, dół, prawo, lewo
bool keysDown[] = {false, false, false, false};

// zoom kamery i krok 
float zoom = 200.0f;
float zoomFactor = 1.2f;

// połozenie, prędkość kamery
float cam_x = 5;
float cam_y = 5;
float cam_vx = 0;
float cam_vy = 0;
float cam_v = 0.2f;

void handleEvents() {
	while(SDL_PollEvent(&event)) { 
		ImGui_ImplSDL2_ProcessEvent(&event);
		switch(event.type) { 
			case SDL_MOUSEWHEEL:
				if (event.wheel.y > 0)  {
					zoom *= zoomFactor;
					cam_v /= zoomFactor;
				} else if (event.wheel.y < 0) {
					zoom /= zoomFactor;
					cam_v *= zoomFactor;
				}
				break;
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
float probabilities[4] = {0.01f, 0.152f, 0.241f, 0.6f};

int N = 1000; // liczba iteracji

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

// rozmiar wyświetlanej powierzchni
const float areaX = -5;
const float areaY = -5;
const float areaWidth = 15;
const float areaHeight = 10;
const float resolution = 100.0f; // pixeli na jednostkę areaWidth/areaHeight
auto density = new float[(int) (areaWidth*resolution) + 1][(int) (areaHeight*resolution) + 1];
float densityStep = 0.3f; // krok przy dodawaniu gęstości
float maxDensity = 0;

void generateFractal() {
	maxDensity = 0;
	for (int i = 0; i < (int) (areaWidth*resolution); i++) 
		for (int j = 0; j < (int) (areaHeight*resolution); j++)
			density[i][j] = 0;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N_points_sqrt*N_points_sqrt; j++) {
			int nt = std::rand() % 101;
			if (nt >= 0 && nt < (int) (probabilities[0] * 100.0f))
				nt = 0;
			else if (nt >= (int) (probabilities[0] * 100.0f) && nt < (int) ((probabilities[0] + probabilities[1]) * 100.0f))
				nt = 1;
			else if (nt >= (int) ((probabilities[0]+ probabilities[1]) * 100.0f) && nt < (int) ((probabilities[0] + probabilities[1] + probabilities[2]) * 100.0f))
				nt = 2;
			else 
				nt = 3;
			std::array<float, 2> new_xy = affineTransform(ax[nt], bx[nt], ay[nt], by[nt], cx[nt], cy[nt], x.at(j), y.at(j));
			x.at(j)=new_xy[0];
			y.at(j)=new_xy[1];
			if (x.at(j) >= areaX && x.at(j) <= areaX + areaWidth) 
				if (y.at(j) >= areaY && y.at(j) <= areaY + areaHeight) {
					density[(int) ((x.at(j)-areaX) * resolution)][(int) ((y.at(j)-areaY) * resolution )] += densityStep;
					if (density[(int) ((x.at(j)-areaX) * resolution )][(int) ((y.at(j)-areaY) * resolution )] > maxDensity)
						maxDensity = density[(int) ((x.at(j)-areaX) * resolution )][(int) ((y.at(j)-areaY) * resolution )];
				}
		}
	}
}


void updateCamera(float delta) {
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

	// dzięki pomnożeniu przez delta kamera porusza się z taką samą prędkością niezależnie od FPS
	cam_x += cam_vx * delta;
	cam_y += cam_vy * delta;
}

float minDensity = 5.0f; // najmniejsza wyświetlana gęstość
// tablica odcieni szarości, aby nie liczyć ich w kółko
auto gray = new Uint32[256];

// generuj odcienie szarości
void generateGray() {
	for (int i = 0; i < 256; i++) {
		gray[i] =  (((255 << 8) + i << 8) + i << 8) + i;
	}
}

void draw() {
	SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
	memset(pixels, 0, WIDTH*HEIGHT*sizeof(Uint32));

	int spacing = std::ceil(zoom / resolution);
	for (int i = 0; i < (int) (areaWidth*resolution); i++) 
		for (int j = 0; j < (int) (areaHeight*resolution); j++) {
			if (density[i][j] < minDensity)
				continue;
			int current_x = (int) (((float) i / resolution - cam_x) * zoom + cam_x + WIDTH / 2);
			int current_y = (int) (((float) j / resolution - cam_y) * zoom + cam_y + HEIGHT / 2);
			if (current_x < WIDTH - spacing && current_x > 0)
				if (current_y < HEIGHT - spacing && current_y > 0) {
					for (int x = 0; x < spacing; x++)
						for (int y = 0; y < spacing; y++)
							pixels[(current_y+y) * WIDTH + current_x + x] = gray[(int) (density[i][j] / maxDensity * 255)];
					//	(((255 << 8) + (int) density[i][j] << 8) + (int) density[i][j] << 8) + (int) density[i][j];
				}
		}	

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Hello, Dear ImGui with SDL2");
	ImGui::Text("This is just a basic Hello World!");
	ImGui::End();
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
	 
        SDL_RenderPresent(renderer);
}

int FPS = 60; // docelowa liczba klatek na sekundę

void loop() {
	double deltaTime = 1000000000.0d / (float) FPS;
	double delta = 0;
	int frames= 0;
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
			updateCamera((float)delta);
			draw();
			delta = 0;
			frames++;
		}

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

// oddzielna pętla dla sprawdzania klawiszy itd. żeby nie lagowało
void eventLoop() {
	double deltaTime = 1000000000.0d / (float) FPS;
	double delta = 0;
	auto lastTime = std::chrono::system_clock::now();
	while (running) {
		auto now = std::chrono::system_clock::now();
		auto duration = now - lastTime;
		auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);	
		float timeElapsed = ns.count();
		delta += timeElapsed / deltaTime;
		lastTime = now;

		if (delta >= 1) {
			handleEvents();
			delta = 0;
		}
	}
}

int main(int argc, char* argv[]) {
	if (!init()) {
		return 1;
	}

	setupSet();

	// uruchom równolegle generateFractal
	std::thread t1(generateFractal);
	t1.detach();
	
	std::thread t2(eventLoop);
	t2.detach();

	generateGray();

	loop();

	close();
	return 0;
}
