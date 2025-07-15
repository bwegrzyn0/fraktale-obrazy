#include <SDL2/SDL.h>

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

void draw() {
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	SDL_RenderClear(renderer); 

	SDL_Rect rect;
	rect.x = 100;
	rect.y = 100;
	rect.w = 100;
	rect.h = 100;

	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); 
	SDL_RenderFillRect(renderer, &rect); 

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
	loop();
	close();
	return 0;
}
