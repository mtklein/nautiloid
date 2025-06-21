from __future__ import annotations
# NOTE: This file is retained while features are ported to nautiloid.c.
# It no longer receives updates.
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
    melee: bool = False


@dataclass
class Attributes:
    strength: int
    agility: int
    wisdom: int
    hp: int


CLASS_ABILITIES: Dict[str, List[Ability]] = {
    "Fighter": [
        Ability("Strike", "enemy", 3, True),
        Ability("Power Attack", "enemy", 5, True),
    ],
    "Rogue": [
        Ability("Stab", "enemy", 3, True),
        Ability("Sneak Attack", "enemy", 4, True),
    ],
    "Mage": [Ability("Firebolt", "enemy", 4), Ability("Barrier", "ally", 3)],
    "Healer": [Ability("Smite", "enemy", 3, True), Ability("Heal", "ally", 4)],
    "Beast": [Ability("Bite", "enemy", 2, True), Ability("Encourage", "ally", 2)],
    "Demon": [Ability("Claw", "enemy", 2, True)],
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


# Sprite drawing helpers

def draw_humanoid(screen: pygame.Surface, x: int, y: int, color: pygame.Color) -> None:
    """Draw a 16x48 style humanoid sprite with (x, y) at the feet."""
    # head
    pygame.draw.rect(screen, color, pygame.Rect(x - 5, y - 48, 10, 10))
    pygame.draw.rect(screen, pygame.Color("white"), pygame.Rect(x - 3, y - 46, 2, 2))
    pygame.draw.rect(screen, pygame.Color("white"), pygame.Rect(x + 1, y - 46, 2, 2))
    pygame.draw.rect(screen, pygame.Color("black"), pygame.Rect(x - 2, y - 42, 4, 1))
    # torso
    pygame.draw.rect(screen, color, pygame.Rect(x - 4, y - 38, 8, 20))
    # arms
    pygame.draw.rect(screen, color, pygame.Rect(x - 8, y - 38, 3, 15))
    pygame.draw.rect(screen, color, pygame.Rect(x + 5, y - 38, 3, 15))
    # legs
    pygame.draw.rect(screen, color, pygame.Rect(x - 4, y - 18, 3, 18))
    pygame.draw.rect(screen, color, pygame.Rect(x + 1, y - 18, 3, 18))


def draw_warrior(screen: pygame.Surface, x: int, y: int) -> None:
    """Humanoid sprite with a small sword."""
    draw_humanoid(screen, x, y, pygame.Color("firebrick"))
    pygame.draw.rect(screen, pygame.Color("sienna"), pygame.Rect(x - 5, y - 52, 10, 3))
    pygame.draw.line(screen, pygame.Color("silver"), (x + 6, y - 20), (x + 10, y - 36), 2)


def draw_cleric(screen: pygame.Surface, x: int, y: int) -> None:
    """Humanoid sprite with a holy symbol."""
    draw_humanoid(screen, x, y, pygame.Color("skyblue"))
    pygame.draw.rect(screen, pygame.Color("khaki"), pygame.Rect(x - 5, y - 52, 10, 2))
    pygame.draw.line(screen, pygame.Color("white"), (x, y - 28), (x, y - 44), 2)
    pygame.draw.line(screen, pygame.Color("white"), (x - 4, y - 36), (x + 4, y - 36), 2)


def draw_familiar(screen: pygame.Surface, x: int, y: int) -> None:
    """Small blob-like creature."""
    pygame.draw.circle(screen, pygame.Color("cyan"), (x, y - 8), 8)

def draw_imp(screen: pygame.Surface, x: int, y: int) -> None:
    """Small imp enemy."""
    pygame.draw.rect(screen, pygame.Color("red"), pygame.Rect(x - 6, y - 16, 12, 16))
    pygame.draw.line(screen, pygame.Color("black"), (x - 4, y - 16), (x - 2, y - 20), 2)
    pygame.draw.line(screen, pygame.Color("black"), (x + 4, y - 16), (x + 2, y - 20), 2)


def draw_rogue(screen: pygame.Surface, x: int, y: int) -> None:
    """Humanoid sprite with a dagger and hood."""
    draw_humanoid(screen, x, y, pygame.Color("olivedrab"))
    pygame.draw.rect(
        screen, pygame.Color("darkolivegreen"), pygame.Rect(x - 6, y - 48, 12, 8)
    )
    pygame.draw.rect(screen, pygame.Color("black"), pygame.Rect(x - 5, y - 46, 10, 3))
    pygame.draw.rect(screen, pygame.Color("white"), pygame.Rect(x - 3, y - 46, 2, 2))
    pygame.draw.rect(screen, pygame.Color("white"), pygame.Rect(x + 1, y - 46, 2, 2))
    pygame.draw.line(screen, pygame.Color("silver"), (x + 6, y - 20), (x + 10, y - 30), 2)


def draw_mage(screen: pygame.Surface, x: int, y: int) -> None:
    """Humanoid sprite with a staff."""
    draw_humanoid(screen, x, y, pygame.Color("slateblue"))
    pygame.draw.polygon(
        screen,
        pygame.Color("purple"),
        [(x - 6, y - 48), (x + 6, y - 48), (x, y - 60)],
    )
    pygame.draw.line(screen, pygame.Color("sienna"), (x + 6, y - 20), (x + 6, y - 40), 2)
    pygame.draw.circle(screen, pygame.Color("sienna"), (x + 6, y - 42), 3)


def draw_chest(screen: pygame.Surface, rect: pygame.Rect, opened: bool) -> None:
    pygame.draw.rect(screen, pygame.Color("sienna"), rect, 2)
    if opened:
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topleft, rect.bottomright, 2)
        pygame.draw.line(screen, pygame.Color("sienna"), rect.topright, rect.bottomleft, 2)


def draw_door(screen: pygame.Surface, rect: pygame.Rect) -> None:
    pygame.draw.rect(screen, pygame.Color("gray"), rect, 2)
    pygame.draw.line(screen, pygame.Color("gray"), (rect.centerx, rect.top), (rect.centerx, rect.bottom), 2)


def draw_prop(screen: pygame.Surface, rect: pygame.Rect) -> None:
    """Simple decorative prop."""
    pygame.draw.rect(screen, pygame.Color("olive"), rect, 1)


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
    surf = pygame.Surface((32, 48), pygame.SRCALPHA)
    if npc.name == "Familiar":
        draw_familiar(surf, 16, 24)
    elif npc.name == "Imp":
        draw_imp(surf, 16, 24)
    elif npc.char_class == "Fighter":
        draw_warrior(surf, 16, 48)
    elif npc.char_class == "Healer":
        draw_cleric(surf, 16, 48)
    elif npc.char_class == "Rogue":
        draw_rogue(surf, 16, 48)
    elif npc.char_class == "Mage":
        draw_mage(surf, 16, 48)
    else:
        draw_humanoid(surf, 16, 48, npc.color)
    return surf


def draw_text_box(
    screen: pygame.Surface,
    font: pygame.font.Font,
    lines: List[str],
    speaker: Optional[str] = None,
    face: Optional[pygame.Surface] = None,
    bottom: Optional[str] = None,
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
    if bottom:
        text = font.render(bottom, True, pygame.Color("white"))
        text_rect = text.get_rect(centerx=rect.centerx)
        text_rect.bottom = rect.bottom - 10
        screen.blit(text, text_rect)


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

    def __hash__(self) -> int:
        return id(self)

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 8, self.y - 48, 16, 48)

    def draw(self, screen: pygame.Surface, pos: Optional[tuple[int, int]] = None) -> None:
        x, y = pos if pos else (self.x, self.y)
        if self.name == "Familiar":
            draw_familiar(screen, x, y)
        elif self.name == "Imp":
            draw_imp(screen, x, y)
        elif self.char_class == "Fighter":
            draw_warrior(screen, x, y)
        elif self.char_class == "Healer":
            draw_cleric(screen, x, y)
        elif self.char_class == "Rogue":
            draw_rogue(screen, x, y)
        elif self.char_class == "Mage":
            draw_mage(screen, x, y)
        else:
            draw_humanoid(screen, x, y, self.color)


def show_message(
    screen: pygame.Surface,
    font: pygame.font.Font,
    lines: List[str],
    draw_bg: Optional[callable] = None,
) -> None:
    """Display message lines until SPACE or E pressed."""
    clock = pygame.time.Clock()
    waiting = True
    while waiting:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
            if (
                event.type == pygame.KEYDOWN
                and event.key in (pygame.K_SPACE, pygame.K_e)
            ):
                waiting = False
        screen.fill((0, 0, 0))
        if draw_bg:
            draw_bg()
        draw_text_box(
            screen,
            font,
            lines,
            bottom="Press SPACE or E to continue",
        )
        pygame.display.flip()
        clock.tick(30)


def show_inventory(
    screen: pygame.Surface,
    font: pygame.font.Font,
    player: Player,
) -> None:
    """Display the player's inventory."""
    if player.inventory:
        lines = ["Inventory:"] + [f" - {item}" for item in player.inventory]
    else:
        lines = ["Your inventory is empty."]
    show_message(screen, font, lines)


def star_wars_scroll(screen: pygame.Surface, font: pygame.font.Font, lines: List[str]) -> None:
    """Scroll lines upward with simple fireworks."""
    clock = pygame.time.Clock()
    surfaces = [font.render(line, True, pygame.Color("yellow")) for line in lines]
    total_h = sum(s.get_height() + 8 for s in surfaces)
    offset = screen.get_height()
    fireworks: List[dict] = []
    hold = 0
    while offset + total_h > 0 or hold < 60:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        screen.fill((0, 0, 0))
        if random.random() < 0.05:
            fireworks.append(
                {
                    "x": random.randint(50, screen.get_width() - 50),
                    "y": screen.get_height(),
                    "vy": random.uniform(2.0, 4.0),
                    "color": pygame.Color(
                        random.randint(128, 255),
                        random.randint(128, 255),
                        random.randint(128, 255),
                    ),
                    "life": 0,
                }
            )
        for fw in fireworks[:]:
            fw["y"] -= fw["vy"]
            fw["life"] += 1
            pygame.draw.circle(screen, fw["color"], (int(fw["x"]), int(fw["y"])), 2)
            if fw["life"] > 60:
                fireworks.remove(fw)
        y = offset
        for surf in surfaces:
            rect = surf.get_rect(centerx=screen.get_width() // 2)
            rect.y = y
            screen.blit(surf, rect)
            y += surf.get_height() + 8
        pygame.display.flip()
        clock.tick(60)
        if offset + total_h > 0:
            offset -= 1
        else:
            hold += 1


def interaction_hint(player: Player, room: Room) -> Optional[str]:
    """Return an interaction hint if the player can interact."""
    pr = player.rect()
    for chest in room.chests:
        if not chest.opened and pr.colliderect(chest.rect):
            return "op(e)n chest"
    for door in room.doors:
        if pr.colliderect(door.rect):
            return "op(e)n door"
    for prop in room.props:
        if pr.colliderect(prop.rect):
            return "insp(e)ct"
    for npc in room.npcs:
        if pr.colliderect(npc.rect()):
            if npc.enemy and not npc.joined:
                return "engag(e)"
            if not npc.joined:
                return "sp(e)ak"
    return None


def draw_instructions(
    screen: pygame.Surface,
    font: pygame.font.Font,
    lines: List[str],
) -> None:
    """Render instruction lines in the bottom left corner."""
    height = screen.get_height()
    base_y = height - 40
    for idx, text in enumerate(lines):
        surf = font.render(text, True, pygame.Color("white"))
        screen.blit(surf, (10, base_y + idx * 20))


def menu_prompt(
    screen: pygame.Surface,
    font: pygame.font.Font,
    question: str,
    options: List[str],
    speaker: Optional[str] = None,
    face: Optional[pygame.Surface] = None,
    draw_bg: Optional[callable] = None,
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
        if draw_bg:
            draw_bg()
        lines = [question] + [f"{i + 1}. {opt}" for i, opt in enumerate(options)]
        draw_text_box(screen, font, lines, speaker, face)
        pygame.display.flip()
        clock.tick(30)
    return choice


def text_input(
    screen: pygame.Surface,
    font: pygame.font.Font,
    prompt: str,
    draw_bg: Optional[callable] = None,
) -> str:
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
        if draw_bg:
            draw_bg()
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
class Prop:
    rect: pygame.Rect
    desc: str

    def draw(self, screen: pygame.Surface) -> None:
        draw_prop(screen, self.rect)


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

    def __hash__(self) -> int:
        return id(self)

    def rect(self) -> pygame.Rect:
        return pygame.Rect(self.x - 8, self.y - 48, 16, 48)

    def draw(self, screen: pygame.Surface, pos: Optional[tuple[int, int]] = None) -> None:
        x, y = pos if pos else (self.x, self.y)
        if self.char_class == "Fighter":
            draw_warrior(screen, x, y)
        elif self.char_class == "Rogue":
            draw_rogue(screen, x, y)
        elif self.char_class == "Mage":
            draw_mage(screen, x, y)
        else:
            draw_humanoid(screen, x, y, pygame.Color("white"))


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
    draw_bg: Optional[callable] = None,
) -> None:
    """Show text box with lines and a floating number at pos."""
    clock = pygame.time.Clock()
    for i in range(30):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                pygame.quit()
                sys.exit()
        screen.fill((0, 0, 0))
        if draw_bg:
            draw_bg()
        draw_text_box(screen, font, lines)
        rise_y = pos[1] - i
        num_surf = font.render(text, True, color)
        num_rect = num_surf.get_rect(center=(pos[0], rise_y))
        screen.blit(num_surf, num_rect)
        pygame.display.flip()
        clock.tick(60)


def draw_health_bar(screen: pygame.Surface, pos: tuple[int, int], hp: int, max_hp: int) -> None:
    """Render a simple health bar."""
    ratio = hp / max_hp
    color = pygame.Color("green")
    if ratio < 0.25:
        color = pygame.Color("red")
    elif ratio < 0.5:
        color = pygame.Color("yellow")
    bar_rect = pygame.Rect(pos[0] - 20, pos[1] - 52, 40, 5)
    pygame.draw.rect(screen, pygame.Color("gray"), bar_rect)
    fill = pygame.Rect(bar_rect.x, bar_rect.y, int(40 * ratio), 5)
    pygame.draw.rect(screen, color, fill)


def combat_encounter(screen: pygame.Surface, font: pygame.font.Font, player: Player, enemies: List[NPC]) -> bool:
    """Group combat encounter. Returns True if player wins."""
    allies: List[NPC | Player] = [player] + player.companions
    combatants: List[NPC | Player] = allies + enemies
    hp: Dict[NPC | Player, int] = {c: c.attributes.hp for c in combatants}

    positions: Dict[NPC | Player, tuple[int, int]] = {}
    for i, ally in enumerate(allies):
        positions[ally] = (100, 300 - i * 60)
    for i, enemy in enumerate(enemies):
        positions[enemy] = (500, 300 - i * 60)

    clock = pygame.time.Clock()

    def draw_fight() -> None:
        screen.fill((0, 0, 0))
        for combatant in combatants:
            if hp[combatant] > 0:
                x, y = positions[combatant]
                combatant.draw(screen, (x, y))
                draw_health_bar(screen, (x, y), hp[combatant], combatant.attributes.hp)

    while hp[player] > 0 and any(hp[e] > 0 for e in enemies):
        order = sorted(combatants, key=lambda c: (c.attributes.agility + random.randint(0, 2)), reverse=True)

        for actor in order:
            if hp[player] <= 0 or not any(hp[e] > 0 for e in enemies):
                break
            if hp[actor] <= 0:
                continue

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    pygame.quit()
                    sys.exit()

            draw_fight()
            pygame.display.flip()
            clock.tick(30)

            if actor is player:
                idx = menu_prompt(
                    screen,
                    font,
                    "Choose action",
                    [ab.name for ab in player.abilities],
                    player.name,
                    get_face_surface(player),
                    draw_bg=draw_fight,
                )
                ability = player.abilities[idx]
                targets = enemies if ability.target == "enemy" else allies
                living = [t for t in targets if hp[t] > 0]
                tnames = [t.name for t in living]
                tidx = 0 if len(living) == 1 else menu_prompt(screen, font, "Choose target", tnames, draw_bg=draw_fight)
                target = living[tidx]

                if ability.melee:
                    dx = positions[target][0] - positions[actor][0]
                    if abs(dx) > 120:
                        show_message(screen, font, ["Too far away!"], draw_bg=draw_fight)
                        continue
                atk_attr = ATTACK_ATTRIBUTE.get(actor.char_class, "strength")
                def_attr = DEFENSE_ATTRIBUTE.get(target.char_class, "strength")
                attack_val = ability.power + getattr(actor.attributes, atk_attr)
                defense_val = getattr(target.attributes, def_attr)
                dmg = max(1, attack_val - defense_val // 2)
                if random.random() < 0.1:
                    dmg *= 2
                if ability.target == "enemy":
                    hp[target] -= dmg
                    float_number(screen, font, [f"{actor.name} uses {ability.name}!"], f"-{dmg}", pygame.Color("white"), positions[target], draw_bg=draw_fight)
                else:
                    hp[target] = min(target.attributes.hp, hp[target] + dmg)
                    float_number(screen, font, [f"{actor.name} uses {ability.name}!"], f"+{dmg}", pygame.Color("green"), positions[target], draw_bg=draw_fight)
            else:
                if actor in enemies:
                    ability = actor.abilities[0] if actor.abilities else Ability("Attack", "enemy", 2, True)
                    target = player
                    atk_attr = ATTACK_ATTRIBUTE.get(actor.char_class, "strength")
                    def_attr = DEFENSE_ATTRIBUTE.get(target.char_class, "strength")
                    attack_val = ability.power + getattr(actor.attributes, atk_attr)
                    defense_val = getattr(target.attributes, def_attr)
                    dmg = max(1, attack_val - defense_val // 2)
                    hp[target] -= dmg
                    float_number(screen, font, [f"{actor.name} attacks!"], f"-{dmg}", pygame.Color("red"), positions[target], draw_bg=draw_fight)
                else:  # companion
                    ability = actor.abilities[0] if actor.abilities else Ability("Strike", "enemy", 2, True)
                    target = next((e for e in enemies if hp[e] > 0), None)
                    if not target:
                        continue
                    atk_attr = ATTACK_ATTRIBUTE.get(actor.char_class, "strength")
                    def_attr = DEFENSE_ATTRIBUTE.get(target.char_class, "strength")
                    attack_val = ability.power + getattr(actor.attributes, atk_attr)
                    defense_val = getattr(target.attributes, def_attr)
                    dmg = max(1, attack_val - defense_val // 2)
                    hp[target] -= dmg
                    float_number(screen, font, [f"{actor.name} attacks!"], f"-{dmg}", pygame.Color("white"), positions[target], draw_bg=draw_fight)

    if hp[player] > 0:
        show_message(screen, font, ["You are victorious!"], draw_bg=draw_fight)
        return True
    else:
        show_message(screen, font, ["You were defeated..."], draw_bg=draw_fight)
        return False


@dataclass
class Room:
    name: str
    npcs: List[NPC]
    chests: List[Chest]
    props: List[Prop]
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
        [Prop(pygame.Rect(260, 260, 20, 20), "A broken glass pod")],
        [Door(pygame.Rect(600, 220, 40, 40), "Corridor")],
        "circle",
    )

    rooms["Corridor"] = Room(
        "Corridor",
        [],
        [],
        [Prop(pygame.Rect(320, 240, 16, 16), "A flickering wall torch")],
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
            NPC(
                "Imp",
                pygame.Color("red"),
                300,
                200,
                None,
                False,
                True,
                "Demon",
                CLASS_ABILITIES["Demon"].copy(),
                CLASS_ATTRIBUTES["Demon"],
            ),
            NPC(
                "Imp",
                pygame.Color("red"),
                340,
                260,
                None,
                False,
                True,
                "Demon",
                CLASS_ABILITIES["Demon"].copy(),
                CLASS_ATTRIBUTES["Demon"],
            ),
        ],
        [Chest(pygame.Rect(280, 240, 32, 24), "an iron sword", "brig_sword")],
        [Prop(pygame.Rect(360, 260, 16, 16), "Chains hang from the wall")],
        [Door(pygame.Rect(300, 380, 40, 40), "Corridor")],
        "square",
    )

    rooms["Storage"] = Room(
        "Storage",
        [],
        [
            Chest(
                pygame.Rect(320, 240, 32, 24),
                "a healing salve",
                "storage_chest_opened",
            ),
            Chest(
                pygame.Rect(360, 240, 32, 24),
                "leather armor",
                "storage_armor",
            ),
        ],
        [Prop(pygame.Rect(300, 300, 20, 20), "Crates of supplies")],
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
            ),
            NPC(
                "Cultist",
                pygame.Color("purple"),
                380,
                200,
                None,
                False,
                True,
                "Mage",
                CLASS_ABILITIES["Mage"].copy(),
                CLASS_ATTRIBUTES["Mage"],
            ),
            NPC(
                "Acolyte",
                pygame.Color("purple"),
                260,
                200,
                None,
                False,
                True,
                "Rogue",
                CLASS_ABILITIES["Rogue"].copy(),
                CLASS_ATTRIBUTES["Rogue"],
            ),
        ],
        [Chest(pygame.Rect(320, 300, 32, 24), "mystic staff", "control_staff")],
        [Prop(pygame.Rect(320, 180, 20, 20), "A glowing altar")],
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
    star_wars_scroll(screen, font, lines)


def main() -> None:
    pygame.init()
    screen = pygame.display.set_mode((640, 480))
    pygame.display.set_caption("Nautiloid Adventure")
    font = pygame.font.SysFont(None, 28)
    clock = pygame.time.Clock()

    name = text_input(screen, font, "Enter your name:")
    class_idx = menu_prompt(screen, font, "Choose a class", ["Fighter", "Rogue", "Mage", "Healer"])
    char_class = ["Fighter", "Rogue", "Mage", "Healer"][class_idx]
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
                        if chest.item == "an iron sword":
                            player.attributes.strength += 2
                        if chest.item == "leather armor":
                            player.attributes.hp += 2
                        if chest.item == "mystic staff":
                            player.attributes.wisdom += 2
                        show_message(screen, font, [f"You find {chest.item}!"])
                        break
                # props
                for prop in current.props:
                    if pr.colliderect(prop.rect):
                        show_message(screen, font, [prop.desc])
                        break
                # npcs and enemies
                for npc in current.npcs:
                    if pr.colliderect(npc.rect()):
                        if npc.enemy and not npc.joined:
                            encounter = [e for e in current.npcs if e.enemy and not e.joined]
                            if combat_encounter(screen, font, player, encounter):
                                for e in encounter:
                                    e.joined = True
                            else:
                                running = False
                            break
                        elif not npc.joined:
                            if npc.dialog:
                                npc.dialog(screen, font)
                            npc.joined = True
                            player.companions.append(npc)
                            break
            if event.type == pygame.KEYDOWN and event.key == pygame.K_i:
                show_inventory(screen, font, player)

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
        for prop in current.props:
            prop.draw(screen)
        for npc in current.npcs:
            if not npc.joined:
                npc.draw(screen)
        player.draw(screen)
        for idx, comp in enumerate(player.companions):
            comp.draw(screen)
        room_text = font.render(current.name, True, pygame.Color("white"))
        screen.blit(room_text, (10, 10))

        hint = interaction_hint(player, current)
        lines = ["(i)nventory", "(p)arty"]
        if hint:
            lines.append(hint)
        draw_instructions(screen, font, lines)
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
