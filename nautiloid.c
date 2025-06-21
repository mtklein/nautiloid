#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = NULL;
    SDL_Renderer *ren = NULL;
    if (SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN,
                                   &win, &ren) != 0) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer Error: %s\n",
                SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowTitle(win, "Nautiloid Adventure");

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        // TODO: Render game graphics and text here
        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

