from __future__ import annotations
# Postpone evaluation of type hints so forward references like `NPC` work

import sys
from dataclasses import dataclass, field
from typing import List, Dict, Optional

import pygame
import random


@dataclass
class Ability:
    """Simple action that can affect a target."""

    name: str
    target: str  # "enemy" or "ally"
    power: int


@dataclass
class Attributes:
    strength: int
    agility: int
    wisdom: int
    hp: int


CLASS_ABILITIES: Dict[str, List[Ability]] = {
    "Fighter": [Ability("Strike", "enemy", 3), Ability("Power Attack", "enemy", 5)],
    "Rogue": [Ability("Stab", "enemy", 3), Ability("Sneak Attack", "enemy", 4)],
    "Mage": [Ability("Firebolt", "enemy", 4), Ability("Barrier", "ally", 3)],
    "Healer": [Ability("Smite", "enemy", 3), Ability("Heal", "ally", 4)],
    "Beast": [Ability("Bite", "enemy", 2), Ability("Encourage", "ally", 2)],
    "Demon": [Ability("Claw", "enemy", 2)],
}

CLASS_ATTRIBUTES: Dict[str, Attributes] = {
    "Fighter": Attributes(8, 4, 3, 12),
    "Rogue": Attributes(5, 8, 3, 10),
    "Mage": Attributes(3, 5, 8, 8),
    "Healer": Attributes(4, 4, 8, 10),
    "Beast": Attributes(6, 6, 2, 8),
    "Demon": Attributes(5, 5, 5, 10),
}

# Which attribute each class uses when attacking or defending. This
# lets combat compare the attacker's focus stat with the defender's
# appropriate stat.
ATTACK_ATTRIBUTE: Dict[str, str] = {
    "Fighter": "strength",
    "Rogue": "agility",
    "Mage": "wisdom",
    "Healer": "wisdom",
    "Beast": "strength",
    "Demon": "strength",
}

DEFENSE_ATTRIBUTE: Dict[str, str] = {
    "Fighter": "strength",
    "Rogue": "agility",
    "Mage": "wisdom",
    "Healer": "wisdom",
    "Beast": "agility",
    "Demon": "strength",
}


# Stick figure drawing helpers

def draw_stick_figure(screen: pygame.Surface, x: int, y: int, color: pygame.Color) -> None:
    """Draw a simple stick figure centered at (x, y)."""
    pygame.draw.circle(screen, color, (x, y - 8), 8, 2)
    pygame.draw.line(screen, color, (x, y), (x, y + 16), 2)
    pygame.draw.line(screen, color, (x, y + 4), (x - 8, y + 12), 2)
    pygame.draw.line(screen, color, (x, y + 4), (x + 8, y + 12), 2)
    pygame.draw.line(screen, color, (x, y + 16), (x - 8, y + 24), 2)
    pygame.draw.line(screen, color, (x, y + 16), (x + 8, y + 24), 2)


def draw_warrior(screen: pygame.Surface, x: int, y: int) -> None:
    """Stick figure with a small sword."""
    draw_stick_figure(screen, x, y, pygame.Color("red"))
    pygame.draw.line(screen, pygame.Color("silver"), (x + 6, y + 12), (x + 16, y + 2), 2)


def draw_cleric(screen: pygame.Surface, x: int, y: int) -> None:
    """Stick figure with a holy symbol."""
    draw_stick_figure(screen, x, y, pygame.Color("yellow"))
    pygame.draw.line(screen, pygame.Color("white"), (x, y + 5), (x, y - 10), 2)
    pygame.draw.line(screen, pygame.Color("white"), (x - 4, y - 2), (x + 4, y - 2), 2)


def draw_familiar(screen: pygame.Surface, x: int, y: int) -> None:
    """Simple blob for the familiar."""
    pygame.draw.circle(screen, pygame.Color("cyan"), (x, y), 6)


def draw_chest(screen: pygame.Surface, rect: pygame.Rect, opened: bool) -> None:
    pygame.draw.rect(screen, pygame.Color("sienna"), rect, 2)
    if opened:
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topleft, rect.bottomright, 2)
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topright, rect.bottomleft, 2)


def draw_door(screen: pygame.Surface, rect: pygame.Rect) -> None:
    pygame.draw.rect(screen, pygame.Color("gray"), rect, 2)
    pygame.draw.line(screen, pygame.Color("gray"), (rect.centerx, rect.top), (rect.centerx, rect.bottom), 2)


def draw_room_bounds(screen: pygame.Surface, shape: str) -> None:
    color = pygame.Color("darkblue")
    if shape == "circle":
        pygame.draw.ellipse(screen, color, pygame.Rect(100, 80, 440, 320), 2)
    elif shape == "wide":
        pygame.draw.rect(screen, color, pygame.Rect(40, 200, 560, 80), 2)
    elif shape == "tall":
        pygame.draw.rect(screen, color, pygame.Rect(260, 40, 120, 400), 2)
    elif shape == "control":
        pygame.draw.polygon(
            screen,
            color,
            [(320, 60), (380, 120), (380, 360), (320, 420), (260, 360), (260, 120)],
            2,
        )
    else:
        pygame.draw.rect(screen, color, pygame.Rect(120, 100, 400, 280), 2)


def draw_gradient_rect(surface: pygame.Surface, rect: pygame.Rect, top: pygame.Color, bottom: pygame.Color) -> None:
    """Fill rect with a vertical gradient."""
    for y in range(rect.height):
        ratio = y / rect.height
        r = int(top.r + (bottom.r - top.r) * ratio)
        g = int(top.g + (bottom.g - top.g) * ratio)
        b = int(top.b + (bottom.b - top.b) * ratio)
        pygame.draw.line(surface, (r, g, b), (rect.x, rect.y + y), (rect.x + rect.width, rect.y + y))


def get_face_surface(npc: NPC) -> pygame.Surface:
    surf = pygame.Surface((32, 32), pygame.SRCALPHA)
    if npc.name == "Warrior":
        draw_warrior(surf, 16, 24)
    elif npc.name == "Cleric":
        draw_cleric(surf, 16, 24)
    elif npc.name == "Familiar":
        draw_familiar(surf, 16, 16)
    else:
        draw_stick_figure(surf, 16, 24, npc.color)
    return surf


def draw_text_box(
    screen: pygame.Surface,
    font: pygame.font.Font,
    lines: List[str],
    speaker: Optional[str] = None,
    face: Optional[pygame.Surface] = None,
) -> None:
    width, height = screen.get_size()
    box_height = height // 3
    rect = pygame.Rect(20, height - box_height - 20, width - 40, box_height)
    draw_gradient_rect(screen, rect, pygame.Color(100, 100, 255), pygame.Color(40, 40, 180))
    pygame.draw.rect(screen, pygame.Color("silver"), rect, 4)

    y = rect.y + 10
    if speaker:
        name_text = font.render(speaker, True, pygame.Color("white"))
        screen.blit(name_text, (rect.x + 10, y))
        y += 30
    for line in lines:
        text = font.render(line, True, pygame.Color("white"))
        screen.blit(text, (rect.x + 10, y))
        y += 26
    if face:
        face_rect = face.get_rect()
        face_rect.midright = (rect.x - 10, rect.y + 40)
        screen.blit(face, face_rect)


@dataclass
class NPC:
    name: str
    color: pygame.Color
    x: int
    y: int
    dialog: Optional[callable] = None
    joined: bool = False
    enemy: bool = False
    char_class: str = ""
    abilities: List[Ability] = field(default_factory=list)
    attributes: Attributes = field(default_factory=lambda: Attributes(5, 5, 5, 10))

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 16, self.y - 16, 32, 32)

    def draw(self, screen: pygame.Surface) -> None:
        if self.name == "Warrior":
            draw_warrior(screen, self.x, self.y)
        elif self.name == "Cleric":
            draw_cleric(screen, self.x, self.y)
        elif self.name == "Familiar":
            draw_familiar(screen, self.x, self.y)
        else:
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
        draw_text_box(screen, font, lines + ["Press SPACE to continue"])
        pygame.display.flip()
        clock.tick(30)


def menu_prompt(
    screen: pygame.Surface,
    font: pygame.font.Font,
    question: str,
    options: List[str],
    speaker: Optional[str] = None,
    face: Optional[pygame.Surface] = None,
) -> int:
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
        lines = [question] + [f"{i + 1}. {opt}" for i, opt in enumerate(options)]
        draw_text_box(screen, font, lines, speaker, face)
        pygame.display.flip()
        clock.tick(30)
    return choice


def text_input(screen: pygame.Surface, font: pygame.font.Font, prompt: str) -> str:
    """Simple text entry field."""
    clock = pygame.time.Clock()
    text = ""
    done = False
    while not done:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_RETURN:
                    done = True
                elif event.key == pygame.K_BACKSPACE:
                    text = text[:-1]
                else:
                    text += event.unicode
        screen.fill((0, 0, 0))
        draw_text_box(screen, font, [prompt, text])
        pygame.display.flip()
        clock.tick(30)
    return text


def familiar_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        face = get_face_surface(NPC("Familiar", pygame.Color("cyan"), 0, 0))
        idx = menu_prompt(
            screen,
            font,
            "The familiar chirps softly.",
            ["\"Who are you?\"", "\"Will you help me escape?\"", "\"Let's go.\""],
            "Familiar",
            face,
        )
        if idx == 0:
            show_message(screen, font, ["It chitters about being bound to the ship by foul magic."])
        elif idx == 1:
            show_message(screen, font, ["The creature nods enthusiastically."])
        elif idx == 2:
            show_message(screen, font, ["It hops onto your shoulder."])
            return


def warrior_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        face = get_face_surface(NPC("Warrior", pygame.Color("red"), 0, 0))
        idx = menu_prompt(
            screen,
            font,
            "The warrior wipes ichor from his blade.",
            ["\"What's your name?\"", "\"Stick with me, we can escape.\"", "\"Enough talk.\""],
            "Warrior",
            face,
        )
        if idx == 0:
            show_message(screen, font, ["\"Call me whatever you like. Let's just survive,\" he grunts."])
        elif idx == 1:
            show_message(screen, font, ["He nods. 'Lead the way.'"])
        elif idx == 2:
            show_message(screen, font, ["He falls in behind you, ready for battle."])
            return


def cleric_dialog(screen: pygame.Surface, font: pygame.font.Font) -> None:
    while True:
        face = get_face_surface(NPC("Cleric", pygame.Color("yellow"), 0, 0))
        idx = menu_prompt(
            screen,
            font,
            "The cleric steadies her breath.",
            ["\"What happened here?\"", "\"Can you heal us?\"", "\"Let's leave this ship.\""],
            "Cleric",
            face,
        )
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
    name: str = ""
    char_class: str = ""
    companions: List[NPC] = field(default_factory=list)
    inventory: List[str] = field(default_factory=list)
    flags: Dict[str, bool] = field(default_factory=dict)
    abilities: List[Ability] = field(default_factory=list)
    attributes: Attributes = field(default_factory=lambda: Attributes(5, 5, 5, 10))

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 16, self.y - 16, 32, 32)

    def draw(self, screen: pygame.Surface) -> None:
        draw_stick_figure(screen, self.x, self.y, pygame.Color("white"))


def update_companions(player: Player) -> None:
    """Move companions toward the entity ahead of them."""
    leader_x, leader_y = player.x, player.y
    for comp in player.companions:
        dx = leader_x - comp.x
        dy = leader_y - comp.y
        dist = (dx * dx + dy * dy) ** 0.5
        if dist > 1:
            comp.x += int(0.2 * dx)
            comp.y += int(0.2 * dy)
        leader_x, leader_y = comp.x, comp.y


def float_number(
    screen: pygame.Surface,
    font: pygame.font.Font,
    lines: List[str],
    text: str,
    color: pygame.Color,
    pos: tuple[int, int],
) -> None:
    """Show text box with lines and a floating number at pos."""
    clock = pygame.time.Clock()
    for i in range(30):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        screen.fill((0, 0, 0))
        draw_text_box(screen, font, lines)
        rise_y = pos[1] - i
        num_surf = font.render(text, True, color)
        num_rect = num_surf.get_rect(center=(pos[0], rise_y))
        screen.blit(num_surf, num_rect)
        pygame.display.flip()
        clock.tick(60)


def combat(screen: pygame.Surface, font: pygame.font.Font, player: Player, enemy: NPC) -> bool:
    """Simple turn-based combat. Returns True if player wins."""
    p_hp = player.attributes.hp
    e_hp = enemy.attributes.hp
    clock = pygame.time.Clock()
    turn_player = True
    while p_hp > 0 and e_hp > 0:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()

        screen.fill((0, 0, 0))
        info = [f"{player.name}: {p_hp} HP", f"{enemy.name}: {e_hp} HP"]
        draw_text_box(screen, font, info)
        pygame.display.flip()
        clock.tick(30)

        if turn_player:
            idx = menu_prompt(
                screen,
                font,
                "Choose action",
                [ab.name for ab in player.abilities],
                player.name,
                get_face_surface(NPC(player.name, pygame.Color("white"), 0, 0)),
            )
            ability = player.abilities[idx]
            atk_attr = ATTACK_ATTRIBUTE.get(player.char_class, "strength")
            def_attr = DEFENSE_ATTRIBUTE.get(enemy.char_class, "strength")
            if ability.target == "enemy":
                attack_val = ability.power + getattr(player.attributes, atk_attr)
                defense_val = getattr(enemy.attributes, def_attr)
                dmg = max(1, attack_val - defense_val // 2)
                critical = False
                if random.random() < 0.1:
                    dmg *= 2
                    critical = True
                e_hp -= dmg
                show_message(screen, font, [f"{player.name} uses {ability.name}!"])
                color = pygame.Color("red") if critical else pygame.Color("white")
                float_number(
                    screen,
                    font,
                    [f"{player.name}: {p_hp} HP", f"{enemy.name}: {e_hp} HP"],
                    f"-{dmg}",
                    color,
                    (460, 220),
                )
            else:
                heal = ability.power + getattr(player.attributes, atk_attr) // 2
                p_hp = min(player.attributes.hp, p_hp + heal)
                show_message(screen, font, [f"{player.name} uses {ability.name}!"])
                float_number(
                    screen,
                    font,
                    [f"{player.name}: {p_hp} HP", f"{enemy.name}: {e_hp} HP"],
                    f"+{heal}",
                    pygame.Color("green"),
                    (180, 220),
                )
        else:
            ability = enemy.abilities[0] if enemy.abilities else Ability("Attack", "enemy", 2)
            atk_attr = ATTACK_ATTRIBUTE.get(enemy.char_class, "strength")
            def_attr = DEFENSE_ATTRIBUTE.get(player.char_class, "strength")
            attack_val = ability.power + getattr(enemy.attributes, atk_attr)
            defense_val = getattr(player.attributes, def_attr)
            dmg = max(1, attack_val - defense_val // 2)
            critical = False
            if random.random() < 0.1:
                dmg *= 2
                critical = True
            p_hp -= dmg
            show_message(screen, font, [f"{enemy.name} attacks!"])
            color = pygame.Color("red") if critical else pygame.Color("white")
            float_number(
                screen,
                font,
                [f"{player.name}: {p_hp} HP", f"{enemy.name}: {e_hp} HP"],
                f"-{dmg}",
                color,
                (180, 220),
            )
        turn_player = not turn_player

    if p_hp > 0:
        show_message(screen, font, ["You are victorious!"])
        return True
    else:
        show_message(screen, font, ["You were defeated..."])
        return False


@dataclass
class Room:
    name: str
    npcs: List[NPC]
    chests: List[Chest]
    doors: List[Door]
    shape: str = "square"


def create_rooms() -> Dict[str, Room]:
    rooms: Dict[str, Room] = {}

    rooms["Pod Room"] = Room(
        "Pod Room",
        [
            NPC(
                "Familiar",
                pygame.Color("cyan"),
                200,
                240,
                familiar_dialog,
                False,
                False,
                "Beast",
                CLASS_ABILITIES["Beast"].copy(),
                CLASS_ATTRIBUTES["Beast"],
            )
        ],
        [],
        [Door(pygame.Rect(600, 220, 40, 40), "Corridor")],
        "circle",
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
        "wide",
    )

    rooms["Brig"] = Room(
        "Brig",
        [
            NPC(
                "Warrior",
                pygame.Color("red"),
                320,
                240,
                warrior_dialog,
                False,
                False,
                "Fighter",
                CLASS_ABILITIES["Fighter"].copy(),
                CLASS_ATTRIBUTES["Fighter"],
            ),
            NPC(
                "Imp",
                pygame.Color("red"),
                380,
                220,
                None,
                False,
                True,
                "Demon",
                CLASS_ABILITIES["Demon"].copy(),
                CLASS_ATTRIBUTES["Demon"],
            ),
        ],
        [],
        [Door(pygame.Rect(300, 380, 40, 40), "Corridor")],
        "square",
    )

    rooms["Storage"] = Room(
        "Storage",
        [],
        [Chest(pygame.Rect(320, 240, 32, 24), "a healing salve", "storage_chest_opened")],
        [Door(pygame.Rect(300, 60, 40, 40), "Corridor")],
        "tall",
    )

    rooms["Control Room"] = Room(
        "Control Room",
        [
            NPC(
                "Cleric",
                pygame.Color("yellow"),
                320,
                240,
                cleric_dialog,
                False,
                False,
                "Healer",
                CLASS_ABILITIES["Healer"].copy(),
                CLASS_ATTRIBUTES["Healer"],
            )
        ],
        [],
        [
            Door(pygame.Rect(40, 220, 40, 40), "Corridor"),
            Door(pygame.Rect(600, 220, 40, 40), "Escape Pod"),
        ],
        "control",
    )

    rooms["Escape Pod"] = Room(
        "Escape Pod",
        [],
        [],
        [],
        "square",
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

    name = text_input(screen, font, "Enter your name:")
    class_idx = menu_prompt(screen, font, "Choose a class", ["Fighter", "Rogue", "Mage"])
    char_class = ["Fighter", "Rogue", "Mage"][class_idx]
    player = Player(
        320,
        240,
        name=name,
        char_class=char_class,
        abilities=CLASS_ABILITIES[char_class].copy(),
        attributes=CLASS_ATTRIBUTES[char_class],
    )

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
                        dest_room = rooms[door.dest]
                        if door.dest == "Escape Pod":
                            current = dest_room
                            game_end(screen, font, player)
                            running = False
                        else:
                            back = next((d for d in dest_room.doors if d.dest == current.name), None)
                            if back:
                                player.x, player.y = back.rect.center
                            else:
                                player.x, player.y = 320, 240
                            current = dest_room
                        break
                # chests
                for chest in current.chests:
                    if not chest.opened and pr.colliderect(chest.rect):
                        chest.opened = True
                        player.inventory.append(chest.item)
                        player.flags[chest.flag] = True
                        show_message(screen, font, [f"You find {chest.item}!"])
                        break
                # npcs and enemies
                for npc in current.npcs:
                    if pr.colliderect(npc.rect()):
                        if npc.enemy and not npc.joined:
                            if combat(screen, font, player, npc):
                                npc.joined = True  # mark as defeated
                            else:
                                running = False
                            break
                        elif not npc.joined:
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
        update_companions(player)

        screen.fill((0, 0, 0))
        draw_room_bounds(screen, current.shape)
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
