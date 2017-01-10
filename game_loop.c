#define _BSB_SOURCE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <GLUT/glut.h>
#include "chip8.h"

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define DRAWWITHTEXTURE
#define MODIFIER 5

int debug_enabled = 0; // debug mode flag
int display_width = SCREEN_WIDTH * MODIFIER;
int display_height = SCREEN_HEIGHT * MODIFIER;
uint8_t screenData[SCREEN_HEIGHT][SCREEN_WIDTH][3]; 
struct chip8 *c8;


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
 * Keyboard down callback for GL
 */
void keyboardDown(unsigned char key, int x, int y){
  if(key == 27)    // esc
    exit(0);

  if(key == '1')      c8->key[0x1] = 1;
  else if(key == '2') c8->key[0x2] = 1;
  else if(key == '3') c8->key[0x3] = 1;
  else if(key == '4') c8->key[0xC] = 1;

  else if(key == 'q') c8->key[0x4] = 1;
  else if(key == 'w') c8->key[0x5] = 1;
  else if(key == 'e') c8->key[0x6] = 1;
  else if(key == 'r') c8->key[0xD] = 1;

  else if(key == 'a') c8->key[0x7] = 1;
  else if(key == 's') c8->key[0x8] = 1;
  else if(key == 'd') c8->key[0x9] = 1;
  else if(key == 'f') c8->key[0xE] = 1;

  else if(key == 'z') c8->key[0xA] = 1;
  else if(key == 'x') c8->key[0x0] = 1;
  else if(key == 'c') c8->key[0xB] = 1;
  else if(key == 'v') c8->key[0xF] = 1;
}


/*
 * Keyboard up callback for GL
 */
void keyboardUp(unsigned char key, int x, int y){
  if(key == '1')      c8->key[0x1] = 0;
  else if(key == '2') c8->key[0x2] = 0;
  else if(key == '3') c8->key[0x3] = 0;
  else if(key == '4') c8->key[0xC] = 0;

  else if(key == 'q') c8->key[0x4] = 0;
  else if(key == 'w') c8->key[0x5] = 0;
  else if(key == 'e') c8->key[0x6] = 0;
  else if(key == 'r') c8->key[0xD] = 0;

  else if(key == 'a') c8->key[0x7] = 0;
  else if(key == 's') c8->key[0x8] = 0;
  else if(key == 'd') c8->key[0x9] = 0;
  else if(key == 'f') c8->key[0xE] = 0;

  else if(key == 'z') c8->key[0xA] = 0;
  else if(key == 'x') c8->key[0x0] = 0;
  else if(key == 'c') c8->key[0xB] = 0;
  else if(key == 'v') c8->key[0xF] = 0;
}


/*
 * converts ASCII to hex for use in reading program
 */
unsigned int asciiToHex(unsigned int ascii){
  if (ascii >= 48 && ascii <= 57) // 0-9
    return ascii - 48;
  else if (ascii >= 65 && ascii <= 70) // A-F
    return ascii - 65 + 10;
  else {
    return 16;
  }
}

/*
 * Setup Texture
 */
void setupTexture(){
  int x, y;

  // Clear screen
  for(y = 0; y < SCREEN_HEIGHT; ++y)    
    for(x = 0; x < SCREEN_WIDTH; ++x)
      screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;

  // Create a texture 
  // Level = none; border = none; format = RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

  // Set up the texture
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); 

  // Enable textures
  glEnable(GL_TEXTURE_2D);
}


/* 
 * Draw on texture
 */
void updateTexture(struct chip8 *cpu){ 
  int x, y;

  // Update pixels
  for(y = 0; y < 32; ++y)   
    for(x = 0; x < 64; ++x)
      if(cpu->graphics[(y * 64) + x] == 0)
        screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;  // Disabled
      else 
        screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 255;  // Enabled
    
  // Update Texture
  glTexSubImage2D(GL_TEXTURE_2D, 0 ,0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

  glBegin( GL_QUADS );
    glTexCoord2d(0.0, 0.0);   glVertex2d(0.0, 0.0);
    glTexCoord2d(1.0, 0.0);   glVertex2d(display_width, 0.0);
    glTexCoord2d(1.0, 1.0);   glVertex2d(display_width, display_height);
    glTexCoord2d(0.0, 1.0);   glVertex2d(0.0, display_height);
  glEnd();
}

/*
 * Non-texture legacy routine
 * Draw vertices
 */
void drawPixel(int x, int y){
  glBegin(GL_QUADS);
    glVertex3f((x * MODIFIER) + 0.0f,     (y * MODIFIER) + 0.0f,   0.0f);
    glVertex3f((x * MODIFIER) + 0.0f,     (y * MODIFIER) + MODIFIER, 0.0f);
    glVertex3f((x * MODIFIER) + MODIFIER, (y * MODIFIER) + MODIFIER, 0.0f);
    glVertex3f((x * MODIFIER) + MODIFIER, (y * MODIFIER) + 0.0f,   0.0f);
  glEnd();
}


/*
 * Non-texture legacy routine
 * Draws sprites
 */
void updateQuads(struct chip8 *cpu){
  int x, y;
  // Draw
  for(y = 0; y < 32; ++y)   
    for(x = 0; x < 64; ++x)
    {
      if(cpu->graphics[(y*64) + x] == 0) 
        glColor3f(0.0f,0.0f,0.0f); // draw white
      else 
        glColor3f(1.0f,1.0f,1.0f); // draw black

      drawPixel(x, y);
    }
}


/*
 * resize window
 */ 
void reshape_window(GLsizei w, GLsizei h){
  glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
  glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);        
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);

  // Resize quad
  display_width = w;
  display_height = h;
}


/*
 * handles one instruction cycle
 */
void emulateCycle(struct chip8 *cpu){
  int key;
  int dont_increment = 0;
  uint8_t temp_register, x, y, n, pixel;
  uint16_t opcode = (cpu->memory[cpu->program_counter] << 8 | 
    cpu->memory[cpu->program_counter + 1]); // fetch opcode
  cpu->opcode = opcode;

  switch (opcode & 0xF000){ // Decode opcode
    // Execute opcode
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
    case 0x1000: // 1NNN: jumps to address NNN
      cpu->program_counter = (opcode & 0x0FFF);
      dont_increment = 1;
      break;
    case 0x2000: // 2NNN: calls subroutine at address NNN
      cpu->stack[cpu->stack_pointer] = cpu->program_counter;
      cpu->stack_pointer++;
      cpu->program_counter = (opcode & 0x0FFF);
      dont_increment = 1;
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
      cpu->program_counter = (opcode & 0x0FFF) + cpu->registers[0];
      dont_increment = 1;
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
          // TODO:
          while(1){
            printf("waiting for key press (0-15)...");
            key = getchar();
            if (key <= 15 && key >= 0){
              cpu->key[key] = 1;
              cpu->registers[(opcode & 0x0F00) >> 8] = key;
              printf("\n");
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
          // TODO: does index get incremented
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
            cpu->memory[cpu->index + i] = cpu->registers[i];
          cpu->index += ((opcode & 0x0F00) >> 8) + 1;
          break;
        case 0x0065: // FX65: fills V0 to VX (including VX) with values from memory starting at 
                     // address in index register
          for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
            cpu->registers[i] = cpu->memory[cpu->index + i];
          cpu->index += ((opcode & 0x0F00) >> 8) + 1;
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

  if (debug_enabled){
    printf("Opcode: %04X\n", cpu->opcode);
    dumpDebug(cpu);
    printf("Press any key to continue");
    getchar();
  }

  if (!dont_increment)
    cpu->program_counter += 2;
  updateTimers(cpu);
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

  c8 = cpu;
}


void display(){
  emulateCycle(c8);
  
  if(c8->draw_flag){
    // Clear framebuffer
    glClear(GL_COLOR_BUFFER_BIT);
        
#ifdef DRAWWITHTEXTURE
    updateTexture(c8);
#else
    updateQuads(c8);   
#endif      

    // Swap buffers!
    glutSwapBuffers();    

    // Processed frame
    c8->draw_flag = FALSE;
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


int main(int argc, char *argv[]){
  struct chip8 cpu1;
  FILE *program;
  unsigned int hex1, hex2 = 0;
  uint8_t half_opcode;
  int start = 0x0200;
  int opt;
  int program_arg = 1;
  int t_flag = 0;
  long psize;
  char *buffer;
  size_t result;

  // process flags
  opterr = 0;
  while ((opt = getopt(argc, argv, "dht")) != -1){
    program_arg++;
    switch (opt){
      case 'd': // debug
        debug_enabled = 1;
        break;
      case 't': // text file
        t_flag = 1;
        break;
      case 'h': // help
        printf("USAGE: %s <program_name>\n", argv[0]);
        printf("OPTIONS: -dh\n");
        printf("\t-d: debug mode\n");
        printf("\t-h: help\n");
        printf("\t-t: load text file\n");
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

  if (t_flag){
    program = fopen(argv[argc - 1], "r");
    if (program == NULL){
      fclose(program);
      printf("ERROR: program failed to open [1]\n");
      return 0;
    } else {
      // load opcode 1-byte at a time
      while (1){
        // get first 4-bits
        hex1 = fgetc(program);
        if (hex1 == EOF) 
          break;
        while (asciiToHex(hex1) == 16) // ignore line feeds and spaces
          hex1 = fgetc(program);

        // get next 4-bits
        hex2 = fgetc(program);
        if (hex2 == EOF) 
          break;
        while (asciiToHex(hex1) == 16) // ignore line feeds and spaces
          hex2 = fgetc(program);

        // assemble opcode
        half_opcode = asciiToHex(hex1);
        half_opcode <<= 4;
        half_opcode += asciiToHex(hex2);

        if (start >= 4096){ // check for end of memory
          fclose(program);
          printf("ERROR: Program too large for memory [1]\n");
          return 0;
        }

        cpu1.memory[start] = half_opcode; // store 
        start++;
      }
      fclose(program);
    }
  } else {
    program = fopen(argv[argc - 1], "rb");
    if (program == NULL){
      fclose(program);
      printf("ERROR: program failed to open [2]\n");
      return 0;
    } else {
      // Check file size
      fseek(program , 0 , SEEK_END);
      psize = ftell(program);
      rewind(program);
  
      // Allocate memory to contain the whole file
      buffer = (char*)malloc(sizeof(char) * psize);
      if (buffer == NULL){
        printf("Buffer error\n");
        fclose(program);
        return 0;
      }

      // Copy the file into the buffer
      result = fread (buffer, 1, psize, program);
      if (result != psize){
        printf("Reading error\n"); 
        return 0;
      }

      // Copy buffer to Chip8 memory
      if((4096-512) > psize){
        for(int i = 0; i < psize; ++i)
          cpu1.memory[i + 512] = buffer[i];
      } else {
        printf("Error: Program too large for memory [2]\n");
      }
  
      free(buffer);
    }
    fclose(program);
  }

  gettimeofday(&cpu1.clock_time, NULL); // reset time after reading in program

  glutInit(&argc, argv);     
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

  glutInitWindowSize(display_width, display_height);
  glutInitWindowPosition(320, 320);
  glutCreateWindow("Chip8");
  
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutReshapeFunc(reshape_window);       
  glutKeyboardFunc(keyboardDown);
  glutKeyboardUpFunc(keyboardUp); 

#ifdef DRAWWITHTEXTURE
  setupTexture();     
#endif  

  glutMainLoop(); 

  return 0;
}