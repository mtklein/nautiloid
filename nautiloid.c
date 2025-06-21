#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * TODO: port features from pygame_adventure.py
 * - Define class data for Healer, Beast and Demon
 * - Create NPC, Chest, Prop, Door and Room structs
 * - Implement create_rooms() to mirror Python rooms
 * - Expand Player with inventory, flags and companions
 * - Update companions with an update_companions() function
 * - Add face drawing and speaker options to the UI helpers
 * - Show floating damage numbers and health bars
 * - Implement combat_encounter() for turn-based fights
 * - Support moving between rooms and interacting with objects
 * - Display a game_end summary after escaping
 * - Use SDL_image to load textured sprites
 * - Add pathfinding for NPC movement
 */

typedef struct {
    char const *name;
    char const *target;
    bool        melee;
    char        _pad[3];
    int         power;
} Ability;

typedef struct {
    int strength;
    int agility;
    int wisdom;
    int hp;
} Attributes;

typedef enum {
    ATTR_STRENGTH,
    ATTR_AGILITY,
    ATTR_WISDOM
} AttrKind;

typedef enum {
    CLASS_FIGHTER,
    CLASS_ROGUE,
    CLASS_MAGE,
    CLASS_HEALER,
    CLASS_BEAST,
    CLASS_DEMON
} ClassId;
#define CLASS_COUNT (CLASS_DEMON + 1)

static inline int __attribute__((unused))
attribute_value(Attributes const *attributes, AttrKind kind) {
    if (kind == ATTR_STRENGTH) {
        return attributes->strength;
    }
    if (kind == ATTR_AGILITY) {
        return attributes->agility;
    }
    if (kind == ATTR_WISDOM) {
        return attributes->wisdom;
    }
    return 0;
}

typedef struct {
    char const *name;
    Ability const *abilities;
    int ability_count;
    Attributes attributes;
    ClassId     id;
} ClassInfo;

typedef struct {
    int x;
    int y;
    char name[64];
    ClassInfo const *info;
    char inventory[8][32];
    int  items;
    char _pad[4];
} Player;

typedef struct {
    SDL_Rect rect;
    bool     opened;
    char     _pad[3];
} Chest;

typedef struct {
    SDL_Rect rect;
} Prop;

typedef struct {
    SDL_Rect rect;
    bool     open;
    char     _pad[3];
} Door;

typedef void (*NpcDraw)(SDL_Renderer *, int, int);
typedef void (*NpcDialog)(SDL_Renderer *, TTF_Font *);

typedef struct {
    int       x;
    int       y;
    NpcDraw   draw;
    NpcDialog dialog;
} Npc;

typedef struct {
    Chest *chest;
    int    chests;
    Prop  *prop;
    int    props;
    Door  *door;
    int    doors;
    Npc   *npc;
    int    npcs;
} Room;

static Ability const fighter_abilities[] = {
    {.name = "Strike", .target = "enemy", .melee = true, .power = 3},
    {.name = "Power Attack", .target = "enemy", .melee = true, .power = 5},
};

static Ability const rogue_abilities[] = {
    {.name = "Stab", .target = "enemy", .melee = true, .power = 3},
    {.name = "Sneak Attack", .target = "enemy", .melee = true, .power = 4},
};

static Ability const mage_abilities[] = {
    {.name = "Firebolt", .target = "enemy", .melee = false, .power = 4},
    {.name = "Barrier", .target = "ally", .melee = false, .power = 3},
};

static Ability const healer_abilities[] = {
    {.name = "Smite", .target = "enemy", .melee = true, .power = 3},
    {.name = "Heal", .target = "ally", .melee = false, .power = 4},
};

static Ability const beast_abilities[] = {
    {.name = "Bite", .target = "enemy", .melee = true, .power = 2},
    {.name = "Encourage", .target = "ally", .melee = false, .power = 2},
};

static Ability const demon_abilities[] = {
    {.name = "Claw", .target = "enemy", .melee = true, .power = 2},
};

static ClassInfo const classes[] = {
    {.name = "Fighter", .abilities = fighter_abilities, .ability_count = 2,
     .attributes = {8, 4, 3, 12}, .id = CLASS_FIGHTER},
    {.name = "Rogue", .abilities = rogue_abilities, .ability_count = 2,
     .attributes = {5, 8, 3, 10}, .id = CLASS_ROGUE},
    {.name = "Mage", .abilities = mage_abilities, .ability_count = 2,
     .attributes = {3, 5, 8, 8}, .id = CLASS_MAGE},
    {.name = "Healer", .abilities = healer_abilities, .ability_count = 2,
     .attributes = {4, 4, 8, 10}, .id = CLASS_HEALER},
    {.name = "Beast", .abilities = beast_abilities, .ability_count = 2,
     .attributes = {6, 6, 2, 8}, .id = CLASS_BEAST},
    {.name = "Demon", .abilities = demon_abilities, .ability_count = 1,
     .attributes = {5, 5, 5, 10}, .id = CLASS_DEMON},
};

static AttrKind const attack_attr[CLASS_COUNT] = {
    ATTR_STRENGTH, /* Fighter */
    ATTR_AGILITY,  /* Rogue   */
    ATTR_WISDOM,   /* Mage    */
    ATTR_WISDOM,   /* Healer  */
    ATTR_STRENGTH, /* Beast   */
    ATTR_STRENGTH  /* Demon   */
};

static AttrKind const defense_attr[CLASS_COUNT] = {
    ATTR_STRENGTH, /* Fighter */
    ATTR_AGILITY,  /* Rogue   */
    ATTR_WISDOM,   /* Mage    */
    ATTR_WISDOM,   /* Healer  */
    ATTR_AGILITY,  /* Beast   */
    ATTR_STRENGTH  /* Demon   */
};

static inline AttrKind __attribute__((unused))
attack_attribute(ClassInfo const *cls) {
    return attack_attr[cls->id];
}

static inline AttrKind __attribute__((unused))
defense_attribute(ClassInfo const *cls) {
    return defense_attr[cls->id];
}


static SDL_Texture *
render_text(SDL_Renderer *renderer, TTF_Font *font, char const *text,
            SDL_Color color) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
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
                    choice = idx;
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
draw_humanoid(SDL_Renderer *renderer, int x, int y, SDL_Color color) {
    SDL_Rect head  = {x - 5, y - 48, 10, 10};
    SDL_Rect torso = {x - 4, y - 38, 8, 20};
    SDL_Rect left_arm  = {x - 8, y - 38, 3, 15};
    SDL_Rect right_arm = {x + 5, y - 38, 3, 15};
    SDL_Rect left_leg  = {x - 4, y - 18, 3, 18};
    SDL_Rect right_leg = {x + 1, y - 18, 3, 18};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &head);
    SDL_RenderFillRect(renderer, &torso);
    SDL_RenderFillRect(renderer, &left_arm);
    SDL_RenderFillRect(renderer, &right_arm);
    SDL_RenderFillRect(renderer, &left_leg);
    SDL_RenderFillRect(renderer, &right_leg);
}

static void
draw_warrior(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){255, 0, 0, 255});
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 10, y - 36);
}

static void
draw_rogue(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){34, 139, 34, 255});
    SDL_Rect hood = {x - 6, y - 48, 12, 8};
    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 255);
    SDL_RenderFillRect(renderer, &hood);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 10, y - 30);
}

static void
draw_mage(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){0, 0, 128, 255});
    SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 6, y - 40);
    SDL_RenderDrawLine(renderer, x + 6, y - 40, x + 6, y - 42);
    SDL_RenderDrawPoint(renderer, x + 6, y - 42);
}

static void
draw_cleric(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){255, 255, 0, 255});
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, x, y - 28, x, y - 44);
    SDL_RenderDrawLine(renderer, x - 4, y - 36, x + 4, y - 36);
}

static void
draw_familiar(SDL_Renderer *renderer, int x, int y) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    int const r = 8;
    for (int dy = -r; dy <= r; ++dy) {
        double width = sqrt((double)(r * r - dy * dy));
        int    dx    = (int)width;
        SDL_RenderDrawLine(renderer, x - dx, y - 8 + dy, x + dx, y - 8 + dy);
    }
}

static void
draw_imp(SDL_Renderer *renderer, int x, int y) {
    SDL_Rect body = {x - 6, y - 16, 12, 16};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &body);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawLine(renderer, x - 4, y - 16, x - 2, y - 20);
    SDL_RenderDrawLine(renderer, x + 4, y - 16, x + 2, y - 20);
}

static void
draw_chest(SDL_Renderer *renderer, SDL_Rect rect, bool opened) {
    SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
    SDL_RenderDrawRect(renderer, &rect);
    if (opened) {
        SDL_RenderDrawLine(renderer, rect.x, rect.y,
                           rect.x + rect.w, rect.y + rect.h);
        SDL_RenderDrawLine(renderer, rect.x + rect.w, rect.y,
                           rect.x, rect.y + rect.h);
    }
}

static void
draw_door(SDL_Renderer *renderer, SDL_Rect rect) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderDrawRect(renderer, &rect);
    SDL_RenderDrawLine(renderer, rect.x + rect.w / 2, rect.y,
                       rect.x + rect.w / 2, rect.y + rect.h);
}

static void
draw_prop(SDL_Renderer *renderer, SDL_Rect rect) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 0, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

static void
familiar_dialog(SDL_Renderer *renderer, TTF_Font *font) {
    char const *opts[] = {"\"Who are you?\"",
                           "\"Will you help me escape?\"",
                           "\"Let's go.\""};
    while (1) {
        int idx = menu_prompt(renderer, font, "The familiar chirps softly.",
                              opts, 3);
        if (idx == 0) {
            char const *msg[] =
                {"It chitters about being bound to the ship by foul magic."};
            show_message(renderer, font, msg, 1);
        } else if (idx == 1) {
            char const *msg[] = {"The creature nods enthusiastically."};
            show_message(renderer, font, msg, 1);
        } else {
            char const *msg[] = {"It hops onto your shoulder."};
            show_message(renderer, font, msg, 1);
            break;
        }
    }
}

static void
cleric_dialog(SDL_Renderer *renderer, TTF_Font *font) {
    char const *opts[] = {"\"What happened here?\"",
                           "\"Can you heal us?\"",
                           "\"Let's leave this ship.\""};
    while (1) {
        int idx = menu_prompt(renderer, font, "The cleric steadies her breath.",
                              opts, 3);
        if (idx == 0) {
            char const *msg[] =
                {"'A ritual went terribly wrong,' she explains."};
            show_message(renderer, font, msg, 1);
        } else if (idx == 1) {
            char const *msg[] =
                {"She murmurs a prayer and a warm light surrounds you."};
            show_message(renderer, font, msg, 1);
        } else {
            char const *msg[] =
                {"She grabs a nearby pack and prepares to follow."};
            show_message(renderer, font, msg, 1);
            break;
        }
    }
}

static void
imp_dialog(SDL_Renderer *renderer, TTF_Font *font) {
    char const *msg[] = {"The imp hisses at you."};
    show_message(renderer, font, msg, 1);
}

static void
text_input(SDL_Renderer *renderer, TTF_Font *font, char const *prompt,
           char *buffer, int capacity) {
    buffer[0] = '\0';
    (void)capacity;
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
                    buffer[len - 1] = '\0';
                }
            } else if (e.type == SDL_TEXTINPUT) {
                strcat(buffer, e.text.text);
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

static void
inventory_add_item(Player *player, char const *item) {
    strncpy(player->inventory[player->items], item,
            sizeof(player->inventory[0]) - 1);
    player->inventory[player->items][sizeof(player->inventory[0]) - 1] = '\0';
    player->items++;
}

static void
show_inventory(SDL_Renderer *renderer, TTF_Font *font, Player const *player) {
    char const *lines[9];
    if (player->items == 0) {
        lines[0] = "Your inventory is empty.";
        show_message(renderer, font, lines, 1);
        return;
    }
    for (int i = 0; i < player->items; ++i) {
        lines[i] = player->inventory[i];
    }
    show_message(renderer, font, lines, player->items);
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
    char const *class_names[CLASS_COUNT];
    for (int i = 0; i < CLASS_COUNT; ++i) {
        class_names[i] = classes[i].name;
    }
    int class_idx =
        menu_prompt(renderer, font, "Choose a class", class_names, CLASS_COUNT);
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "Welcome %s the %s!", name,
             class_names[class_idx]);
    char const *lines[] = {buffer, "Press SPACE to continue"};
    show_message(renderer, font, lines, 2);

    Player player = {320, 240, "", &classes[class_idx], {{0}}, 0, {0}};
    strncpy(player.name, name, sizeof(player.name) - 1);
    Chest chest[] = {{{280, 240, 32, 24}, false, {0}}};
    Door door[]   = {{{500, 220, 40, 40}, false, {0}}};
    Prop  prop[]  = {{{260, 260, 20, 20}}};
    Npc npc[] = {{200, 240, draw_familiar, familiar_dialog},
                 {380, 220, draw_imp,      imp_dialog},
                 {320, 180, draw_cleric,   cleric_dialog}};
    SDL_Rect familiar_rect = {npc[0].x - 8, npc[0].y - 48, 16, 48};
    SDL_Rect imp_rect      = {npc[1].x - 8, npc[1].y - 48, 16, 48};
    SDL_Rect cleric_rect   = {npc[2].x - 8, npc[2].y - 48, 16, 48};
    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                if (e.key.keysym.sym == SDLK_e) {
                    SDL_Rect pr = {player.x - 8, player.y - 48, 16, 48};
                    if (!chest[0].opened &&
                        SDL_HasIntersection(&pr, &chest[0].rect)) {
                        chest[0].opened = true;
                        inventory_add_item(&player, "Rusty Dagger");
                        char const *msg[] =
                            {"You find a rusty dagger! (added to inventory)"};
                        show_message(renderer, font, msg, 1);
                    } else if (SDL_HasIntersection(&pr, &door[0].rect)) {
                        char const *msg[] = {"The door is locked."};
                        show_message(renderer, font, msg, 1);
                    } else if (SDL_HasIntersection(&pr, &prop[0].rect)) {
                        char const *msg[] = {"A broken glass pod"};
                        show_message(renderer, font, msg, 1);
                    } else if (SDL_HasIntersection(&pr, &familiar_rect)) {
                        familiar_dialog(renderer, font);
                    } else if (SDL_HasIntersection(&pr, &imp_rect)) {
                        imp_dialog(renderer, font);
                    } else if (SDL_HasIntersection(&pr, &cleric_rect)) {
                        cleric_dialog(renderer, font);
                    }
                }
                if (e.key.keysym.sym == SDLK_i) {
                    show_inventory(renderer, font, &player);
                }
            }
        }
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if (keys[SDL_SCANCODE_LEFT]) {
            player.x -= 4;
        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            player.x += 4;
        }
        if (keys[SDL_SCANCODE_UP]) {
            player.y -= 4;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            player.y += 4;
        }
        if (player.x < 20) {
            player.x = 20;
        }
        if (player.x > 620) {
            player.x = 620;
        }
        if (player.y < 20) {
            player.y = 20;
        }
        if (player.y > 460) {
            player.y = 460;
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        draw_prop(renderer, prop[0].rect);
        draw_door(renderer, door[0].rect);
        draw_chest(renderer, chest[0].rect, chest[0].opened);
        draw_familiar(renderer, npc[0].x, npc[0].y);
        draw_imp(renderer, npc[1].x, npc[1].y);
        draw_cleric(renderer, npc[2].x, npc[2].y);
        if (player.info->id == CLASS_FIGHTER) {
            draw_warrior(renderer, player.x, player.y);
        } else if (player.info->id == CLASS_ROGUE) {
            draw_rogue(renderer, player.x, player.y);
        } else if (player.info->id == CLASS_MAGE) {
            draw_mage(renderer, player.x, player.y);
        } else if (player.info->id == CLASS_HEALER) {
            draw_cleric(renderer, player.x, player.y);
        } else if (player.info->id == CLASS_BEAST) {
            draw_familiar(renderer, player.x, player.y);
        } else if (player.info->id == CLASS_DEMON) {
            draw_imp(renderer, player.x, player.y);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow  (window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}

