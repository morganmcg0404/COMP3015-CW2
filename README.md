# COMP3015-CW2

## Project report

This project is a small OpenGL action scene built in C++ with GLFW, GLAD, GLM, and custom GLSL shaders. It started from the supplied coursework scene/template and was extended into a playable arena game with stylised rendering and post-processing.

### Development environment

- **Visual Studio:** Visual Studio 2022 using the v143 toolset
- **Operating System:** Windows 11
- **Tested locally with:** the `Debug|x64` configuration

### How it works

The game renders a simple arena with a player, enemies, fireball attacks, a mana system, and a HUD. The rendering pipeline is split into a few stages:

1. **Scene rendering** — the world is drawn with OpenGL using a shared lighting shader setup.
2. **Shadow mapping** — the scene is rendered from the light's point of view into a depth texture, then sampled during the main pass so objects cast and receive shadows.
3. **Toon shading** — the main lighting is quantised into softer bands to create a cel-shaded look.
4. **Post-processing** — bloom, vignette, and screen-space edge detection are applied before presenting the final image.
5. **HUD and gameplay** — the screen overlay draws the mana bar, health bar, score, time, and crosshair while the gameplay logic updates player movement, fireballs, and enemy behaviour.

The arena also includes interaction mechanics such as jumping, sprinting, shooting fireballs, enemies that move toward the player, and resource management through mana and health.

### What makes the shader program special

The shader setup combines several effects rather than using just one standalone technique:

- **Toon shading with shadows** — the scene is not only toon shaded, but the toon shader also receives the same shadow information as the standard lighting shader.
- **Shadow mapping with PCF** — shadows are produced using a separate depth pass and filtered with percentage-closer filtering to reduce jagged edges.
- **Normal + depth edge detection** — the final composite pass uses the scene depth and normal buffers to detect outlines in screen space.
- **Stylised gameplay presentation** — the rendering supports the game-like elements of the scene instead of being a pure technical demo.

The only thing I started with was a basic scene with a green plane, blue sky and bright yellow sun. I knew that I wanted to use a bloom shader so thats the first thing I worked towards. I still had no idea on what I wanted to create so just looked at what shaders could be cool to work with, I looked through and thought that a particle fountain could be fun. After seeing the particle fountain I imediately had the idea for a throwing a fireball and using the particle as the fire trail behind it. After creating this I got the idea for a zombie surival type game where the player kills enemies through hitting them with with the fireball. The play would gain score through killing enemies and a timer would track how long they can last.

### What makes it unique

Compared with a standard OpenGL coursework scene, this prototype is more distinctive because it combines:

- a playable arena with enemies and combat,
- toon shading rather than only physically shaded rendering,
- shadow mapping integrated into both standard and toon lighting,
- a post-process outline effect based on depth and normals,
- a HUD with score, health, mana, and time,
- toggles for key visual effects so the scene can be demonstrated clearly.

### Additional notes

- The project uses a borderless 1920x1080 windowed-fullscreen setup.
- Fireballs cost mana, and mana regenerates over time.
- The scene includes keyboard and mouse controls for movement and combat.
- Visual features can be toggled during gameplay to compare the final look with and without the effects.

### Repository and video

- **GitHub repository:** [GitHub repo link](https://github.com/morganmcg0404/COMP3015-CW2)
- **Unlisted YouTube video:** [YouTube video link](https://youtu.be/<your-unlisted-video>)

## Features

- First-person player movement
- Jumping and sprinting
- Throwing fireballs
- Fire trails and explosions
- Enemies that spawn around the arena
- Mana system
- Bloom, vignette, and crosshair HUD

## Controls

- W / A / S / D: Move
- Mouse: Look around
- Space: Jump
- Left Shift / Right Shift: Sprint
- F: Throw fireball
- B: Toggle bloom
- V: Toggle vignette
- C: Toggle toon shading
- X: Toggle edge detection
- Up / Down: Adjust bright threshold
- Page Up / Page Down: Adjust bloom strength
- Esc: Quit

## Build

Open the solution in Visual Studio and build the project in Debug|x64.

## Notes

- The project uses a borderless 1920x1080 windowed-fullscreen setup.
- Fireballs cost mana, and mana regenerates over time.

## AI assistance note

AI assistance was used throughout development to support planning, debugging, report drafting, shader changes, gameplay iteration, and other code edits.
The full prompt transcript for AI-assisted development is recorded in [ai-transcript.md](ai-transcript.md).
