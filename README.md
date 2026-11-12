# CHAOS SHIFT

**Play it on itch.io: [cupcake666.itch.io/chaos-shift](https://cupcake666.itch.io/chaos-shift)**

A solo entry for the **GMTK Game Jam 2026** (theme: *Count Down*), made in **Unreal Engine 5.7** with gameplay
written in **C++**.

You run an industrial sorting line: grab parts with a robotic arm and drop them in the right container before a
180-second countdown runs out. Every 20 seconds the factory breaks its own rules. The controls invert, gravity
lets go and you are forced into a drone to chase floating parts, or the whole line goes into overdrive.

This repository contains the **source code and configuration**. The 3D models, textures and sounds are
third-party assets (Fab, Quixel Megascans, Freesound) that cannot be redistributed, so they are not included;
the playable build is on itch.io.

## Gameplay

- **Green part** → drop it in a container at the end of the lines. **Red (defective) part** → the reject
  container behind the arm.
- Right container: **+100** (**+200** while a mutator is active). Wrong container: **−50**.
- Every 20 seconds one mutator fires for 20 seconds, or nothing happens and you get a moment of calm:
  - **Scramble**: the arm's controls are inverted.
  - **Anti-Gravity**: parts float above their crates; you are thrown out of the arm and into the drone,
    whose magnetic grabber carries parts to the right container.
  - **Overdrive**: time dilation spikes and the factory doubles its output.
- The last ten seconds of the countdown tick out loud, faster and sharper.

## Code structure

All gameplay lives in `Source/Simulator`. The game is built on top of a physically based industrial simulator
I developed earlier as an academic project (a digital twin of a robotic sorting cell). The arcade layer
(chaos director, mutators, scoring, countdown, forced possession, levitation and drone flight) was written
during the jam. Class names are in Romanian; the translation is given below.

| Class | Meaning | Responsibility |
|---|---|---|
| `UChaosDirector` | – | `UTickableWorldSubsystem` that runs the countdown, schedules and applies the mutators (Scramble, Anti-Gravity, Overdrive), keeps the score and forces possession between arm and drone. Exposed to Blueprint/UMG for the HUD. |
| `ABratRobotic` | robotic arm | Player `APawn`: articulated joints selected and rotated one at a time, grab/release of parts, servo sounds, inverted controls during Scramble. |
| `ADrona` | drone | Second `APawn`: free flight with collision and a magnetic grabber that carries floating parts during Anti-Gravity. |
| `APiesa` | part | A part on the line, good or defective. |
| `ABanda` | conveyor belt | Moves parts along the line, with a looping belt sound. |
| `ASpawner` | spawner | Spawns parts on the conveyors; its rate is doubled by Overdrive. |
| `ACutie` | container | A drop area that accepts parts. |
| `AContorDepozitare` | storage counter | Checks whether a dropped part went to the right container and awards or deducts points. |
| `ASimulatorGameMode` | game mode | Background music, countdown ticks and the link between the level and the Chaos Director. |

Input uses **Enhanced Input** (`Config/DefaultInput.ini`).

## Controls

| Robotic arm | | Inspection drone | |
|---|---|---|---|
| `Q` / `E` | select active joint | `W` `A` `S` `D` | move |
| `A` / `D` | rotate base (360°) | `Space` / `Shift` | up / down |
| `W` / `S` | rotate joint up / down | `Mouse` | look |
| `Z` / `X` | roll forearm (360°) | `E` | grab / release part |
| `Mouse` | look | `F` | switch back to the arm |
| `Space` | grab / release part | | |
| `F` | switch to the drone | | |

## Building

Requires **Unreal Engine 5.7** and Visual Studio 2022 with the *Game development with C++* workload.
Right-click `Simulator.uproject` → *Generate Visual Studio project files*, then build the `Simulator Editor`
target. The C++ code compiles on its own, but the level and assets in `Content/` are not part of this repository,
so the game itself cannot be run from a clone. Download the build from itch.io to play.

## Credits

Code and game design by **Adam Iannis (Cupcake666)**. No generative AI was used for any art or audio in the game.

Third-party assets used in the game (not included in this repository), each under its own license:

- **3D art:** "Factory Environment Collection" by ArtCore Studios in collaboration with Epic Games (Fab; the robotic
  arm was split into articulated joints in Blender), Quixel Megascans (warehouse, rusty wheel), "Sci-fi Industrial
  Container Game 03" by CGGame (Fab), "The Sphere" by Treehouseassets (Fab).
- **Audio:** "Machine Rooms" by yellaUmbrella Music, "Glitch Noises" by Vladislav Zharkov and "Fun House Music" by
  juanjo_sound (Fab, Standard License); "Drone Flying" by Sheyvan, "Triple Ping Notification" by
  PiesHelpfulOven and "Access denied buzz" by Jacco18 (Freesound, CC0); "Lock click in steel door" by Zabuhailo
  and "Conveyor belt" by freemaster2 (Freesound, CC-BY 4.0).

## License

Source code released under the [MIT License](LICENSE). The third-party assets listed above keep their own licenses.
