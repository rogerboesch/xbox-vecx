
#pragma once

#include "vec3x_emulator_types.hpp"
#include "vec3x_emulator_8910.hpp"
#include "vec3x_emulator_6809.hpp"

class Vec3XEmulator {
    friend class Vec3XEmulator6809;

public:
    Vec3XEmulator();

// Emulation
public:
    void Key(int vk, int pressed);
    void Init(int width, int height);
    void Start(const char* romfile, const char* romName, const char* cartfile, const char* cartName);
    void Frame();
    void Stop();
    void Pause();
    void Resume();
    void Command(int command, int parameter);

    Vec3XEmulator6809& GetCPU() { return ic6809;}
    
// Drawing
private:
    void CreateBuffer(long width, long height);
    void FastClearBuffer(Uint8 color);
    void ClearBuffer(Uint8 color);
    void SetPixel(int x, int y, Uint8 color);
    void DrawLine(int x1, int y1, int x2, int y2, Uint8 color);
    void Render();

// Helper
private:
    void LoadFile(const char* romfile, const char* romName, const char* cartfile, const char* cartName);
    void ResizeScreen(int width, int height);
    
// Internal
private:
    void Reset();
    void Emulate(long cycles);

// Intrenal bus hnadling
private:
    void SndUpdate();
    void AlgUpdate();
    void IntUpdate();
    unsigned char Read8(unsigned address);
    void Write8(unsigned address, unsigned char data);
    void ViaSstep0();
    void ViaSstep1();
    void AlgAddline(long x0, long y0, long x1, long y1, unsigned char color);
    void AlgSstep();

private:
    Vec3XEmulator6809 ic6809;
    Vec3XEmulator8910 ic8910;

private:
    byte* _pixelBuffer = NULL;

    long _scaling = 0;
    long _xOffset = 0;
    long _yOffset = 0;
    int _bufferWidth = 0;
    int _bufferHeight = 0;
    bool _isInitialised = false;
    bool _liveUpdate = false;
    bool _paused = false;
    
private:
    unsigned char _rom[8192];
    unsigned char _cartridge[32768];
    unsigned char _ram[1024];

    // sound chip registers
    unsigned _soundRegisters[16];
    unsigned _soundSelect;

    // VIA 6522 registers
    unsigned via_ora;
    unsigned via_orb;
    unsigned via_ddra;
    unsigned via_ddrb;
    unsigned via_t1on;  // is timer 1 on?
    unsigned via_t1int; // are timer 1 interrupts allowed?
    unsigned via_t1c;
    unsigned via_t1ll;
    unsigned via_t1lh;
    unsigned via_t1pb7; // timer 1 controlled version of pb7
    unsigned via_t2on;  // is timer 2 on?
    unsigned via_t2int; // are timer 2 interrupts allowed?
    unsigned via_t2c;
    unsigned via_t2ll;
    unsigned via_sr;
    unsigned via_srb;   // number of bits shifted so far
    unsigned via_src;   // shift counter
    unsigned via_srclk;
    unsigned via_acr;
    unsigned via_pcr;
    unsigned via_ifr;
    unsigned via_ier;
    unsigned via_ca2;
    unsigned via_cb2h;  // basic handshake version of cb2
    unsigned via_cb2s;  // version of cb2 controlled by the shift register

    // analog devices
    unsigned alg_rsh;   // zero ref sample and hold
    unsigned alg_xsh;   // x sample and hold
    unsigned alg_ysh;   // y sample and hold
    unsigned alg_zsh;   // z sample and hold
    unsigned alg_jch0;  // joystick direction channel 0
    unsigned alg_jch1;  // joystick direction channel 1
    unsigned alg_jch2;  // joystick direction channel 2
    unsigned alg_jch3;  // joystick direction channel 3
    unsigned alg_jsh;   // joystick sample and hold

    unsigned alg_compare;

    long alg_dx;     // delta x
    long alg_dy;     // delta y
    long alg_curr_x; // current x position
    long alg_curr_y; // current y position

    unsigned alg_vectoring; // are we drawing a vector right now?
    long alg_vector_x0;
    long alg_vector_y0;
    long alg_vector_x1;
    long alg_vector_y1;
    long alg_vector_dx;
    long alg_vector_dy;
    unsigned char alg_vector_color;

    long vector_draw_cnt;
    long vector_erse_cnt;
    vector_t vectors_set[2 * VECTOR_CNT];
    vector_t *vectors_draw;
    vector_t *vectors_erse;
    long vector_hash[VECTOR_HASH];

    long fcycles;
};
