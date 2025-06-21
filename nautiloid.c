#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    if (0 != SDL_CreateWindowAndRenderer(640,480, SDL_WINDOW_SHOWN, &window,&renderer)) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer() -> %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowTitle(window, "Nautiloid Adventure");

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        // TODO: Render game graphics and text here
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow  (window);
    SDL_Quit();
    return 0;
}

