#include <SDL2/SDL.h>
#include <vector>
#include <array>
#include <chrono>
#include <stdio.h>
#include <thread>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"

const int WIDTH=1280;
const int HEIGHT=720;

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
float zoomFactor = 1.1f;

// połozenie, prędkość kamery
float cam_x = 5;
float cam_y = 5;
float cam_vx = 0;
float cam_vy = 0;
float cam_v = 0.1f;

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

class GenerateFractal {
	public:
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

		int N; // liczba iteracji

		// zbiór punktów
		std::vector<float> x, y;
		int N_points_sqrt; // pierwiastek kwadratowy liczby punktów 
					
		// rozmiar wyświetlanej powierzchni
		float areaX;
		float areaY;
		float areaWidth;
		float areaHeight;
		float resolution; // pixeli na jednostkę areaWidth/areaHeight
		std::vector<std::vector<float>> density;
		float densityStep; // krok przy dodawaniu gęstości
		float maxDensity; 
		bool generated;
		bool killThread;

		GenerateFractal() {
			// domyślne parametry
			N = 50; 
			N_points_sqrt = 500;
			areaX = -5;
			areaY = -5;
			areaWidth = 15;
			areaHeight = 10;
			resolution = 100.0f;
			densityStep = 0.3f; 
			maxDensity = 0; 
			generated = false;
			killThread = false;
		}

		// x'=ax*x+bx*y+cx
		// y'=ay*x+by*y+cy
		std::array<float, 2>  affineTransform(float ax, float bx, float ay, float by, float cx, float cy, float x, float y) {
			float new_x = ax * x + bx * y + cx;
			float new_y = ay * x + by * y + cy;
			return {new_x, new_y};
		}


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
			maxDensity = 0;
			std::vector<float> zeroes((int) (areaHeight*resolution) + 1);
			for (int i = 0; i < (int) (areaWidth*resolution) + 1; i++) {
				density.push_back(zeroes);
			}
			generated = true;
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
					std::array<float, 2> new_xy = affineTransform(ax[nt], bx[nt], ay[nt], by[nt], cx[nt], cy[nt], x[j], y[j]);
					x[j]=new_xy[0];
					y[j]=new_xy[1];
					if (x[j] >= areaX && x[j] <= areaX + areaWidth) 
						if (y[j] >= areaY && y[j] <= areaY + areaHeight) {
							if (killThread)
								return;
							int xindex = (int) ((x[j]-areaX) * resolution);
							int yindex = (int) ((y[j]-areaY) * resolution);
							density[xindex][yindex] += densityStep;
							if (density[xindex][yindex] > maxDensity)
								maxDensity = density[xindex][yindex];
						}
				}
			}
		}
};

std::thread t;
int framesSinceCalled = 0; // służy do wprowadzenia opóźnienia w kilkaniu guzikiem, bo inaczej thread nie nadąża skończyć pracy i jest błąd
float adjustResolution = 10.0f;
bool threadCreated = false;
void newFractal(GenerateFractal* currentFractal) {
	framesSinceCalled = 0;
	(*currentFractal).killThread = true;	
	(*currentFractal) = GenerateFractal();
	(*currentFractal).resolution = adjustResolution;
	(*currentFractal).setupSet();
	t = std::thread(&GenerateFractal::generateFractal, currentFractal);
	t.detach();
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

float minDensity = 0.025f; // najmniejsza wyświetlana gęstość jako część maksimum
// tablica odcieni szarości, aby nie liczyć ich w kółko
auto gray = new Uint32[256];

// generuj odcienie szarości
void generateGray() {
	for (int i = 0; i < 256; i++) {
		gray[i] =  (((255 << 8) + i << 8) + i << 8) + i;
	}
}

void draw(GenerateFractal* gF) {
	SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(Uint32));
	memset(pixels, 0, WIDTH*HEIGHT*sizeof(Uint32));

	int spacing = std::ceil(zoom / (*gF).resolution);
	float floatSpacing = zoom / (*gF).resolution;
	float currentZoom = zoom; // jeśli w samym środku pętli zmieni się zoom to program się zcrashuje
	float minDensityMax = minDensity * (*gF).maxDensity;
	if ((*gF).generated)
		for (int i = 0; i < (int) ((*gF).areaWidth*(*gF).resolution); i++) 
			for (int j = 0; j < (int) ((*gF).areaHeight*(*gF).resolution); j++) {
				if ((*gF).density[i][j] < minDensityMax)
					continue;
				int current_x = (int) ((float) i * floatSpacing - cam_x * currentZoom + cam_x + WIDTH / 2);
				int current_y = (int) ((float) j * floatSpacing - cam_y * currentZoom + cam_y + HEIGHT / 2);

				if(current_x < WIDTH + spacing && current_x > -spacing) 
					if (current_y < HEIGHT + spacing && current_y > -spacing)
						for (int x = 0; x < spacing; x++)
							for (int y = 0; y < spacing; y++)
								if (current_x +x < WIDTH && current_x+x > 0)
									if (current_y+y < HEIGHT && current_y+y > 0) {
										int pixelVal = (int) ((*gF).density[i][j] / (*gF).maxDensity * 255);
										pixels[(current_y+y) * WIDTH + current_x + x] = gray[pixelVal];
									}
			}	

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);

	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Adjust parameters");
	ImGui::SliderFloat("Minimal value shown", &minDensity, 0.0f, 1.0f);
	ImGui::SliderFloat("Resolution", &adjustResolution, 0.0f, 200.0f);
	if (ImGui::Button("Generate!")) {
		if (framesSinceCalled >= 10) {
			newFractal(gF);
		}
	}
	ImGui::End();
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

	framesSinceCalled++;
	 
        SDL_RenderPresent(renderer);

}

int FPS = 60; // docelowa liczba klatek na sekundę

void loop(GenerateFractal* gF) {
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
			draw(gF);
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

	GenerateFractal gF;
	newFractal(&gF);
	
	std::thread t(eventLoop);
	t.detach();

	generateGray();

	loop(&gF);

	close();
	return 0;
}
