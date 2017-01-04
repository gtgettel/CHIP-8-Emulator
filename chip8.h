#include <stdlib.h>
#include <stdint.h>
#include <time.h>

typedef int bool;
#define TRUE 1
#define FALSE 0

struct chip8 {
  uint16_t opcode; 
  uint8_t memory[4096]; // 4K memory
  uint8_t registers[16]; // 16 registers
  uint16_t index;
  uint16_t program_counter;

  // 0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
  // 0-x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
  // 0x200-0xFFF - Program ROM and work RAM

  uint8_t graphics[64 * 32]; // 2048 pixels - 64x32
  bool draw_flag;

  // register timers at 60Hz (aka 60 instructions per second)
  uint8_t delay_timer;
  uint8_t sound_timer;
  struct timeval clock_time;

  uint16_t stack[16];
  uint16_t stack_pointer;

  // HEX based keypad
  uint8_t key[16];

} chip8;