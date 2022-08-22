
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum {
    VECTREX_MHZ     = 1500000, // speed of the vectrex being emulated
    VECTREX_COLORS  = 128,     // number of possible colors ... grayscales
    ALG_MAX_X       = 33000,
    ALG_MAX_Y       = 41000
};

enum {
    VECTREX_PC,     // reg_pc, program counter
    VECTREX_REG_X,  // reg_x, x-register
    VECTREX_REG_Y,  // reg_y, y-register
    VECTREX_ACC_A,  // reg_a, accumlator a
    VECTREX_ACC_B,  // reg_b, accumlator b
    VECTREX_HSP,    // reg_u, hardware stack pointer
    VECTREX_USP,    // reg_s, user stack pointer
    VECTREX_REG_DP, // reg_dp, direct page register
    VECTREX_REG_CC  // reg_cc, condition code register
};

enum {
    VECTREX_PDECAY = 30,                            // phosphor decay rate
    FCYCLES_INIT = VECTREX_MHZ / VECTREX_PDECAY,    // number of 6809 cycles before a frame redraw
    VECTOR_CNT = VECTREX_MHZ / VECTREX_PDECAY,      // max number of possible vectors that maybe on the screen at one time
    VECTOR_HASH = 65521
};

typedef enum _DebugCommand {
    DEBUG_GOTO_DISASSEMBLY = 1, DEBUG_GOTO_MEMORY,
    DEBUG_PAUSE, DEBUG_SETBREAKPOINT,
    DEBUG_STEP_OVER, DEBUG_STEP_INTO, DEBUG_STEP_OUT,
    DEBUG_MEM_EDIT, DEBUG_MEM_SHOWCHECKSUM,
    DEBUG_MEM_PAGEDOWN, DEBUG_MEM_PAGEUP,
    DEBUG_CURSOR_DOWN, DEBUG_CURSOR_UP,
    DEBUG_LIVE_UPDATE
} DebugCommand;

enum {
    PL1_LEFT = 0,
    PL1_RIGHT,
    PL1_UP,
    PL1_DOWN,

    PL2_LEFT,
    PL2_RIGHT,
    PL2_UP,
    PL2_DOWN
};

typedef struct vector_type {
    long x0, y0;             // start coordinate
    long x1, y1;             // end coordinate
    unsigned char color;     // 0..VECTREX_COLORS-1
} vector_t;

typedef unsigned char byte;
typedef uint8_t Uint8;
typedef uint32_t Uint32;
