# Old‑School RPG Engine — High‑Level Design

## 1. Vision & Scope

- **Goal:** Build a light‑weight 2D engine capable of retelling the Baldur’s Gate 3 *Nautiloid* intro in 16‑bit glory. Think `FF6` opera house meets `Chrono Trigger` cut‑scenes, with `Secret of Mana`’s action feel where feasible.
- **Target Platform:** Desktop + handheld (SDL‑based), 60 fps at 1080p yet happy on a Steam Deck.
- **Pipeline:** JSON/YAML data, hot‑reload, git‑friendly. One binary, plus a companion *Editor* mirroring RPG Maker’s database panes.

## 2. Top‑Level Modules (mirrors the screenshot)

| DB Tab                       | Engine Sub‑system                          | Notes                              |
| ---------------------------- | ------------------------------------------ | ---------------------------------- |
| **Actors**                   | Entity Component System (ECS) for PCs/NPCs | Stats, sprites, dialogue hooks.    |
| **Classes**                  | Job templates                              | Growth curves, equipment grids.    |
| **Skills**                   | Action definitions                         | Cost, animation cue, hit box data. |
| **Items / Weapons / Armors** | Asset bundles                              | Sprite + effect + shop metadata.   |
| **Enemies / Troops**         | AI archetypes + encounter packs            | Supports mixed parties and waves.  |
| **States**                   | Buff / debuff system                       | Time‑based or conditional.         |
| **Animations**               | Sprite‑sheet player                        | Reference by ID inside Skills.     |
| **Tilesets**                 | Tiled‑style TMX importer                   | 32×32 base tile, parallax layers.  |
| **Common Events**            | Re‑usable script snippets (Lua)            | Eg. door, pod‑unlock, teleport.    |
| **System 1 / System 2**      | Global constants & UI skin                 | Font, sound bank, ATB tuning.      |
| **Types**                    | Enumerations registry                      | Damage types, weapon tags.         |
| **Terms**                    | Localisation strings                       | Plural‑aware.                      |

## 3. Runtime Architecture

1. **Renderer** — OpenGL sprite batcher, 4 layers (BG 0‑2, FG) + screen‑wide shader stack (mode‑7 fake rotation for flying Nautiloid).
2. **ECS Core** — `Transform`, `Sprite`, `Physics`, `Input`, `AI`, `Combatant`, `DialogueNode`.
3. **Event Bus** — Cut‑scene and gameplay share same signal pipeline → deterministic replays.
4. **Combat Engine**
   - **Tempo:** Hybrid ATB—queue. Party acts in order; hold button → live pause ala *Secret of Mana* ring menu.
   - **Hit Resolution:** DamageType × Resist × RNG table; supports positional cones for Mind Flayer blast.
5. **Scripting** — Embedded Lua 5.4; sandboxed; coroutines drive cinematics.
6. **Audio** — FMOD or OpenAL; tracker‑style music, SFX slots with RT‑pitch.

## 4. Toolchain / Editor

- **Database panes** exactly as screenshot for instant familiarity.
- **Tile‑map view** with event layer; REPL console for Lua snippets.
- **Animation previewer** — onion‑skin frames, hit‑box overlay.
- **Cut‑scene timeline** — keyframes call Lua; camera, shake, sprite swap.

## 5. Re‑Building the *Nautiloid* Sequence

### 5.1 Scene List

1. **Ext. Avernus — Mode 7 Fly‑over**
   - Background city layer scrolls; foreground tentacles.
   - Camera tween + parallax; thunder SFX.
2. **Pod Chamber Awakening**
   - Player `Actor` spawns `State: Petrified` → Lua lifts state after dialogue.
   - Tutorial prompts (interactive text boxes bound to `CommonEvent: ShowHint`).
3. **Rescue Lae’zel Beat**
   - Door puzzle uses `Skill: Stomp` on console → sets switch, plays `Animation: Sparks`.
4. **Hallway Skirmish**
   - `Troop` = 2 Imps + 1 BrainDog; ATB battle embedded on map, no scene load.
5. **Bridge Crash Cinematic**
   - `CameraShake`, `State: Falling` applies 30 frames; moves party to deck map.
6. **Avernus Dogfight** (optional boss tutorial)
   - Reuses fly‑over renderer, now interactive: dodge fireballs.
7. **Impact & Fade‑Out**
   - Timed white flash; save file banner.

### 5.2 Asset Budget

- **Tilesets:** 3 (Interior, Exterior Flesh, Avernus Sky) ≈ 768 tiles.
- **Sprites:**
  - 4‑dir walk, run, attack, hurt (FF6 style) — 6 frames each.
  - Large boss sprite 128×128 for Mind Flayer.
- **Music:** Chiptune cover of BG3 intro, 2:30 loop.

## 6. Combat Details (SNES DNA)

| Mechanic            | Borrowed From  | Implementation Twist                       |
| ------------------- | -------------- | ------------------------------------------ |
| **Row/Back‑attack** | FF6            | Tile‑based facing grants +10 % crit.       |
| **Unison Techs**    | Chrono Trigger | Trigger when two skills queued within 1 s. |
| **Ring Menu**       | Secret of Mana | Pause world; radial selection overlay.     |

## 7. File Formats

```yaml
actor:
  id: 7
  name: "Prisoner"
  class: 2  # Fighter
  base_stats: {hp: 120, mp: 10, str: 14, dex: 12}
  sprite: "actors/prisoner.png"
  dialogue: "pods_intro.yaml"
```

Everything hashed by `id` for network‑free deterministic builds.

## 8. Roadmap (6 Months)

| Month | Milestone                              |
| ----- | -------------------------------------- |
| 1     | Renderer + ECS skeleton, TMX loader    |
| 2     | Database Editor MVP, Actor movement    |
| 3     | Combat prototype (Imps fight)          |
| 4     | Cut‑scene tooling, Mode 7, audio       |
| 5     | Nautiloid content build, playtest loop |
| 6     | Polish, mod support, packaging         |

## 9. Extension Hooks

- **Procedural dialogue** — plug small GPT‑2 offline model.
- **Mod.io integration** — users swap scripts, sprites via `zip` mods.
- **Rollback net‑code** for coop (stretch goal).

---

### Cheat‑Sheet

- **Core loop:** `Update → Physics → Combat → Events → Render`.
- **Everything = Data:** engine recompiled rarely; content drives behavior.
- **One true random seed** per save → perfect replays.

> Build clever, ship small, and let the pixel art sing.

