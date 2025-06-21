#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SDL_Texture *
render_text(SDL_Renderer *renderer, TTF_Font *font, char const *text,
            SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

static void
draw_gradient_rect(SDL_Renderer *renderer, SDL_Rect rect,
                   SDL_Color top, SDL_Color bottom) {
    for (int y = 0; y < rect.h; ++y) {
        float ratio = (float)y / (float)rect.h;
        Uint8 r = (Uint8)(top.r + (bottom.r - top.r) * ratio);
        Uint8 g = (Uint8)(top.g + (bottom.g - top.g) * ratio);
        Uint8 b = (Uint8)(top.b + (bottom.b - top.b) * ratio);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, rect.x, rect.y + y,
                           rect.x + rect.w, rect.y + y);
    }
}

static void
draw_text_box(SDL_Renderer *renderer, TTF_Font *font,
              char const *lines[], int line_count) {
    int   width  = 0;
    int   height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    int        box_height = height / 3;
    SDL_Rect   rect       = {20, height - box_height - 20,
                             width - 40, box_height};
    SDL_Color const top    = {100, 100, 255, 255},
                       bottom = {40, 40, 180, 255};
    draw_gradient_rect(renderer, rect, top, bottom);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawRect(renderer, &rect);

    int y = rect.y + 10;
    for (int i = 0; i < line_count; ++i) {
        SDL_Texture *text =
            render_text(renderer, font, lines[i], (SDL_Color){255, 255, 255, 255});
        if (text) {
            SDL_Rect dst = {rect.x + 10, y, 0, 0};
            SDL_QueryTexture(text, NULL, NULL, &dst.w, &dst.h);
            SDL_RenderCopy(renderer, text, NULL, &dst);
            SDL_DestroyTexture(text);
        }
        y += 26;
    }
}

static void
show_message(SDL_Renderer *renderer, TTF_Font *font,
             char const *lines[], int line_count) {
    bool waiting = true;
    SDL_Event e;
    while (waiting) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE) {
                waiting = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        draw_text_box(renderer, font, lines, line_count);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

static int
menu_prompt(SDL_Renderer *renderer, TTF_Font *font, char const *question,
            char const *options[], int option_count) {
    int choice = -1;
    char const *lines[10];
    SDL_Event    e;
    while (choice == -1) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
            if (e.type == SDL_KEYDOWN) {
                if (SDLK_1 <= e.key.keysym.sym && e.key.keysym.sym <= SDLK_9) {
                    int idx = (int)(e.key.keysym.sym - SDLK_1);
                    if (idx < option_count) {
                        choice = idx;
                    }
                }
            }
        }
        lines[0] = question;
        for (int i = 0; i < option_count; ++i) {
            static char buffers[9][64];
            snprintf(buffers[i], sizeof(buffers[i]), "%d. %s", i + 1, options[i]);
            lines[i + 1] = buffers[i];
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        draw_text_box(renderer, font, lines, option_count + 1);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return choice;
}

static void
text_input(SDL_Renderer *renderer, TTF_Font *font, char const *prompt,
           char *buffer, int capacity) {
    buffer[0] = '\0';
    bool done = false;
    SDL_StartTextInput();
    SDL_Event e;
    while (!done) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                SDL_StopTextInput();
                exit(0);
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_RETURN) {
                    done = true;
                } else if (e.key.keysym.sym == SDLK_BACKSPACE) {
                    size_t len = strlen(buffer);
                    if (len > 0) {
                        buffer[len - 1] = '\0';
                    }
                }
            } else if (e.type == SDL_TEXTINPUT) {
                if (strlen(buffer) + strlen(e.text.text) < (size_t)capacity - 1) {
                    strcat(buffer, e.text.text);
                }
            }
        }
        char const *lines[2] = {prompt, buffer};
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        draw_text_box(renderer, font, lines, 2);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_StopTextInput();
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window   *window   = NULL;
    SDL_Renderer *renderer = NULL;
    if (0 != SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN,
                                         &window, &renderer)) {
        fprintf(stderr, "SDL_CreateWindowAndRenderer() -> %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_SetWindowTitle(window, "Nautiloid Adventure");

    TTF_Font *font = TTF_OpenFont("Final Fantasy VI SNESb.ttf", 28);
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    char name[64];
    text_input(renderer, font, "Enter your name:", name, (int)sizeof(name));
    char const *classes[] = {"Fighter", "Rogue", "Mage"};
    int class_idx =
        menu_prompt(renderer, font, "Choose a class", classes,
                    (int)(sizeof(classes) / sizeof(classes[0])));
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "Welcome %s the %s!", name,
             classes[class_idx]);
    char const *lines[] = {buffer, "Press SPACE to quit"};
    show_message(renderer, font, lines, 2);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow  (window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

