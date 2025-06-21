# Nautiloid Adventure Vertical Slice

This repository contains a prototype for a 2D RPG inspired by classics like Chrono Trigger.
The scenario resembles an escape from a nautiloid ship with original characters and dialogue.
The goal is to demonstrate core gameplay features in a small vertical slice.

See `docs/vertical_slice_design.md` for the design overview.
The old text-based and Pygame demos were removed once the SDL2 prototype covered their mechanics.

## SDL2 Prototype
A minimal C/SDL2 prototype resides in `nautiloid.c`.
Build it with `ninja` to produce `out/nautiloid`.
It now uses SDL_ttf and the bundled Final Fantasy font for on-screen text.
The program includes a basic entity system with simple class data and sprite rendering.
Hold Shift while moving to sprint across rooms.

