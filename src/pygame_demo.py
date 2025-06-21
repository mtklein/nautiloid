import sys
from dataclasses import dataclass
from typing import List, Optional

import pygame


@dataclass
class Entity:
    """Simple game entity represented by a large letter."""

    x: int
    y: int
    char: str
    color: pygame.Color

    def draw(self, screen: pygame.Surface, font: pygame.font.Font) -> None:
        text = font.render(self.char, True, self.color)
        rect = text.get_rect(center=(self.x, self.y))
        screen.blit(text, rect)


def collide(a: Entity, b: Entity) -> bool:
    return abs(a.x - b.x) < 32 and abs(a.y - b.y) < 32


def main() -> None:
    """Run a basic interactive demo with letter graphics."""
    pygame.init()
    screen = pygame.display.set_mode((640, 480))
    pygame.display.set_caption("Nautiloid Adventure")
    font = pygame.font.SysFont(None, 72)
    small_font = pygame.font.SysFont(None, 28)
    clock = pygame.time.Clock()

    player = Entity(320, 240, "F", pygame.Color("white"))
    familiar: Optional[Entity] = Entity(120, 360, "f", pygame.Color("cyan"))
    cleric: Optional[Entity] = Entity(520, 120, "C", pygame.Color("yellow"))
    imp: Optional[Entity] = Entity(320, 120, "i", pygame.Color("red"))
    exit_door = Entity(600, 440, "E", pygame.Color("green"))

    companions: List[Entity] = []
    message: Optional[str] = None
    message_timer = 0

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        keys = pygame.key.get_pressed()
        if keys[pygame.K_LEFT]:
            player.x -= 4
        if keys[pygame.K_RIGHT]:
            player.x += 4
        if keys[pygame.K_UP]:
            player.y -= 4
        if keys[pygame.K_DOWN]:
            player.y += 4

        # keep player on screen
        player.x = max(20, min(620, player.x))
        player.y = max(20, min(460, player.y))

        if familiar and collide(player, familiar):
            companions.append(familiar)
            familiar = None
            message = "Familiar joins you!"
            message_timer = 120

        if cleric and collide(player, cleric):
            companions.append(cleric)
            cleric = None
            message = "Cleric joins you!"
            message_timer = 120

        if imp and collide(player, imp):
            imp = None
            message = "Imp defeated!"
            message_timer = 120

        if collide(player, exit_door):
            message = "You escaped with " + str(len(companions)) + " companion(s)!"
            running = False

        if message_timer > 0:
            message_timer -= 1
        else:
            message = None

        screen.fill((0, 0, 0))

        exit_door.draw(screen, font)
        player.draw(screen, font)
        if familiar:
            familiar.draw(screen, font)
        if cleric:
            cleric.draw(screen, font)
        if imp:
            imp.draw(screen, font)
        for idx, comp in enumerate(companions):
            comp_offset_x = player.x + (-40 + idx * 40)
            comp_offset_y = player.y + 40
            text = font.render(comp.char, True, comp.color)
            rect = text.get_rect(center=(comp_offset_x, comp_offset_y))
            screen.blit(text, rect)

        if message:
            msg_surf = small_font.render(message, True, pygame.Color("white"))
            screen.blit(msg_surf, (20, 440))

        pygame.display.flip()
        clock.tick(60)

    pygame.time.wait(1000)
    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
