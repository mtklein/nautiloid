#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/*
 * TODO: port features from pygame_adventure.py
 * - Show floating damage numbers and health bars
 * - Implement combat_encounter() for turn-based fights
 * - Support moving between rooms and interacting with objects
 *
 * FUTURE
 * - Use SDL_image to load textured sprites
 * - Add pathfinding for NPC movement
 * - Implement save/load functionality
 * - Add sound effects using SDL_mixer
 * - Support gamepad input
 * - Display a mini map of explored rooms
 * - Assign weapons and armor in the party menu
 * - Cache face textures for NPCs to avoid recreating them
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
#define PLAYABLE_CLASS_COUNT 4
#define MAX_COMBATANTS      8

typedef enum {
    ROOM_POD_ROOM,
    ROOM_CORRIDOR,
    ROOM_BRIG,
    ROOM_STORAGE,
    ROOM_CONTROL_ROOM,
    ROOM_ESCAPE_POD,
    ROOM_COUNT
} RoomId;

typedef struct {
    char const *name;
    Ability const *abilities;
    int ability_count;
    Attributes attributes;
    ClassId     id;
} ClassInfo;

typedef struct Npc Npc;

typedef struct {
    char name[32];
    bool value;
    char _pad[3];
} Flag;

typedef struct {
    int x;
    int y;
    char name[64];
    ClassInfo const *info;
    Npc *companions[4];
    int  companion_count;
    char inventory[8][32];
    int  items;
    Flag flags[16];
    int  flag_count;
    char _pad[4];
} Player;

typedef struct {
    SDL_Rect    rect;
    bool        opened;
    char        _pad[7];
    char const *item;
    char const *flag;
} Chest;

typedef struct {
    SDL_Rect    rect;
    char const *desc;
} Prop;

typedef struct {
    SDL_Rect    rect;
    char const *dest;
    bool        open;
    char        _pad[7];
    char const *key;
} Door;

typedef void (*NpcDraw)(SDL_Renderer *, int, int);
typedef void (*NpcDialog)(SDL_Renderer *, TTF_Font *, struct Npc *);

typedef struct Npc {
    int               x;
    int               y;
    NpcDraw           draw;
    NpcDialog         dialog;
    char              name[32];
    ClassInfo const   *info;
    bool              joined;
    bool              enemy;
    char              _pad[6];
} Npc;

typedef struct {
    Chest      *chest;
    Prop       *prop;
    Door       *door;
    Npc        *npc;
    char const *name;
    char const *shape;
    int         chests;
    int         props;
    int         doors;
    int         npcs;
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

static AttrKind const attack_attr[] = {
    [CLASS_FIGHTER] = ATTR_STRENGTH,
    [CLASS_ROGUE]   = ATTR_AGILITY,
    [CLASS_MAGE]    = ATTR_WISDOM,
    [CLASS_HEALER]  = ATTR_WISDOM,
    [CLASS_BEAST]   = ATTR_STRENGTH,
    [CLASS_DEMON]   = ATTR_STRENGTH,
};

static AttrKind const defense_attr[] = {
    [CLASS_FIGHTER] = ATTR_STRENGTH,
    [CLASS_ROGUE]   = ATTR_AGILITY,
    [CLASS_MAGE]    = ATTR_WISDOM,
    [CLASS_HEALER]  = ATTR_WISDOM,
    [CLASS_BEAST]   = ATTR_AGILITY,
    [CLASS_DEMON]   = ATTR_STRENGTH,
};

static int
attr_value(Attributes const *a, AttrKind kind) {
    switch (kind) {
    case ATTR_STRENGTH:
        return a->strength;
    case ATTR_AGILITY:
        return a->agility;
    case ATTR_WISDOM:
        return a->wisdom;
    }
    return 0;
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
              char const *lines[], int line_count,
              char const *speaker, SDL_Texture *face,
              char const *footer) {
    int   width  = 0;
    int   height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    int        box_height = height / 3;
    SDL_Rect   rect       = {20, height - box_height - 20,
                             width - 40, box_height};
    SDL_Color const top       = {100, 100, 255, 255},
                       botclr = {40, 40, 180, 255};
    draw_gradient_rect(renderer, rect, top, botclr);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawRect(renderer, &rect);

    int y = rect.y + 10;
    if (speaker) {
        SDL_Texture *name =
            render_text(renderer, font, speaker,
                        (SDL_Color){255, 255, 255, 255});
        if (name) {
            SDL_Rect dst = {rect.x + 10, y, 0, 0};
            SDL_QueryTexture(name, NULL, NULL, &dst.w, &dst.h);
            SDL_RenderCopy(renderer, name, NULL, &dst);
            SDL_DestroyTexture(name);
        }
        y += 30;
    }
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
    if (face) {
        SDL_Rect dst = {rect.x - 42, rect.y + 8, 32, 48};
        SDL_RenderCopy(renderer, face, NULL, &dst);
    }
    if (footer) {
        SDL_Texture *text =
            render_text(renderer, font, footer, (SDL_Color){255, 255, 255, 255});
        if (text) {
            SDL_Rect dst = {0, rect.y + rect.h - 26, 0, 0};
            SDL_QueryTexture(text, NULL, NULL, &dst.w, &dst.h);
            dst.x = rect.x + (rect.w - dst.w) / 2;
            SDL_RenderCopy(renderer, text, NULL, &dst);
            SDL_DestroyTexture(text);
        }
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
            if (e.type == SDL_KEYDOWN &&
                (e.key.keysym.sym == SDLK_SPACE || e.key.keysym.sym == SDLK_e)) {
                waiting = false;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        char const *all[9];
        int         n = 0;
        for (int i = 0; i < line_count && i < 9; ++i) {
            all[n++] = lines[i];
        }
        draw_text_box(renderer, font, all, n,
                      NULL, NULL,
                      "Press SPACE or E to continue");
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

typedef struct {
    float     x;
    float     y;
    float     vy;
    SDL_Color color;
    int       life;
} Firework;

static void
star_wars_scroll(SDL_Renderer *renderer, TTF_Font *font,
                 char const *lines[], int line_count) {
    int width  = 0;
    int height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    SDL_Texture *texts[16];
    SDL_Rect     rects[16];
    int          total = 0;
    for (int i = 0; i < line_count && i < 16; ++i) {
        texts[i] =
            render_text(renderer, font, lines[i], (SDL_Color){255, 255, 0, 255});
        rects[i] = (SDL_Rect){0, 0, 0, 0};
        SDL_QueryTexture(texts[i], NULL, NULL, &rects[i].w, &rects[i].h);
        rects[i].x = (width - rects[i].w) / 2;
        total += rects[i].h + 8;
    }
    Firework fireworks[32];
    int      fw_count = 0;
    int      offset   = height;
    int      hold     = 0;
    bool     running  = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (fw_count < 32 && (rand() % 100) < 5) {
            fireworks[fw_count] = (Firework){
                (float)(50 + rand() % (width - 100)),
                (float)height,
                2.0f + (float)(rand() % 20) / 5.0f,
                {(Uint8)(128 + rand() % 128),
                 (Uint8)(128 + rand() % 128),
                 (Uint8)(128 + rand() % 128),
                 255},
                0};
            fw_count++;
        }
        for (int i = 0; i < fw_count; ++i) {
            Firework *fw = &fireworks[i];
            fw->y -= fw->vy;
            fw->life++;
            SDL_SetRenderDrawColor(renderer, fw->color.r, fw->color.g,
                                   fw->color.b, 255);
            SDL_RenderDrawPoint(renderer, (int)fw->x, (int)fw->y);
            if (fw->life > 60) {
                fireworks[i] = fireworks[--fw_count];
                i--;
            }
        }
        int y = offset;
        for (int i = 0; i < line_count && i < 16; ++i) {
            rects[i].y = y;
            SDL_RenderCopy(renderer, texts[i], NULL, &rects[i]);
            y += rects[i].h + 8;
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
        if (offset + total > 0) {
            offset--;
        } else if (++hold > 60) {
            running = false;
        }
    }
    for (int i = 0; i < line_count && i < 16; ++i) {
        SDL_DestroyTexture(texts[i]);
    }
}

static int
menu_prompt(SDL_Renderer *renderer, TTF_Font *font, char const *question,
            char const *options[], int option_count,
            char const *speaker, SDL_Texture *face) {
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
        draw_text_box(renderer, font, lines, option_count + 1,
                      speaker, face, NULL);
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
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect eye1 = {x - 3, y - 46, 2, 2};
    SDL_Rect eye2 = {x + 1, y - 46, 2, 2};
    SDL_RenderFillRect(renderer, &eye1);
    SDL_RenderFillRect(renderer, &eye2);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawLine(renderer, x - 2, y - 42, x + 2, y - 42);
}

static void
draw_warrior(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){178, 34, 34, 255});
    SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
    SDL_Rect band = {x - 5, y - 52, 10, 3};
    SDL_RenderFillRect(renderer, &band);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 10, y - 36);
}

static void
draw_rogue(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){107, 142, 35, 255});
    SDL_Rect hood = {x - 6, y - 48, 12, 8};
    SDL_SetRenderDrawColor(renderer, 85, 107, 47, 255);
    SDL_RenderFillRect(renderer, &hood);
    SDL_SetRenderDrawColor(renderer, 192, 192, 192, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 10, y - 30);
}

static void
draw_mage(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){106, 90, 205, 255});
    SDL_SetRenderDrawColor(renderer, 128, 0, 128, 255);
    int const hat_lines = 12;
    for (int i = 0; i < hat_lines; ++i) {
        int dx = (hat_lines - 1 - i) / 2;
        SDL_RenderDrawLine(renderer, x - dx, y - 48 - i,
                           x + dx, y - 48 - i);
    }
    SDL_SetRenderDrawColor(renderer, 160, 82, 45, 255);
    SDL_RenderDrawLine(renderer, x + 6, y - 20, x + 6, y - 42);
    SDL_Rect knob = {x + 4, y - 46, 4, 4};
    SDL_RenderFillRect(renderer, &knob);
}

static void
draw_cleric(SDL_Renderer *renderer, int x, int y) {
    draw_humanoid(renderer, x, y, (SDL_Color){135, 206, 235, 255});
    SDL_SetRenderDrawColor(renderer, 240, 230, 140, 255);
    SDL_Rect hat = {x - 5, y - 52, 10, 2};
    SDL_RenderFillRect(renderer, &hat);
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

static SDL_Texture *
make_face(SDL_Renderer *renderer, NpcDraw draw) {
    SDL_Texture *face = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_TARGET, 32, 48);
    SDL_SetTextureBlendMode(face, SDL_BLENDMODE_BLEND);
    SDL_Texture *prev = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, face);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    draw(renderer, 16, 48);
    SDL_SetRenderTarget(renderer, prev);
    return face;
}

static SDL_Texture *
npc_face(SDL_Renderer *renderer, Npc const *npc) {
    return make_face(renderer, npc->draw);
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

typedef void (*DrawBg)(void *);

static void
draw_health_bar(SDL_Renderer *renderer, int x, int y,
                int hp, int max_hp) {
    float      ratio = (float)hp / (float)max_hp;
    SDL_Rect   bar   = {x - 20, y - 52, 40, 5};
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer, &bar);
    SDL_Rect fill = {bar.x, bar.y, (int)(40.0f * ratio), 5};
    SDL_Color color = {0, 255, 0, 255};
    if (ratio < 0.25f) {
        color = (SDL_Color){255, 0, 0, 255};
    } else if (ratio < 0.5f) {
        color = (SDL_Color){255, 255, 0, 255};
    }
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &fill);
}

static void
float_number(SDL_Renderer *renderer, TTF_Font *font, char const *lines[],
             int line_count, char const *text, SDL_Color color, SDL_Point pos,
             DrawBg draw_bg, void *ctx) {
    for (int i = 0; i < 30; ++i) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                exit(0);
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (draw_bg) {
            draw_bg(ctx);
        }
        draw_text_box(renderer, font, lines, line_count, NULL, NULL, NULL);
        SDL_Texture *num = render_text(renderer, font, text, color);
        if (num) {
            SDL_Rect dst = {pos.x, pos.y - i, 0, 0};
            SDL_QueryTexture(num, NULL, NULL, &dst.w, &dst.h);
            dst.x -= dst.w / 2;
            dst.y -= dst.h / 2;
            SDL_RenderCopy(renderer, num, NULL, &dst);
            SDL_DestroyTexture(num);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

static void
familiar_dialog(SDL_Renderer *renderer, TTF_Font *font, Npc *npc) {
    SDL_Texture *face = NULL;
    face = npc_face(renderer, npc);
    char const *opts[] = {"\"Who are you?\"",
                           "\"Will you help me escape?\"",
                           "\"Let's go.\""};
    while (1) {
        int idx = menu_prompt(renderer, font, "The familiar chirps softly.",
                              opts, 3, npc->name, face);
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
    SDL_DestroyTexture(face);
}

static void
cleric_dialog(SDL_Renderer *renderer, TTF_Font *font, Npc *npc) {
    SDL_Texture *face = npc_face(renderer, npc);
    char const *opts[] = {"\"What happened here?\"",
                           "\"Can you heal us?\"",
                           "\"Let's leave this ship.\""};
    while (1) {
        int idx = menu_prompt(renderer, font, "The cleric steadies her breath.",
                              opts, 3, npc->name, face);
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
    SDL_DestroyTexture(face);
}

static void
imp_dialog(SDL_Renderer *renderer, TTF_Font *font, Npc *npc) {
    (void)npc;
    char const *msg[] = {"The imp hisses at you."};
    show_message(renderer, font, msg, 1);
}

static void
warrior_dialog(SDL_Renderer *renderer, TTF_Font *font, Npc *npc) {
    SDL_Texture *face = npc_face(renderer, npc);
    char const *opts[] = {"\"What's your name?\"",
                           "\"Stick with me, we can escape.\"",
                           "\"Enough talk.\""};
    while (1) {
        int idx = menu_prompt(renderer, font,
                              "The warrior wipes ichor from his blade.", opts,
                              3, npc->name, face);
        if (idx == 0) {
            char const *msg[] =
                {"\"Call me whatever you like. Let's just survive,\" he grunts."};
            show_message(renderer, font, msg, 1);
        } else if (idx == 1) {
            char const *msg[] = {"He nods. 'Lead the way.'"};
            show_message(renderer, font, msg, 1);
        } else {
            char const *msg[] = {"He falls in behind you, ready for battle."};
            show_message(renderer, font, msg, 1);
            break;
        }
    }
    SDL_DestroyTexture(face);
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
        draw_text_box(renderer, font, lines, 2, NULL, NULL, NULL);
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

static bool
inventory_has_item(Player const *player, char const *item) {
    for (int i = 0; i < player->items; ++i) {
        if (0 == strcmp(player->inventory[i], item)) {
            return true;
        }
    }
    return false;
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

static void
update_companions(Player *player) {
    int lead_x = player->x;
    int lead_y = player->y;
    for (int i = 0; i < player->companion_count; ++i) {
        Npc *comp = player->companions[i];
        int dx = lead_x - comp->x;
        int dy = lead_y - comp->y;
        double dist = hypot((double)dx, (double)dy);
        if (dist > 64.0) {
            comp->x += (int)(0.1 * dx);
            comp->y += (int)(0.1 * dy);
        }
        lead_x = comp->x;
        lead_y = comp->y;
    }
}

static void
npc_join(Player *player, Npc *npc) {
    if (player->companion_count < 4 && !npc->joined && !npc->enemy) {
        player->companions[player->companion_count] = npc;
        player->companion_count++;
        npc->joined = true;
    }
}

static void
npc_dismiss(Player *player, int index) {
    player->companions[index]->joined = false;
    for (int i = index; i < player->companion_count - 1; ++i) {
        player->companions[i] = player->companions[i + 1];
    }
    player->companion_count--;
}

static Room *
find_room(Room rooms[ROOM_COUNT], char const *name) {
    for (int i = 0; i < ROOM_COUNT; ++i) {
        if (0 == strcmp(rooms[i].name, name)) {
            return &rooms[i];
        }
    }
    return NULL;
}

static void
create_rooms(Room rooms[ROOM_COUNT]) {
    static Npc pod_npcs[] = {{200, 240, draw_familiar, familiar_dialog,
                              "Familiar", &classes[CLASS_BEAST], false,
                              false, {0}}};
    static Chest pod_chests[] = {
        {{280, 240, 32, 24}, false, {0}, "small key", "pod_key"}};
    static Prop pod_props[] = {{{260, 260, 20, 20}, "A broken glass pod"}};
    static Door pod_doors[] = {{{600, 220, 40, 40}, "Corridor", false,
                                {0}, "small key"}};
    rooms[ROOM_POD_ROOM] =
        (Room){pod_chests, pod_props, pod_doors, pod_npcs, "Pod Room",
               "circle", 1, 1, 1, 1};

    static Prop cor_props[] = {{{320, 240, 16, 16}, "A flickering wall torch"}};
    static Door cor_doors[] = {{{40, 220, 40, 40}, "Pod Room", false,
                                {0}, "small key"},
                               {{300, 60, 40, 40}, "Brig", true, {0}, NULL},
                               {{300, 380, 40, 40}, "Storage", true, {0}, NULL},
                               {{600, 220, 40, 40}, "Control Room", false,
                                {0}, "control key"}};
    rooms[ROOM_CORRIDOR] =
        (Room){NULL, cor_props, cor_doors, NULL, "Corridor", "wide", 0, 1, 4, 0};

    static Npc brig_npcs[] = {{320, 240, draw_warrior, warrior_dialog,
                               "Warrior", &classes[CLASS_FIGHTER], false,
                               false, {0}},
                              {380, 220, draw_imp, imp_dialog, "Imp",
                               &classes[CLASS_DEMON], false, true, {0}},
                              {300, 200, draw_imp, imp_dialog, "Imp",
                               &classes[CLASS_DEMON], false, true, {0}},
                              {340, 260, draw_imp, imp_dialog, "Imp",
                               &classes[CLASS_DEMON], false, true, {0}}};
    static Chest brig_chests[] = {
        {{280, 240, 32, 24}, false, {0}, "an iron sword", "brig_sword"},
        {{360, 240, 32, 24}, false, {0}, "control key", "brig_key"}};
    static Prop brig_props[] = {{{360, 260, 16, 16}, "Chains hang from the wall"}};
    static Door brig_doors[] = {{{300, 380, 40, 40}, "Corridor", true, {0}, NULL}};
    rooms[ROOM_BRIG] = (Room){brig_chests, brig_props, brig_doors, brig_npcs,
                              "Brig", "square", 2, 1, 1, 4};

    static Chest storage_chests[] = {
        {{320, 240, 32, 24}, false, {0}, "a healing salve",
         "storage_chest_opened"},
        {{360, 240, 32, 24}, false, {0}, "leather armor", "storage_armor"}};
    static Prop storage_props[] = {{{300, 300, 20, 20}, "Crates of supplies"}};
    static Door storage_doors[] = {{{300, 60, 40, 40}, "Corridor", true, {0}, NULL}};
    rooms[ROOM_STORAGE] = (Room){storage_chests, storage_props, storage_doors,
                                 NULL, "Storage", "tall", 2, 1, 1, 0};

    static Npc control_npcs[] = {{320, 240, draw_cleric, cleric_dialog,
                                  "Cleric", &classes[CLASS_HEALER], false,
                                  false, {0}},
                                 {380, 200, draw_mage, NULL, "Cultist",
                                  &classes[CLASS_MAGE], false, true, {0}},
                                 {260, 200, draw_rogue, NULL, "Acolyte",
                                  &classes[CLASS_ROGUE], false, true, {0}}};
    static Chest control_chests[] = {
        {{320, 300, 32, 24}, false, {0}, "mystic staff", "control_staff"}};
    static Prop control_props[] = {{{320, 180, 20, 20}, "A glowing altar"}};
    static Door control_doors[] = {{{40, 220, 40, 40}, "Corridor", false,
                                    {0}, "control key"},
                                   {{600, 220, 40, 40}, "Escape Pod", true,
                                    {0}, NULL}};
    rooms[ROOM_CONTROL_ROOM] =
        (Room){control_chests, control_props, control_doors, control_npcs,
               "Control Room", "control", 1, 1, 2, 3};

    rooms[ROOM_ESCAPE_POD] = (Room){NULL, NULL, NULL, NULL, "Escape Pod",
                                    "square", 0, 0, 0, 0};
}

static void
show_party_menu(SDL_Renderer *renderer, TTF_Font *font, Player *player) {
    if (player->companion_count == 0) {
        char const *lines[] = {"You have no companions."};
        show_message(renderer, font, lines, 1);
        return;
    }
    char const *options[4];
    char        name_bufs[4][32];
    for (int i = 0; i < player->companion_count; ++i) {
        snprintf(name_bufs[i], sizeof(name_bufs[i]), "%s", player->companions[i]->name);
        options[i] = name_bufs[i];
    }
    int idx = menu_prompt(renderer, font, "Choose companion", options,
                          player->companion_count, NULL, NULL);
    char const *acts[] = {"Talk", "Dismiss", "Back"};
    int action = menu_prompt(renderer, font, "Party action", acts, 3, NULL, NULL);
    if (action == 0) {
        player->companions[idx]->dialog(renderer, font,
                                        player->companions[idx]);
    } else if (action == 1) {
        npc_dismiss(player, idx);
    }
}

typedef struct {
    bool      is_player;
    char      _pad[7];
    Npc      *npc;
} Fighter;

typedef struct {
    SDL_Renderer *renderer;
    Fighter      *fighters;
    int          *hp;
    int          *max_hp;
    SDL_Point    *pos;
    Player       *player;
    int           count;
    char          _pad[4];
} FightCtx;

static void
draw_fight(void *data) {
    FightCtx *ctx = data;
    for (int i = 0; i < ctx->count; ++i) {
        if (ctx->hp[i] <= 0) {
            continue;
        }
        int x = ctx->pos[i].x;
        int y = ctx->pos[i].y;
        if (ctx->fighters[i].is_player) {
            switch (ctx->player->info->id) {
            case CLASS_FIGHTER:
                draw_warrior(ctx->renderer, x, y);
                break;
            case CLASS_ROGUE:
                draw_rogue(ctx->renderer, x, y);
                break;
            case CLASS_MAGE:
                draw_mage(ctx->renderer, x, y);
                break;
            case CLASS_HEALER:
                draw_cleric(ctx->renderer, x, y);
                break;
            case CLASS_BEAST:
                draw_familiar(ctx->renderer, x, y);
                break;
            case CLASS_DEMON:
                draw_imp(ctx->renderer, x, y);
                break;
            }
        } else {
            ctx->fighters[i].npc->draw(ctx->renderer, x, y);
        }
        draw_health_bar(ctx->renderer, x, y, ctx->hp[i], ctx->max_hp[i]);
    }
}

static bool
combat_encounter(SDL_Renderer *renderer, TTF_Font *font, Player *player,
                 Npc **enemies, int enemy_count) {
    Fighter   fighters[MAX_COMBATANTS];
    SDL_Point pos[MAX_COMBATANTS];
    int       hp[MAX_COMBATANTS];
    int       max_hp[MAX_COMBATANTS];

    int ally_count = 1 + player->companion_count;
    int total      = ally_count + enemy_count;

    fighters[0] = (Fighter){true, {0}, NULL};
    pos[0]      = (SDL_Point){100, 300};
    hp[0]       = player->info->attributes.hp;
    max_hp[0]   = player->info->attributes.hp;

    for (int i = 0; i < player->companion_count; ++i) {
        fighters[1 + i] = (Fighter){false, {0}, player->companions[i]};
        pos[1 + i]       = (SDL_Point){100, 240 - i * 60};
        hp[1 + i]        = fighters[1 + i].npc->info->attributes.hp;
        max_hp[1 + i]    = fighters[1 + i].npc->info->attributes.hp;
    }

    for (int i = 0; i < enemy_count; ++i) {
        fighters[ally_count + i] = (Fighter){false, {0}, enemies[i]};
        pos[ally_count + i]      = (SDL_Point){500, 300 - i * 60};
        hp[ally_count + i]       = enemies[i]->info->attributes.hp;
        max_hp[ally_count + i]   = enemies[i]->info->attributes.hp;
    }

    FightCtx ctx = {renderer, fighters, hp, max_hp, pos, player, total, {0}};

    while (hp[0] > 0) {
        bool enemy_alive = false;
        for (int i = ally_count; i < total; ++i) {
            if (hp[i] > 0) {
                enemy_alive = true;
                break;
            }
        }
        if (!enemy_alive) {
            char const *msg[] = {"You are victorious!"};
            show_message(renderer, font, msg, 1);
            return true;
        }

        int order[MAX_COMBATANTS];
        int score[MAX_COMBATANTS];
        for (int i = 0; i < total; ++i) {
            order[i] = i;
            if (fighters[i].is_player) {
                score[i] = attr_value(&player->info->attributes,
                                      attack_attr[player->info->id]) +
                           (rand() % 3);
            } else {
                score[i] = attr_value(&fighters[i].npc->info->attributes,
                                      attack_attr[fighters[i].npc->info->id]) +
                           (rand() % 3);
            }
        }
        for (int i = 0; i < total - 1; ++i) {
            for (int j = i + 1; j < total; ++j) {
                if (score[order[i]] < score[order[j]]) {
                    int t   = order[i];
                    order[i] = order[j];
                    order[j] = t;
                }
            }
        }

        for (int oi = 0; oi < total; ++oi) {
            int idx = order[oi];
            if (hp[idx] <= 0) {
                continue;
            }
            if (hp[0] <= 0) {
                break;
            }

            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    exit(0);
                }
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            draw_fight(&ctx);
            SDL_RenderPresent(renderer);
            SDL_Delay(16);

            if (idx == 0) {
                char const *opts[4];
                for (int i = 0; i < player->info->ability_count; ++i) {
                    opts[i] = player->info->abilities[i].name;
                }
                NpcDraw face_draw;
                switch (player->info->id) {
                case CLASS_FIGHTER:
                    face_draw = draw_warrior;
                    break;
                case CLASS_ROGUE:
                    face_draw = draw_rogue;
                    break;
                case CLASS_MAGE:
                    face_draw = draw_mage;
                    break;
                case CLASS_HEALER:
                    face_draw = draw_cleric;
                    break;
                case CLASS_BEAST:
                    face_draw = draw_familiar;
                    break;
                case CLASS_DEMON:
                    face_draw = draw_imp;
                    break;
                }
                SDL_Texture *face = make_face(renderer, face_draw);
                int abidx = menu_prompt(renderer, font, "Choose action", opts,
                                       player->info->ability_count, player->name,
                                       face);
                SDL_DestroyTexture(face);
                Ability const *ab = &player->info->abilities[abidx];
                int targets[MAX_COMBATANTS];
                int tcount = 0;
                if (strcmp(ab->target, "enemy") == 0) {
                    for (int i = ally_count; i < total; ++i) {
                        if (hp[i] > 0) {
                            targets[tcount++] = i;
                        }
                    }
                } else {
                    for (int i = 0; i < ally_count; ++i) {
                        if (hp[i] > 0) {
                            targets[tcount++] = i;
                        }
                    }
                }
                int tidx = 0;
                if (tcount > 1) {
                    char const *names[4];
                    for (int i = 0; i < tcount; ++i) {
                        if (targets[i] == 0) {
                            names[i] = player->name;
                        } else {
                            names[i] = fighters[targets[i]].npc->name;
                        }
                    }
                    tidx = menu_prompt(renderer, font, "Choose target", names,
                                       tcount, NULL, NULL);
                }
                int target = targets[tidx];
                Attributes const *aatr = fighters[idx].is_player
                                              ? &player->info->attributes
                                              : &fighters[idx].npc->info->attributes;
                Attributes const *datr = (target == 0)
                                              ? &player->info->attributes
                                              : &fighters[target].npc->info->attributes;
                int attack_val = ab->power +
                                attr_value(aatr, attack_attr[player->info->id]);
                int defense_val =
                    attr_value(datr,
                               defense_attr[fighters[target].is_player
                                               ? player->info->id
                                               : fighters[target].npc->info->id]);
                int dmg = attack_val - defense_val / 2;
                if (dmg < 1) {
                    dmg = 1;
                }
                char const *msg[] = {""};
                char        num[16];
                if (strcmp(ab->target, "enemy") == 0) {
                    hp[target] -= dmg;
                    snprintf(num, sizeof(num), "-%d", dmg);
                    float_number(renderer, font, msg, 0, num,
                                 (SDL_Color){255, 255, 255, 255}, pos[target],
                                 draw_fight, &ctx);
                } else {
                    if (hp[target] + dmg > max_hp[target]) {
                        dmg = max_hp[target] - hp[target];
                    }
                    hp[target] += dmg;
                    snprintf(num, sizeof(num), "+%d", dmg);
                    float_number(renderer, font, msg, 0, num,
                                 (SDL_Color){0, 255, 0, 255}, pos[target],
                                 draw_fight, &ctx);
                }
            } else {
                Fighter *actor = &fighters[idx];
                Ability const *ab = &actor->npc->info->abilities[0];
                int target = (idx >= ally_count) ? 0
                                                : ally_count; /* first enemy */
                while (target < total && hp[target] <= 0) {
                    target++;
                }
                if (target >= total) {
                    continue;
                }
                Attributes const *aatr = &actor->npc->info->attributes;
                Attributes const *datr =
                    (target == 0) ? &player->info->attributes
                                  : &fighters[target].npc->info->attributes;
                int attack_val = ab->power +
                                attr_value(aatr, attack_attr[actor->npc->info->id]);
                int defense_val =
                    attr_value(datr,
                               defense_attr[fighters[target].is_player
                                               ? player->info->id
                                               : fighters[target].npc->info->id]);
                int dmg = attack_val - defense_val / 2;
                if (dmg < 1) {
                    dmg = 1;
                }
                char const *msg[] = {""};
                char        num[16];
                hp[target] -= dmg;
                SDL_Color col = (idx >= ally_count)
                                    ? (SDL_Color){255, 0, 0, 255}
                                    : (SDL_Color){255, 255, 255, 255};
                snprintf(num, sizeof(num), "-%d", dmg);
                float_number(renderer, font, msg, 0, num, col, pos[target],
                             draw_fight, &ctx);
            }
        }
        if (hp[0] <= 0) {
            break;
        }
    }
    char const *msg[] = {"You were defeated..."};
    show_message(renderer, font, msg, 1);
    return false;
}

static char const *
interaction_hint(Player const *player, Room const *room) {
    SDL_Rect pr = {player->x - 8, player->y - 48, 16, 48};
    for (int i = 0; i < room->chests; ++i) {
        if (!room->chest[i].opened &&
            SDL_HasIntersection(&pr, &room->chest[i].rect)) {
            return "e - open chest";
        }
    }
    for (int i = 0; i < room->doors; ++i) {
        if (SDL_HasIntersection(&pr, &room->door[i].rect)) {
            return "e - open door";
        }
    }
    for (int i = 0; i < room->props; ++i) {
        if (SDL_HasIntersection(&pr, &room->prop[i].rect)) {
            return "e - inspect";
        }
    }
    for (int i = 0; i < room->npcs; ++i) {
        if (!room->npc[i].joined) {
            SDL_Rect nr = {room->npc[i].x - 8, room->npc[i].y - 48, 16, 48};
            if (SDL_HasIntersection(&pr, &nr)) {
                return "e - talk";
            }
        }
    }
    return NULL;
}

static void
draw_instructions(SDL_Renderer *renderer, TTF_Font *font, char const *hint) {
    int        w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    char       buffer[64];
    if (hint) {
        snprintf(buffer, sizeof(buffer), "i - inventory  p - party  %s", hint);
    } else {
        snprintf(buffer, sizeof(buffer), "i - inventory  p - party");
    }
    SDL_Texture *text =
        render_text(renderer, font, buffer, (SDL_Color){255, 255, 255, 255});
    if (text) {
        SDL_Rect dst = {10, h - 40, 0, 0};
        SDL_QueryTexture(text, NULL, NULL, &dst.w, &dst.h);
        SDL_RenderCopy(renderer, text, NULL, &dst);
        SDL_DestroyTexture(text);
    }
}

static void
game_end(SDL_Renderer *renderer, TTF_Font *font, Player const *player) {
    char lines[16][64];
    char const *msgs[16];
    int n = 0;
    snprintf(lines[n], sizeof(lines[n]),
             "You and your companions have survived!");
    msgs[n] = lines[n];
    n++;
    if (player->companion_count > 0) {
        msgs[n] = "Companions:";
        n++;
        for (int i = 0; i < player->companion_count; ++i) {
            snprintf(lines[n], sizeof(lines[n]), " - %s",
                     player->companions[i]->name);
            msgs[n] = lines[n];
            n++;
        }
    }
    if (player->items > 0) {
        msgs[n] = "Inventory:";
        n++;
        for (int i = 0; i < player->items; ++i) {
            snprintf(lines[n], sizeof(lines[n]), " - %s",
                     player->inventory[i]);
            msgs[n] = lines[n];
            n++;
        }
    }
    star_wars_scroll(renderer, font, msgs, n);
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
    srand((unsigned)time(NULL));

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
    char const *class_names[PLAYABLE_CLASS_COUNT];
    for (int i = 0; i < PLAYABLE_CLASS_COUNT; ++i) {
        class_names[i] = classes[i].name;
    }
    int class_idx = menu_prompt(renderer, font, "Choose a class", class_names,
                                PLAYABLE_CLASS_COUNT, NULL, NULL);
    static char buffer[128];
    snprintf(buffer, sizeof(buffer), "Welcome %s the %s!", name,
             class_names[class_idx]);
    char const *lines[] = {buffer};
    show_message(renderer, font, lines, 1);

    Player player = {.x = 320,
                     .y = 240,
                     .name = "",
                     .info = &classes[class_idx],
                     .companions = {NULL},
                     .companion_count = 0,
                     .inventory = {{0}},
                     .items = 0,
                     .flags = {{0}},
                     .flag_count = 0,
                     ._pad = {0}};
    strncpy(player.name, name, sizeof(player.name) - 1);

    Room rooms[ROOM_COUNT];
    create_rooms(rooms);
    Room *current = &rooms[ROOM_POD_ROOM];

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
                    bool handled = false;
                    for (int i = 0; i < current->chests && !handled; ++i) {
                        if (!current->chest[i].opened &&
                            SDL_HasIntersection(&pr, &current->chest[i].rect)) {
                            current->chest[i].opened = true;
                            inventory_add_item(&player, current->chest[i].item);
                            char        found[64];
                            snprintf(found, sizeof(found),
                                     "You find %s!", current->chest[i].item);
                            char const *msg[] = {found};
                            show_message(renderer, font, msg, 1);
                            handled = true;
                        }
                    }
                    for (int i = 0; i < current->doors && !handled; ++i) {
                        if (SDL_HasIntersection(&pr, &current->door[i].rect)) {
                            Door *door = &current->door[i];
                            if (!door->open) {
                                if (!door->key ||
                                    inventory_has_item(&player, door->key)) {
                                    door->open = true;
                                    Room *dest = find_room(rooms, door->dest);
                                    if (dest) {
                                        for (int j = 0; j < dest->doors; ++j) {
                                            if (0 == strcmp(dest->door[j].dest,
                                                            current->name)) {
                                                dest->door[j].open = true;
                                                break;
                                            }
                                        }
                                    }
                                    char const *msg[] = {
                                        "You unlock the door with the key."};
                                    show_message(renderer, font, msg, 1);
                                } else {
                                    char const *msg[] = {"The door is locked."};
                                    show_message(renderer, font, msg, 1);
                                }
                                handled = true;
                            } else {
                                Room *dest = find_room(rooms, door->dest);
                                if (dest) {
                                    Door *back = NULL;
                                    for (int j = 0; j < dest->doors; ++j) {
                                        if (0 == strcmp(dest->door[j].dest,
                                                        current->name)) {
                                            back = &dest->door[j];
                                            break;
                                        }
                                    }
                                    if (back) {
                                        player.x = back->rect.x + back->rect.w / 2;
                                        player.y = back->rect.y + back->rect.h / 2;
                                    } else {
                                        player.x = 320;
                                        player.y = 240;
                                    }
                                    current = dest;
                                    if (dest == &rooms[ROOM_ESCAPE_POD]) {
                                        game_end(renderer, font, &player);
                                        running = false;
                                    }
                                }
                                handled = true;
                            }
                        }
                    }
                    for (int i = 0; i < current->props && !handled; ++i) {
                        if (SDL_HasIntersection(&pr, &current->prop[i].rect)) {
                            char const *msg[] = {current->prop[i].desc};
                            show_message(renderer, font, msg, 1);
                            handled = true;
                        }
                    }
                    for (int i = 0; i < current->npcs && !handled; ++i) {
                        SDL_Rect nr = {current->npc[i].x - 8,
                                       current->npc[i].y - 48, 16, 48};
                        if (!current->npc[i].joined &&
                            SDL_HasIntersection(&pr, &nr)) {
                            if (current->npc[i].enemy) {
                                Npc *encounter[4];
                                int  ec = 0;
                                for (int j = 0; j < current->npcs; ++j) {
                                    if (current->npc[j].enemy &&
                                        !current->npc[j].joined) {
                                        encounter[ec++] = &current->npc[j];
                                    }
                                }
                                if (combat_encounter(renderer, font, &player,
                                                    encounter, ec)) {
                                    for (int j = 0; j < ec; ++j) {
                                        encounter[j]->joined = true;
                                    }
                                } else {
                                    running = false;
                                }
                            } else {
                                current->npc[i].dialog(renderer, font,
                                                       &current->npc[i]);
                                npc_join(&player, &current->npc[i]);
                            }
                            handled = true;
                        }
                    }
                }
                if (e.key.keysym.sym == SDLK_i) {
                    show_inventory(renderer, font, &player);
                }
                if (e.key.keysym.sym == SDLK_p) {
                    show_party_menu(renderer, font, &player);
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
        update_companions(&player);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        for (int i = 0; i < current->props; ++i) {
            draw_prop(renderer, current->prop[i].rect);
        }
        for (int i = 0; i < current->doors; ++i) {
            draw_door(renderer, current->door[i].rect);
        }
        for (int i = 0; i < current->chests; ++i) {
            draw_chest(renderer, current->chest[i].rect,
                       current->chest[i].opened);
        }
        for (int i = 0; i < current->npcs; ++i) {
            current->npc[i].draw(renderer, current->npc[i].x,
                                 current->npc[i].y);
        }
        switch (player.info->id) {
        case CLASS_FIGHTER:
            draw_warrior(renderer, player.x, player.y);
            break;
        case CLASS_ROGUE:
            draw_rogue(renderer, player.x, player.y);
            break;
        case CLASS_MAGE:
            draw_mage(renderer, player.x, player.y);
            break;
        case CLASS_HEALER:
            draw_cleric(renderer, player.x, player.y);
            break;
        case CLASS_BEAST:
            draw_familiar(renderer, player.x, player.y);
            break;
        case CLASS_DEMON:
            draw_imp(renderer, player.x, player.y);
            break;
        }
        for (int i = 0; i < player.companion_count; ++i) {
            Npc *comp = player.companions[i];
            comp->draw(renderer, comp->x, comp->y);
        }
        char const *hint = interaction_hint(&player, current);
        draw_instructions(renderer, font, hint);
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

