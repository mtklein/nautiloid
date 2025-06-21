# Nautiloid Adventure Vertical Slice

This repository contains a prototype for a 2D RPG inspired by classics like Chrono Trigger. The scenario resembles an escape from a nautiloid ship, with original characters and dialogues. The goal is to demonstrate core gameplay features in a small vertical slice.

See `docs/vertical_slice_design.md` for the design overview. Run `python3 src/main.py` to try the text-based demo with multiple rooms, chests, and companion dialog.
You can also play a simple graphical port of the text adventure using Pygame:
`python3 src/pygame_adventure.py`.
The Pygame version now features attribute-based combat with floating HP numbers for damage and healing.

## SDL2 Prototype

A minimal C/SDL2 prototype resides in `nautiloid.c`. Build it with `ninja` to produce `out/nautiloid`.
It now uses SDL_ttf and the bundled Final Fantasy font for on-screen text.

