import sys
import pygame


def main() -> None:
    """Launch a basic Pygame window."""
    pygame.init()
    screen = pygame.display.set_mode((640, 480))
    pygame.display.set_caption("Nautiloid Adventure")
    font = pygame.font.SysFont(None, 48)
    text = font.render("Nautiloid Adventure", True, (255, 255, 255))
    clock = pygame.time.Clock()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        screen.fill((0, 0, 0))
        screen.blit(text, (20, 20))
        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
