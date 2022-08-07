#include <cstdint>
#include <fstream>
#include <random>
#include <chrono>
#include <cstring>
#include <iostream>


const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;


class Chip8 {

public:

    Chip8()
        : program_ctr(0x200), 
        random_gen(std::chrono::system_clock::now().time_since_epoch().count()) {

        //load fonts into the memory
        for (int i = 0; i < 80; i++) {
            memory[0x50 + i] = fonts[i]; //fonts are stored starting at address 0x50
        }
    }



    uint8_t registers[16];
    uint8_t memory[4096];

    uint16_t index_reg;     //stores memory addresses for operation
    uint16_t program_ctr;   //holds address of next instruction
    uint16_t stack[16];     //holds 16 program counters
    uint8_t stack_ptr;      //location of top of stack

    uint8_t delay_timer;    //decreases until reaches 0
    uint8_t sound_timer;    //plays sound if nonzero

    uint8_t keypad_state[16];     //CHIP-8 has 16 input keys.
    uint8_t keypad_state_copy[16];
    bool waiting_for_key;

    // keypad       keyboard
    // 1 2 3 C  ->  1 2 3 4 
    // 4 5 6 D      Q W E R 
    // 7 8 9 E      A S D F
    // A 0 B F      Z X C V

    uint32_t display[64 * 32]; //represents each of the 64*32 pixels as a uint32
    uint16_t opcode;        //represents a CPU instruction


    uint8_t fonts[80]{  //CHIP-8 has 16 fonts, each composed of 5 uint8_t values
        0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
        0x20, 0x60, 0x20, 0x20, 0x70,		// 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
        0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
        0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
        0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
        0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
        0xF0, 0x80, 0xF0, 0x80, 0x80	    // F
    };

    std::default_random_engine random_gen;
    std::uniform_int_distribution<unsigned short> random_byte;

    void Load_ROM(char const* file_in) {
        std::ifstream f(file_in, std::ios_base::binary | std::ios_base::ate);

        if (!f.is_open()) {
            std::cout << "Couldn't open file";
            return;
        }

        // Get size of file
        int f_size = (int)f.tellg();
        char* buffer = new char[f_size];

        f.seekg(0, std::ios_base::beg);
        f.read(buffer, f_size);
        f.close();

        for (int i = 0; i < f_size; i++) {
            memory[0x200 + i] = buffer[i];  //CHIP-8 programs start at address 0x200
        }

        delete[] buffer;

    }

    void Cycle() {
        program_ctr += 2;

        //opcode consists of two bytes in memory
        opcode = (memory[program_ctr] << 8u) | memory[program_ctr + 1];

        Execute(opcode);

        if (delay_timer > 0) --delay_timer;
        if (sound_timer > 0) --sound_timer;
    }


private:

    void Execute(uint16_t opcode_in) {
        int first_nib = (opcode_in & 0xF000u) >> 12u;
        switch (first_nib) {
        case 0x0: {
            int last_nib = opcode_in & 0x00FFu;
            switch (last_nib) {
            case 0xE0: op_00E0(); break;
            case 0xEE: op_00EE(); break;
            default: op_NULL(); break;
            }
        }
        case 0x1: op_1NNN(); break;
        case 0x2: op_2NNN(); break;
        case 0x3: op_3XNN(); break;
        case 0x4: op_4XNN(); break;
        case 0x5: op_5XY0(); break;
        case 0x6: op_6XNN(); break;
        case 0x7: op_7XNN(); break;
        case 0x8: {
            int last_nib = opcode_in & 0x000Fu;
            switch (last_nib) {
            case 0: op_8XY0(); break;
            case 1: op_8XY1(); break;
            case 2: op_8XY2(); break;
            case 3: op_8XY3(); break;
            case 4: op_8XY4(); break;
            case 5: op_8XY5(); break;
            case 6: op_8XY6(); break;
            case 7: op_8XY7(); break;
            case 0xE: op_8XYE(); break;
            default: op_NULL(); break;
            }
        }
        case 0x9: op_9XY0(); break;
        case 0xA: op_ANNN(); break;
        case 0xB: op_BNNN(); break;
        case 0xC: op_CXNN(); break;
        case 0xD: op_DXYN(); break;
        case 0xE: {
            int last_nib = opcode_in & 0x00FFu;
            switch (last_nib) {
            case 0x9E: op_EX9E(); break;
            case 0xA1: op_EXA1(); break;
            default: op_NULL(); break;
            }
        }
        case 0xF: {
            int last_nib = opcode_in & 0x00FFu;
            switch (last_nib) {
            case 0x07: op_FX07(); break;
            case 0x0A: op_FX0A(); break;
            case 0x15: op_FX15(); break;
            case 0x18: op_FX18(); break;
            case 0x1E: op_FX1E(); break;
            case 0x29: op_FX29(); break;
            case 0x33: op_FX33(); break;
            case 0x55: op_FX55(); break;
            case 0x65: op_FX65(); break;
            default: op_NULL(); break;
            }
        }
        default: op_NULL(); break;

        }
    }


    void op_NULL() { }

    //
    // Instructions for the CHIP-8 (there are 34)
    // NNN: address
    // NN: 8-bit constant
    // N: 4-bit constant
    // X and Y: 4-bit register identifier
    // I: 16-bit index register
    //

    //Clears the screen.
    void op_00E0() {
        std::fill_n(display, 64 * 32, 0);
    }

    //Returns from a subroutine
    void op_00EE() {
        program_ctr = stack[stack_ptr--];
    }


    //Jumps to address NNN
    void op_1NNN() {
        uint16_t address = opcode & 0x0FFFu;
        program_ctr = address;
    }


    //Calls subroutine at NNN
    void op_2NNN() {
        uint16_t address = opcode & 0x0FFFu;
        stack[stack_ptr] = program_ctr;
        stack_ptr++;
        program_ctr = address;
    }


    //Skips the next instruction (usually a jump to skip a code block) if VX equals NN
    void op_3XNN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t NN = opcode & 0x00FFu;

        if (registers[VX] == NN) {
            program_ctr += 2;
        }
    }


    //Skips the next instruction if VX does not equal NN
    void op_4XNN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t NN = opcode & 0x00FFu;

        if (registers[VX] != NN) {
            program_ctr += 2;
        }
    }


    //Skips the next instruction if VX equals VY
    void op_5XY0() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (registers[VX] == registers[VY]) {
            program_ctr += 2;
        }
    }


    //Sets VX to NN
    void op_6XNN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t NN = opcode & 0x00FFu;

        registers[VX] = NN;
    }


    //Adds NN to VX (carry flag is not changed)
    void op_7XNN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t NN = opcode & 0x00FFu;

        registers[VX] += NN;
    }


    //Sets VX to the value of VY
    void op_8XY0() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        registers[VX] = registers[VY];
    }

    //Sets VX to VX or VY (bitwise OR operation)
    void op_8XY1() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        registers[VX] = registers[VX] | registers[VY];
    }

    //Sets VX to VX and VY (bitwise AND operation)
    void op_8XY2() {
        int8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        registers[VX] = registers[VX] & registers[VY];
    }

    //Sets VX to VX xor VY
    void op_8XY3() {
        int8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        registers[VX] = registers[VX] ^ registers[VY];
    }

    //Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not. 
    void op_8XY4() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        uint16_t res = registers[VX] + registers[VY];

        if (res > 255U) {
            registers[0xF] = 1;
        }
        else {
            registers[0xF] = 0;
        }

        registers[VX] = res & 0xFFu;
    }

    //VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void op_8XY5() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (registers[VX] > registers[VY]) {
            registers[0xF] = 0;
        }
        else {
            registers[0xF] = 1;
        }

        registers[VX] = registers[VX] - registers[VY];
    }

    //Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
    void op_8XY6() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t LSB = (registers[VX] & 0x1u);

        registers[0xF] = LSB;  //Saves the least significant bit in VF
        registers[VX] = registers[VX] >> 1;
    }

    //Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
    void op_8XY7() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (registers[VY] > registers[VX]) {
            registers[0xF] = 1;
        }
        else {
            registers[0xF] = 0;
        }

        registers[VX] = registers[VY] - registers[VX];
    }

    //Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
    void op_8XYE() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t MSB = (registers[VX] & 0x80u) >> 7u;

        registers[0xF] = MSB;
        registers[VX] = registers[VX] << 1;
    }


    //Skips the next instruction if VX does not equal VY.
    void op_9XY0() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;

        if (registers[VX] != registers[VY]) {
            program_ctr += 2;
        }
    }


    //Sets I to the address NNN.
    void op_ANNN() {
        uint16_t NNN = opcode & 0x0FFFu;

        index_reg = NNN;
    }


    //Jumps to the address NNN plus V0.
    void op_BNNN() {
        uint16_t NNN = opcode & 0x0FFFu;

        program_ctr = NNN + registers[0];
    }


    //Sets VX to the result of a bitwise AND operation on a random number (Typically: 0 to 255) and NN.
    void op_CXNN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        registers[VX] = random_byte(random_gen) & byte;
    }


    //Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
    void op_DXYN() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VY = (opcode & 0x00F0u) >> 4u;
        uint8_t N = opcode & 0x000Fu;

        //VF will be set to collision
        registers[0xF] = 0;

        //Wraps around screen if beyond boundaries
        uint8_t x_pos = registers[VX] % VIDEO_WIDTH;
        uint8_t y_pos = registers[VY] % VIDEO_HEIGHT;

        for (int r = 0; r < N; r++) {
            uint8_t sprite_byte = memory[index_reg + r];

            for (int c = 0; c < 8; c++) {
                uint8_t sprite_pixel = sprite_byte & (0x80u >> c);
                uint32_t* screen_pixel = &display[(y_pos + r) * VIDEO_WIDTH + (x_pos + c) * VIDEO_HEIGHT];

                if (sprite_pixel) {
                    //Collision is present
                    if (*screen_pixel == 0xFFFFFFFF) {
                        registers[0xF] = 1;
                    }

                    //XOR the screen pixel with the sprite pixel
                    *screen_pixel = *screen_pixel ^ 0xFFFFFFFF;
                }
            }

        }
    }


    //Skips the next instruction if the key stores in VX is pressed.
    void op_EX9E() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t key = registers[VX];

        if (keypad_state[key]) {
            program_ctr += 2;
        }
    }

    //Skips the next instruction if the key stored in VX is not pressed.
    void op_EXA1() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t key = registers[VX];

        if (!keypad_state[key]) {
            program_ctr += 2;
        }
    }


    //Sets VX to the value of the delay timer.
    void op_FX07() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        registers[VX] = delay_timer;
    }

    //A key press is awaited, and then stored in VX. Halts other operations.
    void op_FX0A() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        if (!waiting_for_key) {
            memcpy(keypad_state_copy, keypad_state, 16);
            waiting_for_key = 1;
            return;
        }
        else {
            for (int i = 0; i < 16; i++) {
                if (keypad_state[i] && !keypad_state_copy) {
                    waiting_for_key = 0;
                    registers[VX] = i;
                    program_ctr += 2;
                    return;
                }

                //Copying the keypad state allows a key that was pressed twice in a row to be detected
                keypad_state_copy[i] = keypad_state[i];
            }

            return;
        }
    }

    //Sets the delay timer to VX.
    void op_FX15() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        delay_timer = registers[VX];
    }

    //Sets the sound timer to VX.
    void op_FX18() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        sound_timer = registers[VX];
    }

    //Adds VX to I. VF is not affected.
    void op_FX1E() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        index_reg += registers[VX];
    }

    //Sets I to the location of the sprite for the character in VX.
    void op_FX29() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VX_value = registers[VX];

        //Fonts start at address 0x50 and are 5 bytes each
        index_reg = 0x50 + (5 * VX_value);
    }

    //Stores the binary-coded decimal representation of VX in addresses I, I+1, and I+2
    void op_FX33() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;
        uint8_t VX_value = registers[VX];

        //Store ones-place in address I+2
        memory[index_reg + 2] = VX_value % 10;
        VX_value = VX_value / 10;

        //Store tens-place in address I+1
        memory[index_reg + 1] = VX_value % 10;
        VX_value = VX_value / 10;

        //Store hundreds-place in address I
        memory[index_reg] = VX_value % 10;
    }

    //Stores from V0 to VX in memory, starting at address I.
    void op_FX55() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        for (uint8_t i = 0; i <= VX; i++) {
            memory[index_reg + i] = registers[i];
        }
    }

    //Fills from V0 to VX with values from memory, starting at address I.
    void op_FX65() {
        uint8_t VX = (opcode & 0x0F00u) >> 8u;

        for (uint8_t i = 0; i <= VX; i++) {
            registers[i] = memory[index_reg + i];
        }
    }


};