
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    SDL_Rect rect;
    const char *dest;
} Door;

typedef struct {
    SDL_Rect rect;
    const char *item;
    bool opened;
    char padding[7];
} Chest;

typedef struct {
    const char *name;
    SDL_Color color;
    Point pos;
    bool joined;
    char padding[3];
} NPC;

typedef struct {
    const char *name;
    Door doors[4];
    int door_count;
    char padding_a[4];
    Chest chests[2];
    int chest_count;
    char padding_b[4];
    NPC npcs[4];
    int npc_count;
    char padding_c[4];
    const char *shape;
} Room;

static void draw_humanoid(SDL_Renderer *ren, Point pos, SDL_Color color) {
    SDL_SetRenderDrawColor(ren, color.r, color.g, color.b, 255);
    SDL_Rect head = {pos.x - 5, pos.y - 48, 10, 10};
    SDL_RenderFillRect(ren, &head);
    SDL_Rect torso = {pos.x - 4, pos.y - 38, 8, 20};
    SDL_RenderFillRect(ren, &torso);
    SDL_Rect left_leg = {pos.x - 4, pos.y - 18, 3, 18};
    SDL_RenderFillRect(ren, &left_leg);
    SDL_Rect right_leg = {pos.x + 1, pos.y - 18, 3, 18};
    SDL_RenderFillRect(ren, &right_leg);
}

static void draw_room_bounds(SDL_Renderer *ren, const char *shape) {
    SDL_SetRenderDrawColor(ren, 0, 0, 100, 255);
    if (!shape) {
        SDL_Rect r = {120, 100, 400, 280};
        SDL_RenderDrawRect(ren, &r);
        return;
    }
    if (strcmp(shape, "circle") == 0) {
        /* crude ellipse using many rectangles */
        for (int i = 0; i < 440; i += 8) {
            int h = (int)(160.0 * (1.0 - ((i - 220) * (i - 220)) / (220.0 * 220.0)));
            SDL_Rect r = {100 + i, 240 - h / 2, 8, h};
            SDL_RenderDrawRect(ren, &r);
        }
    } else if (strcmp(shape, "wide") == 0) {
        SDL_Rect r = {40, 200, 560, 80};
        SDL_RenderDrawRect(ren, &r);
    } else if (strcmp(shape, "tall") == 0) {
        SDL_Rect r = {260, 40, 120, 400};
        SDL_RenderDrawRect(ren, &r);
    } else {
        SDL_Rect r = {120, 100, 400, 280};
        SDL_RenderDrawRect(ren, &r);
    }
}

static bool rect_overlap(SDL_Rect a, SDL_Rect b) {
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

static void setup_rooms(Room *pod, Room *corr) {
    *pod = (Room){
        .name = "Pod Room",
        .shape = "circle",
        .door_count = 1,
        .chest_count = 0,
        .npc_count = 1,
    };
    pod->doors[0] = (Door){{600, 220, 40, 40}, "Corridor"};
    pod->npcs[0] = (NPC){"Familiar", {0, 255, 255, 255}, {200, 240}, false, {0}};

    *corr = (Room){
        .name = "Corridor",
        .shape = "wide",
        .door_count = 1,
        .chest_count = 0,
        .npc_count = 0,
    };
    corr->doors[0] = (Door){{40, 220, 40, 40}, "Pod Room"};
}


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

    Room pod      = {0},
         corridor = {0};
    setup_rooms(&pod, &corridor);
    Room *current = &pod;
    Point player = {320, 240};
    Uint32 last = SDL_GetTicks();

    bool running = true;
    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = (float)(now - last) / 16.0f;
        last = now;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_E) {
                SDL_Rect pr = {player.x - 8, player.y - 48, 16, 48};
                for (int i = 0; i < current->door_count; i++) {
                    if (rect_overlap(pr, current->doors[i].rect)) {
                        if (strcmp(current->doors[i].dest, "Pod Room") == 0) {
                            current = &pod;
                        } else if (strcmp(current->doors[i].dest, "Corridor") == 0) {
                            current = &corridor;
                        }
                        player.x = 320;
                        player.y = 240;
                        printf("You go to %s\n", current->name);
                        break;
                    }
                }
                for (int i = 0; i < current->chest_count; i++) {
                    Chest *c = &current->chests[i];
                    if (!c->opened && rect_overlap(pr, c->rect)) {
                        c->opened = true;
                        printf("You found %s!\n", c->item);
                        break;
                    }
                }
                for (int i = 0; i < current->npc_count; i++) {
                    NPC *n = &current->npcs[i];
                    SDL_Rect nr = {n->pos.x - 8, n->pos.y - 48, 16, 48};
                    if (!n->joined && rect_overlap(pr, nr)) {
                        n->joined = true;
                        printf("%s joins you!\n", n->name);
                        break;
                    }
                }
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_LEFT])  player.x -= (int)(4 * dt);
        if (keys[SDL_SCANCODE_RIGHT]) player.x += (int)(4 * dt);
        if (keys[SDL_SCANCODE_UP])    player.y -= (int)(4 * dt);
        if (keys[SDL_SCANCODE_DOWN])  player.y += (int)(4 * dt);

        if (player.x < 20)  player.x = 20;
        if (player.x > 620) player.x = 620;
        if (player.y < 20)  player.y = 20;
        if (player.y > 460) player.y = 460;

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        draw_room_bounds(ren, current->shape);
        SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
        for (int i = 0; i < current->door_count; i++) {
            SDL_RenderDrawRect(ren, &current->doors[i].rect);
        }
        for (int i = 0; i < current->chest_count; i++) {
            Chest *c = &current->chests[i];
            SDL_SetRenderDrawColor(ren, 160, 82, 45, 255);
            SDL_RenderDrawRect(ren, &c->rect);
            if (c->opened) {
                SDL_RenderDrawLine(ren, c->rect.x, c->rect.y, c->rect.x + c->rect.w, c->rect.y + c->rect.h);
                SDL_RenderDrawLine(ren, c->rect.x + c->rect.w, c->rect.y, c->rect.x, c->rect.y + c->rect.h);
            }
        }
        for (int i = 0; i < current->npc_count; i++) {
            if (!current->npcs[i].joined) {
                draw_humanoid(ren, current->npcs[i].pos, current->npcs[i].color);
            }
        }
        draw_humanoid(ren, player, (SDL_Color){255,255,255,255});

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}


