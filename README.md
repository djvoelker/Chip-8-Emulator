# Chip-8-Emulator
Chip-8 emulator written in C++ on Visual Studio. Chip-8 is an interpreted programming language used for 8-bit computers.

This emulator can take in a Chip-8 program and display the output using SDL. The program takes three command-line arguments:
1. The scale of the display: The Chip-8 video buffer only displays in a 64x32 resolution, so we need to scale this up for modern displays.
2. Delay between cycles: This specifies the delay in milliseconds between each cycle, as the Chip-8 has no specified clock speed.
3. The name of the ROM file to load.

The Wikipedia page for the Chip-8 (https://en.wikipedia.org/wiki/CHIP-8) was very helpful as it included the different instructions for the Chip-8.

My original inspiration for this project came from a post online on the emulator101 website: http://www.emulator101.com/welcome.html.
