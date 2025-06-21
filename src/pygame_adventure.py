import sys
from dataclasses import dataclass, field
from typing import List, Dict, Optional

import pygame


# Stick figure drawing helpers

def draw_stick_figure(screen: pygame.Surface, x: int, y: int, color: pygame.Color) -> None:
    """Draw a simple stick figure centered at (x, y)."""
    pygame.draw.circle(screen, color, (x, y - 8), 8, 2)
    pygame.draw.line(screen, color, (x, y), (x, y + 16), 2)
    pygame.draw.line(screen, color, (x, y + 4), (x - 8, y + 12), 2)
    pygame.draw.line(screen, color, (x, y + 4), (x + 8, y + 12), 2)
    pygame.draw.line(screen, color, (x, y + 16), (x - 8, y + 24), 2)
    pygame.draw.line(screen, color, (x, y + 16), (x + 8, y + 24), 2)


def draw_chest(screen: pygame.Surface, rect: pygame.Rect, opened: bool) -> None:
    pygame.draw.rect(screen, pygame.Color("sienna"), rect, 2)
    if opened:
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topleft, rect.bottomright, 2)
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topright, rect.bottomleft, 2)


def draw_door(screen: pygame.Surface, rect: pygame.Rect) -> None:
    pygame.draw.rect(screen, pygame.Color("gray"), rect, 2)
    pygame.draw.line(screen, pygame.Color("gray"), (rect.centerx, rect.top), (rect.centerx, rect.bottom), 2)


@dataclass
class NPC:
    name: str
    color: pygame.Color
    x: int
    y: int
    dialog: Optional[callable] = None
    joined: bool = False

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 16, self.y - 16, 32, 32)

    def draw(self, screen: pygame.Surface) -> None:
        draw_stick_figure(screen, self.x, self.y, self.color)


def show_message(screen: pygame.Surface, font: pygame.font.Font, lines: List[str]) -> None:
    """Display message lines until SPACE pressed."""
    clock = pygame.time.Clock()
    waiting = True
    while waiting:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            if event.type == pygame.KEYDOWN and event.key == pygame.K_SPACE:
                waiting = False
        screen.fill((0, 0, 0))
        for idx, line in enumerate(lines):
            text = font.render(line, True, pygame.Color("white"))
            screen.blit(text, (40, 40 + idx * 30))
        prompt = font.render("Press SPACE to continue", True, pygame.Color("white"))
        screen.blit(prompt, (40, screen.get_height() - 40))
        pygame.display.flip()
        clock.tick(30)


def menu_prompt(screen: pygame.Surface, font: pygame.font.Font, question: str, options: List[str]) -> int:
    """Display a numbered choice menu and return chosen index."""
    clock = pygame.time.Clock()
    choice = -1
    while choice == -1:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            if event.type == pygame.KEYDOWN:
                if pygame.K_1 <= event.key <= pygame.K_9:
                    idx = event.key - pygame.K_1
                    if idx < len(options):
                        choice = idx
        screen.fill((0, 0, 0))
        q = font.render(question, True, pygame.Color("white"))
        screen.blit(q, (40, 40))
        for idx, opt in enumerate(options):
            opt_text = font.render(f"{idx + 1}. {opt}", True, pygame.Color("white"))
            screen.blit(opt_text, (60, 80 + idx * 30))
        pygame.display.flip()
        clock.tick(30)
    return choice


def familiar_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        idx = menu_prompt(screen, font, "The familiar chirps softly.", ["\"Who are you?\"", "\"Will you help me escape?\"", "\"Let's go.\""])
        if idx == 0:
            show_message(screen, font, ["It chitters about being bound to the ship by foul magic."])
        elif idx == 1:
            show_message(screen, font, ["The creature nods enthusiastically."])
        elif idx == 2:
            show_message(screen, font, ["It hops onto your shoulder."])
            return


def warrior_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        idx = menu_prompt(screen, font, "The warrior wipes ichor from his blade.", ["\"What's your name?\"", "\"Stick with me, we can escape.\"", "\"Enough talk.\""])
        if idx == 0:
            show_message(screen, font, ["\"Call me whatever you like. Let's just survive,\" he grunts."])
        elif idx == 1:
            show_message(screen, font, ["He nods. 'Lead the way.'"])
        elif idx == 2:
            show_message(screen, font, ["He falls in behind you, ready for battle."])
            return


def cleric_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        idx = menu_prompt(screen, font, "The cleric steadies her breath.", ["\"What happened here?\"", "\"Can you heal us?\"", "\"Let's leave this ship.\""])
        if idx == 0:
            show_message(screen, font, ["'A ritual went terribly wrong,' she explains."])
        elif idx == 1:
            show_message(screen, font, ["She murmurs a prayer and a warm light surrounds you."])
        elif idx == 2:
            show_message(screen, font, ["She grabs a nearby pack and prepares to follow."])
            return


@dataclass
class Chest:
    rect: pygame.Rect
    item: str
    flag: str
    opened: bool = False

    def draw(self, screen: pygame.Surface) -> None:
        draw_chest(screen, self.rect, self.opened)


@dataclass
class Door:
    rect: pygame.Rect
    dest: str

    def draw(self, screen: pygame.Surface) -> None:
        draw_door(screen, self.rect)


@dataclass
class Player:
    x: int
    y: int
    companions: List[NPC] = field(default_factory=list)
    inventory: List[str] = field(default_factory=list)
    flags: Dict[str, bool] = field(default_factory=dict)

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 16, self.y - 16, 32, 32)

    def draw(self, screen: pygame.Surface) -> None:
        draw_stick_figure(screen, self.x, self.y, pygame.Color("white"))


@dataclass
class Room:
    name: str
    npcs: List[NPC]
    chests: List[Chest]
    doors: List[Door]


def create_rooms() -> Dict[str, Room]:
    rooms: Dict[str, Room] = {}

    rooms["Pod Room"] = Room(
        "Pod Room",
        [NPC("Familiar", pygame.Color("cyan"), 200, 240, familiar_dialog)],
        [],
        [Door(pygame.Rect(600, 220, 40, 40), "Corridor")],
    )

    rooms["Corridor"] = Room(
        "Corridor",
        [],
        [],
        [
            Door(pygame.Rect(40, 220, 40, 40), "Pod Room"),
            Door(pygame.Rect(300, 60, 40, 40), "Brig"),
            Door(pygame.Rect(300, 380, 40, 40), "Storage"),
            Door(pygame.Rect(600, 220, 40, 40), "Control Room"),
        ],
    )

    rooms["Brig"] = Room(
        "Brig",
        [NPC("Warrior", pygame.Color("red"), 320, 240, warrior_dialog)],
        [],
        [Door(pygame.Rect(40, 220, 40, 40), "Corridor")],
    )

    rooms["Storage"] = Room(
        "Storage",
        [],
        [Chest(pygame.Rect(320, 240, 32, 24), "a healing salve", "storage_chest_opened")],
        [Door(pygame.Rect(40, 220, 40, 40), "Corridor")],
    )

    rooms["Control Room"] = Room(
        "Control Room",
        [NPC("Cleric", pygame.Color("yellow"), 320, 240, cleric_dialog)],
        [],
        [
            Door(pygame.Rect(40, 220, 40, 40), "Corridor"),
            Door(pygame.Rect(600, 220, 40, 40), "Escape Pod"),
        ],
    )

    rooms["Escape Pod"] = Room(
        "Escape Pod",
        [],
        [],
        [],
    )

    return rooms


def game_end(screen: pygame.Surface, font: pygame.font.Font, player: Player) -> None:
    lines = ["You and your companions have survived!"]
    if player.companions:
        lines.append("Companions:")
        for c in player.companions:
            lines.append(f" - {c.name}")
    if player.inventory:
        lines.append("Inventory:")
        for item in player.inventory:
            lines.append(f" - {item}")
    show_message(screen, font, lines)


def main() -> None:
    pygame.init()
    screen = pygame.display.set_mode((640, 480))
    pygame.display.set_caption("Nautiloid Adventure")
    font = pygame.font.SysFont(None, 28)
    clock = pygame.time.Clock()

    name = input("Enter your character's name: ")
    char_class = input("Choose a class (Fighter/Rogue/Mage): ")
    player = Player(320, 240)

    rooms = create_rooms()
    current = rooms["Pod Room"]

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            if event.type == pygame.KEYDOWN and event.key == pygame.K_e:
                # Interactions
                pr = player.rect()
                # doors
                for door in current.doors:
                    if pr.colliderect(door.rect):
                        if door.dest == "Escape Pod":
                            current = rooms[door.dest]
                            game_end(screen, font, player)
                            running = False
                        else:
                            player.x, player.y = 320, 240
                            current = rooms[door.dest]
                        break
                # chests
                for chest in current.chests:
                    if not chest.opened and pr.colliderect(chest.rect):
                        chest.opened = True
                        player.inventory.append(chest.item)
                        player.flags[chest.flag] = True
                        show_message(screen, font, [f"You find {chest.item}!"])
                        break
                # npcs
                for npc in current.npcs:
                    if not npc.joined and pr.colliderect(npc.rect()):
                        if npc.dialog:
                            npc.dialog(screen, font)
                        npc.joined = True
                        player.companions.append(npc)
                        break

        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            player.x -= 4
        if keys[pygame.K_RIGHT]:
            player.x += 4
        if keys[pygame.K_UP]:
            player.y -= 4
        if keys[pygame.K_DOWN]:
            player.y += 4
        player.x = max(20, min(620, player.x))
        player.y = max(20, min(460, player.y))

        screen.fill((0, 0, 0))
        for door in current.doors:
            door.draw(screen)
        for chest in current.chests:
            chest.draw(screen)
        for npc in current.npcs:
            if not npc.joined:
                npc.draw(screen)
        player.draw(screen)
        for idx, comp in enumerate(player.companions):
            comp.draw(screen)
        room_text = font.render(current.name, True, pygame.Color("white"))
        screen.blit(room_text, (10, 10))
        instr = font.render("Arrows: move  E: interact", True, pygame.Color("white"))
        screen.blit(instr, (10, 440))
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
