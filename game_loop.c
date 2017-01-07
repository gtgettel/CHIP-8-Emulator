#define _BSB_SOURCE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "chip8.h"

int debug_enabled = 0; // debug mode flag


/*
 * dumps debug information
 */
void dumpDebug(struct chip8 *cpu){
  for (int i = 0; i < 16; i++)
    printf("register %d: %02X\n", i, cpu->registers[i]);
  printf("program_counter: %04X\n", cpu->program_counter);
  printf("index: %04X\n", cpu->index);
}


/*
 * records key press or release
 */
void updateKeys(struct chip8 *cpu){
  int key;
  key = getchar();
  if (key <= 15 && key >= 0){
    if (cpu->key[key]) // if key is not pressed
      cpu->key[key] = 0;
    else // if key is not pressed
      cpu->key[key] = 1;
  }
}


/*
 * updates graphics
 */
void updateGraphics(struct chip8 *cpu){
  // loop each row
  for (int height = 0; height < 32; height++){
    for (int width = 0; width < 64; width++){
      if (cpu->graphics[width + (height * 64)] == 1)
        printf("*");
      else
        printf(" ");
    }
    printf("\n");
  }
  printf("=================================\n");

  // reset flag
  cpu->draw_flag = FALSE;
}


/*
 * handles one instruction cycle
 */
void emulateCycle(struct chip8 *cpu){
  int key, temp_index;
  uint8_t temp_val, temp_register, x, y, n, pixel;
  uint16_t temp_val_16;
  uint16_t opcode = (cpu->memory[cpu->program_counter] << 8 | 
    cpu->memory[cpu->program_counter + 1]); // fetch opcode
  cpu->opcode = opcode;

  switch (opcode & 0xF000){ // Decode opcode
    // Execute opcode
    case 0x1000: // 1NNN: jumps to address NNN
      cpu->program_counter = (opcode & 0x0FFF) - 2;
      break;
    case 0x2000: // 2NNN: calls subroutine at address NNN
      cpu->stack[cpu->stack_pointer] = cpu->program_counter;
      cpu->stack_pointer++;
      cpu->program_counter = (opcode & 0x0FFF) - 2;
      break;
    case 0x3000: // 3XNN: skips next instruction if VX == NN
      if (cpu->registers[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        cpu->program_counter += 2;
      break;
    case 0x4000: // 4XNN: skips next instruction if VX != NN
      if (cpu->registers[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        cpu->program_counter += 2;
      break;
    case 0x5000: // 5XY0: skips next instruction if VX == VY
      if (cpu->registers[(opcode & 0x0F00) >> 8] == 
          cpu->registers[(opcode & 0x00FF) >> 4])
        cpu->program_counter += 2;
      break;
    case 0x6000: // 6XNN: sets VX to NN
      cpu->registers[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      break;
    case 0x7000: // 7XNN: Adds NN to VX
      cpu->registers[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      break;
    case 0x8000: // 8NNN: 
      switch (opcode & 0x000F){
        case 0x0000: // 8XY0: sets VX to VY
          cpu->registers[(opcode & 0x0F00) >> 8] = cpu->registers[(opcode & 0x00F0) >> 4];
          break;
        case 0x0001: // 8XY1: sets VX to VX | VY
          cpu->registers[(opcode & 0x0F00) >> 8] |= cpu->registers[(opcode & 0x00F0) >> 4];
          break;
        case 0x0002: // 8XY2: sets VX to VX & VY
          cpu->registers[(opcode & 0x0F00) >> 8] &= cpu->registers[(opcode & 0x00F0) >> 4];
          break;
        case 0x0003: // 8XY3: sets VX to VX ^ VY
          cpu->registers[(opcode & 0x0F00) >> 8] ^= cpu->registers[(opcode & 0x00F0) >> 4];
          break;
        case 0x0004: // 8XY4: adds VY to VX
                     // VF is set to 1 when there's a carry, and to 0 when there isn't
          if (cpu->registers[(opcode & 0x00F0) >> 4] > (0xFF - cpu->registers[(opcode & 0x0F00) >> 8]))
            cpu->registers[0xF] = 1;
          else
            cpu->registers[0xF] = 0;
          cpu->registers[(opcode & 0x0F00) >> 8] += cpu->registers[(opcode & 0x00F0) >> 4];
          break;
        case 0x0005: // 8XY5: subtracts VY from VX 
                     // VF is set to 0 when there's a borrow, and 1 when there isn't.
          if (cpu->registers[(opcode & 0x00F0) >> 4] > cpu->registers[(opcode & 0x0F00) >> 8])
            cpu->registers[0xF] = 0;
          else
            cpu->registers[0xF] = 1;
          cpu->registers[(opcode & 0x0F00) >> 8] += (-cpu->registers[(opcode & 0x00F0) >> 4]);
          break;
        case 0x0006: // 8XY6: VX >> 1
                     // VF is set to the value of the least significant bit of VX before the shift
          cpu->registers[0xF] = ((opcode & 0x0F00) >> 8) & (-((opcode & 0x0F00) >> 8));
          cpu->registers[(opcode & 0x0F00) >> 8] >>= 1;
          break;
        case 0x0007: // 8XY7: Sets VX to VY minus VX
                     // VF is set to 0 when there's a borrow, and 1 when there isn't
          if (cpu->registers[(opcode & 0x00F0) >> 4] < cpu->registers[(opcode & 0x0F00) >> 8])
            cpu->registers[0xF] = 0;
          else
            cpu->registers[0xF] = 1;
          cpu->registers[(opcode & 0x0F00) >> 8] = cpu->registers[(opcode & 0x00F0) >> 4] - 
            cpu->registers[(opcode & 0x0F00) >> 8];
          break; 
        case 0x000E: // 8XYE: VX << 1
                     // VF is set to the value of the most significant bit of VX before the shift
          temp_register = cpu->registers[(opcode & 0x0F00) >> 8];
          while (temp_register >>= 1)
            cpu->registers[0xF]++;
          cpu->registers[(opcode & 0x0F00) >> 8] <<= 1;
          break;
        default:
          printf("Opcode not recognized: %04X\n", opcode);
          dumpDebug(cpu);
          exit(0);
      }
      break;
    case 0x9000: // 9XY0: skips next instruction if VX != VY
      if (cpu->registers[(opcode & 0x0F00) >> 8] != 
          cpu->registers[(opcode & 0x00F0) >> 4])
        cpu->program_counter += 2;
      break;
    case 0xA000: // ANNN: sets I to the address NNN
      cpu->index = opcode & 0x0FFF;
      break;
    case 0xB000: // BNNN: jumps to address NNN plus V0
      cpu->program_counter = (opcode & 0x0FFF) + cpu->registers[0] - 2;
      break;
    case 0xC000: // CXNN: VX = rand() & NN
      cpu->registers[(opcode & 0x0F00) >> 8] = rand() & (opcode & 0x00FF);
      break;
    case 0xD000: // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a 
                 // height of N pixels. Each row of 8 pixels is read as bit-coded starting from memory 
                 // location I; I value doesn’t change after the execution of this instruction. As 
                 // described above, VF is set to 1 if any screen pixels are flipped from set to unset 
                 // when the sprite is drawn, and to 0 if that doesn’t happen
      x = cpu->registers[(opcode & 0x0F00) >> 8];
      y = cpu->registers[(opcode & 0x00F0) >> 4];
      n = opcode & 0x000F;
      cpu->registers[0xF] = 0;
      for (int height = 0; height < n; height++){
        pixel = cpu->memory[cpu->index + height];
        for (int width = 0; width < 8; width++){
          if ((pixel & (0x80 >> width)) != 0){
            if (cpu->graphics[(x + width + ((y + height) * 64))] == 1)
              cpu->registers[0xF] = 1;
            cpu->graphics[x + width + ((y + height) * 64)] ^= 1;
          }
        }
      }
      cpu->draw_flag = TRUE;
      break;
    case 0xE000: // ENNN: 
      switch (opcode & 0x00FF){
        case 0x009E: // EX9E: skips the next instruction if the key stored in VX is pressed
          if (cpu->key[cpu->registers[(opcode & 0x0F00) >> 8]])
            cpu->program_counter += 2;
          break;
        case 0x00A1: // EXA1: Skips the next instruction if the key stored in VX isn't pressed
          if (!cpu->key[cpu->registers[(opcode & 0x0F00) >> 8]])
            cpu->program_counter += 2;
          break;
        default:
          printf("Opcode not recognized: %04X\n", opcode);
          dumpDebug(cpu);
          exit(0);
      }
      break;
    case 0xF000: // FNNN: 
      switch (opcode & 0x00FF){
        case 0x0007: // FX07: sets VX to the value of the delay timer
          cpu->registers[(opcode & 0x0F00) >> 8] = cpu->delay_timer;
          break;
        case 0x000A: // FX0A: Wait for key, then store in VX
          while(1){
            printf("waiting for key press (0-15)...");
            key = getchar();
            if (key <= 15 && key >= 0){
              cpu->key[key] = 1;
              cpu->registers[(opcode & 0x0F00) >> 8] = key;
              break;
            }
          }
          break;
        case 0x0015: // FX15: sets delay timer to VX
          cpu->delay_timer = cpu->registers[(opcode & 0x0F00) >> 8];
          break;
        case 0x0018: // FX18: sets sound time to VX
          cpu->sound_timer = cpu->registers[(opcode & 0x0F00) >> 8];
          break;
        case 0x001E: // FX1E: adds VX to index register
          cpu->index += cpu->registers[(opcode & 0x0F00) >> 8];
          break;
        case 0x0029: // FX29: sets index register to the location of the sprite for the character in 
                     // VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font
          for (int i = 0; i < cpu->registers[(opcode & 0x0F00) >> 8]; i++){
            cpu->index += 0x5;
          }
          break;
        case 0x0033: // FX33: stores the binary-coded decimal representation of VX, with the most 
                     // significant of three digits at the address in index register, the middle digit 
                     // at index register plus 1, and the least significant digit at index register 
                     // plus 2.
          cpu->memory[cpu->index]     =  cpu->registers[(opcode & 0x0F00) >> 8] / 100;
          cpu->memory[cpu->index + 1] = (cpu->registers[(opcode & 0x0F00) >> 8] / 10) % 10;
          cpu->memory[cpu->index + 2] = (cpu->registers[(opcode & 0x0F00) >> 8] % 100) % 10;
          break;
        case 0x0055: // FX55: stores V0 to VX (including VX) in memory starting at address in index register
          temp_index = cpu->index;
          temp_val = 0;
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++){
            // first 8 bits
            temp_val = cpu->registers[i] >> 8;
            cpu->memory[temp_index] = temp_val;
            temp_index++;
            // second 8 bits
            temp_val = cpu->registers[i];
            cpu->memory[temp_index] = temp_val;
            temp_index++;
          }
          break;
        case 0x0065: // FX65: fills V0 to VX (including VX) with values from memory starting at 
                     // address in index register
          temp_index = cpu->index;
          temp_val_16 = 0;
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++){
            // first 8 bits
            temp_val_16 = cpu->memory[temp_index] << 8;
            temp_index++;
            // second 8 bits
            temp_val_16 += cpu->memory[temp_index];
            temp_index++;
            cpu->registers[i] = temp_val_16;
          }
          break; 
        default:
          printf("Opcode not recognized: %04X\n", opcode);
          dumpDebug(cpu);
          exit(0);
      }
      break;
    case 0x0000:
      switch(opcode & 0x000F){
        case  0x0000: // 0x00E0: clear screen
          if (opcode != 0x00E0){
            printf("Opcode not recognized: %04X\n", opcode);
            dumpDebug(cpu);
            exit(0);
          }
          memset(&cpu->graphics, 0, 64 * 32);
          cpu->draw_flag = TRUE;
          break;
        case 0x000E: // 0x00EE: returns from subroutine
          cpu->program_counter = cpu->stack[cpu->stack_pointer];
          cpu->stack_pointer--;
          break;
        default:
          printf("Opcode not recognized: %04X\n", opcode);
          dumpDebug(cpu);
          exit(0);
      }
      break;
    default:
      printf("Opcode not recognized: %04X\n", opcode);
      dumpDebug(cpu);
      exit(0);
  }
  cpu->program_counter += 2;
}


/*  
 * Resets chip8
 */
void coldBoot(struct chip8 *cpu){
  uint8_t fontset[80] = { 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  // reset chip
  memset(cpu, 0, sizeof(struct chip8));
  cpu->program_counter = 0x200;
  cpu->delay_timer = 60;

  // load fontset into memory
  for (int i = 0; i < 80; i++)
    cpu->memory[i] = fontset[i];

  // set start time
  gettimeofday(&cpu->clock_time, NULL);
}


/*
 * Updates timers (delay and sound)
 * Chip8 run at 60Hz
 */
void updateTimers(struct chip8 *cpu){
  struct timeval current_time;
  int usec_to_sleep;

  // update delay timer
  if (cpu->delay_timer > 0){
    if (cpu->delay_timer == 1){
      gettimeofday(&current_time, NULL); // determine time to sleep
      usec_to_sleep = current_time.tv_usec - cpu->clock_time.tv_usec;
      if (usec_to_sleep > 0)
        usleep(usec_to_sleep);
      gettimeofday(&cpu->clock_time, NULL); // reset clock time
    }
    cpu->delay_timer--;
    if (cpu->delay_timer == 0)
      cpu->delay_timer = 60;
  }

  // update sound timer
  if (cpu->sound_timer > 0){
    if (cpu->sound_timer == 1)
      printf("BEEP!\n");
    cpu->sound_timer--;
    if (cpu->sound_timer == 0)
      cpu->sound_timer = 60;
  } 
}


/*
 * Loads practice addition program
 * Adds input nums and displays results
 */
void loadGenericAddition(struct chip8 *cpu){
  uint8_t num1, num2;
  printf("Enter first integer: ");
  scanf("%hhu", &num1);
  printf("Enter second integer: ");
  scanf("%hhu", &num2);

  // load program starting at 0x200
  // 1: 0x60NN: V0 = 1
  cpu->memory[0x200] = 0x60;
  cpu->memory[0x201] = num1;
  // 2: 0x61NN: V1 = 2
  cpu->memory[0x202] = 0x61;
  cpu->memory[0x203] = num2;
  // 3: 0x8014: V0 = V0 + V1
  cpu->memory[0x204] = 0x80;
  cpu->memory[0x205] = 0x14;
  // 4: 0xF029: load sprite for V0 into index
  cpu->memory[0x206] = 0xF0;
  cpu->memory[0x207] = 0x29;
  // 5: 0xD005: draw sprite
  cpu->memory[0x208] = 0xD0;
  cpu->memory[0x209] = 0x05;
}


/*
 * converts ASCII to hex for use in reading program
 */
unsigned int asciiToHex(unsigned int ascii){
  if (ascii >= 48 && ascii <= 57) // 0-9
    return ascii - 48;
  else if (ascii >= 65 && ascii <= 70) // A-F
    return ascii - 65;
  else {
    printf("ERROR: Not hex value %u\n", ascii);
    exit(0);
  }
}


int main(int argc, char *argv[]){
  struct chip8 cpu1;
  FILE *program;
  unsigned int hex1, hex2 = 0;
  uint8_t half_opcode;
  int start = 0x0200;
  char opt;
  int program_arg = 1;

  // process flags
  opterr = 0;
  while ((opt = getopt(argc, argv, "dh")) != -1){
    program_arg++;
    switch (opt){
      case 'd': // debug
        debug_enabled = 1;
        break;
      case 'h': // help
        printf("USAGE: %s <program_name>\n", argv[0]);
        printf("OPTIONS: -dh\n");
        printf("\t-d: debug mode\n");
        printf("\t-h: help\n");
        return 0;
      default:
        break;
    }
  }

  if (argc < 2){ // no program given
    printf("USAGE: %s <program_name>\n", argv[0]);
    return 0;  
  }

  coldBoot(&cpu1); // setup chip8

  program = fopen(argv[program_arg], "r");
  if (program == NULL){
    fclose(program);
    printf("ERROR: program failed to open\n");
    return 0;
  } else {
    // load opcode 1-byte at a time
    while (1){
      // get first 4-bits
      hex1 = fgetc(program);
      while (hex1 == 10 || hex1 == 32) // ignore line feeds and spaces
        hex1 = fgetc(program);
      if (hex1 == EOF) 
        break;

      // get next 4-bits
      hex2 = fgetc(program);
      while (hex2 == 10 || hex2 == 32) // ignore line feeds and spaces
        hex2 = fgetc(program);
      if (hex2 == EOF) 
        break;

      // ignore "0x"
      if (hex1 == '0' && hex2 == 'x')
        continue;

      // assemble opcode
      half_opcode = asciiToHex(hex1);
      half_opcode <<= 4;
      half_opcode += asciiToHex(hex2);

      if (start >= 4096){ // check for end of memory
        fclose(program);
        printf("ERROR: Program too large for memory\n");
        return 0;
      }
      cpu1.memory[start] = half_opcode; // store 
      start++;
    }
    fclose(program);
  }
     

  gettimeofday(&cpu1.clock_time, NULL); // reset time after reading in program

  while(1){
    emulateCycle(&cpu1); // handle one opcode

    if (cpu1.draw_flag)
      updateGraphics(&cpu1); // draw to screen if needed

    //updateKeys(&cpu1);
    updateTimers(&cpu1); // TODO: limit to 60 instructions per sec

    if (debug_enabled){
      printf("Opcode: %04X\n", cpu1.opcode);
      dumpDebug(&cpu1);
      printf("Press any key to continue");
      getchar();
    }
  }

  return 0;
}