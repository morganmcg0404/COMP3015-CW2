# COMP3015-CW2

A small OpenGL game scene built in C++ with GLFW, GLAD, and GLM.

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
- Up / Down: Adjust bright threshold
- Page Up / Page Down: Adjust bloom strength
- Esc: Quit

## Build
Open the solution in Visual Studio and build the project in Debug|x64.

## Notes
- The project uses a borderless 1920x1080 windowed-fullscreen setup.
- Fireballs cost mana, and mana regenerates over time.
