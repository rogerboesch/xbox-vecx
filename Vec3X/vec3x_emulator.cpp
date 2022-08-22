//
//  vec3x_emulator.cpp
//  classics-coder
//
//  Created by Roger Boesch on 02.01.21.
//  Copyright © 2021 Roger Boesch. All rights reserved.
//

#include "pch.h"
#include "vec3x_emulator.hpp"

#define EMU_TIMER 20
#define USE_PIXEL_BUFFER 1

extern "C" {
    void vectrex_render_frame(byte* data) {}

    void audio_processor_start(void* audioclass) {}
    void audio_processor_stop(void) {}

    void vectrex_add_line(int x1, int y1, int x2, int y2, uint8_t color);
    void vectrex_update_cpu_view(unsigned pc, unsigned usp, unsigned hsp, unsigned acc_a, unsigned acc_b, unsigned reg_x, unsigned reg_y, unsigned reg_dp, unsigned reg_cc, long vectors) {}
    unsigned vectrex_get_register(int reg);

    void platform_print(const char* msg) { printf("%s", msg); }
}

Vec3XEmulator::Vec3XEmulator() : ic6809(this) {
}

#pragma mark - Drawing

void Vec3XEmulator::CreateBuffer(long width, long height) {
    if (_pixelBuffer != NULL) {
        free(_pixelBuffer);
        _pixelBuffer = NULL;
    }
    
    _bufferWidth = (int)width;
    _bufferHeight = (int)height;
    
    _pixelBuffer = (byte *)malloc(height * width * 4);
}

void Vec3XEmulator::FastClearBuffer(Uint8 color) {
    memset(_pixelBuffer, color, _bufferWidth * _bufferHeight * 4);
}

void Vec3XEmulator::ClearBuffer(Uint8 color) {
    int offset = 0;

    for (int i = 0 ; i < _bufferWidth * _bufferHeight ; ++i) {
        _pixelBuffer[offset] = color;
        _pixelBuffer[offset+1] = color;
        _pixelBuffer[offset+2] = color;
        _pixelBuffer[offset+3] = 0;
        
        offset += 4;
    }

}

void Vec3XEmulator::SetPixel(int x, int y, Uint8 color) {
    int width = _bufferWidth;
    int height = _bufferHeight;
    
    // Is the pixel actually visible?
    if (x >= 0 && x < width && y >= 0 && y < height) {
        
        int offset = (x + ((height - 1) - y) * width) * 4;
        
        // Use this to make the origin top-left instead of bottom-right.
        offset = (x + y * width) * 4;
        
        _pixelBuffer[offset] = color;
        _pixelBuffer[offset+1] = color;
        _pixelBuffer[offset+2] = color;
        _pixelBuffer[offset+3] = 255;
    }
}

void Vec3XEmulator::DrawLine(int x1, int y1, int x2, int y2, Uint8 color)  {
    vectrex_add_line(x1, y1, x2, y2, color);
 
#ifdef USE_PIXEL_BUFFER
    int dx = x2 - x1;
    int dy = y2 - y1;
    
    // calculate steps required for generating pixels
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    
    // calculate increment in x & y for each steps
    float xInc = dx / (float) steps;
    float yInc = dy / (float) steps;
    
    float x = x1;
    float y = y1;
    for (int i = 0; i <= steps; i++) {
        SetPixel(x, y, color);
        
        x += xInc;
        y += yInc;
    }
#endif
}

void Vec3XEmulator::Render() {
#ifdef USE_PIXEL_BUFFER
    ClearBuffer(0);
#endif

    int v;
    for(v = 0; v < vector_draw_cnt; v++){
        Uint8 color = vectors_draw[v].color * 256 / VECTREX_COLORS;
        
        DrawLine((int)(_xOffset + vectors_draw[v].x0 / _scaling), (int)(_yOffset + vectors_draw[v].y0 / _scaling),
                 (int)(_xOffset + vectors_draw[v].x1 / _scaling), (int)(_yOffset + vectors_draw[v].y1 / _scaling), color);
    }
}

#pragma mark - Internal emulation

void Vec3XEmulator::Reset() {
    unsigned r;

    // RAM
    for (r = 0; r < 1024; r++) {
        _ram[r] = r & 0xff;
    }

    for (r = 0; r < 16; r++) {
        _soundRegisters[r] = 0;
        ic8910.Write(r, 0);
    }

    // input buttons
    _soundRegisters[14] = 0xff;
    ic8910.Write(14, 0xff);

    _soundSelect = 0;

    via_ora = 0;
    via_orb = 0;
    via_ddra = 0;
    via_ddrb = 0;
    via_t1on = 0;
    via_t1int = 0;
    via_t1c = 0;
    via_t1ll = 0;
    via_t1lh = 0;
    via_t1pb7 = 0x80;
    via_t2on = 0;
    via_t2int = 0;
    via_t2c = 0;
    via_t2ll = 0;
    via_sr = 0;
    via_srb = 8;
    via_src = 0;
    via_srclk = 0;
    via_acr = 0;
    via_pcr = 0;
    via_ifr = 0;
    via_ier = 0;
    via_ca2 = 1;
    via_cb2h = 1;
    via_cb2s = 0;

    alg_rsh = 128;
    alg_xsh = 128;
    alg_ysh = 128;
    alg_zsh = 0;
    alg_jch0 = 128;
    alg_jch1 = 128;
    alg_jch2 = 128;
    alg_jch3 = 128;
    alg_jsh = 128;

    alg_compare = 0;

    alg_dx = 0;
    alg_dy = 0;
    alg_curr_x = ALG_MAX_X / 2;
    alg_curr_y = ALG_MAX_Y / 2;

    alg_vectoring = 0;

    vector_draw_cnt = 0;
    vector_erse_cnt = 0;
    vectors_draw = vectors_set;
    vectors_erse = vectors_set + VECTOR_CNT;

    fcycles = FCYCLES_INIT;

    ic6809.Reset();
}

void Vec3XEmulator::Emulate(long cycles) {
    unsigned c, icycles;

    while (cycles > 0) {
        icycles = ic6809.Step(via_ifr & 0x80, 0);

        for (c = 0; c < icycles; c++) {
            ViaSstep0();
            AlgSstep();
            ViaSstep1();
        }

        cycles -= (long) icycles;

        fcycles -= (long) icycles;

        if (fcycles < 0) {
            vector_t *tmp;

            fcycles += FCYCLES_INIT;
            Render();

            // everything that was drawn during this pass now now enters
            vector_erse_cnt = vector_draw_cnt;
            vector_draw_cnt = 0;

            tmp = vectors_erse;
            vectors_erse = vectors_draw;
            vectors_draw = tmp;
        }
    }
}

#pragma mark - Load file

void Vec3XEmulator::LoadFile(const char* romfile, const char* romName, const char* cartfile, const char* cartName) {
    FILE *fp;
    char msg[255];
    errno_t error;

    error = fopen_s(&fp, romfile, "rb");
    if (error != 0) {
        platform_print("ERROR LOADING ROMFILE (1)");
        return;
    }

    if (fread(_rom, 1, sizeof (_rom), fp) != sizeof (_rom)) {
        platform_print("ERROR LOADING ROMFILE (2)");
        return;
    }
    
    fclose(fp);

    sprintf_s(msg, "Rom file loaded: %s", romName);
    platform_print(msg);
    
    memset(_cartridge, 0, sizeof (_cartridge));
    if (cartfile) {
        error = fopen_s(&fp, cartfile, "rb");
        if (error != 0) {
            platform_print("ERROR LOADING GAMEFILE (1)");
            return;
        }

        fread(_cartridge, 1, sizeof (_cartridge), fp);
        fclose(fp);

        sprintf_s(msg, "Cartridge file loaded: %s", cartName);
        platform_print(msg);
    }
}

#pragma mark - Screen resizing

void Vec3XEmulator::ResizeScreen(int width, int height) {
    long sclx, scly;

    long screenx = width;
    long screeny = height;
    
    CreateBuffer(screenx, screeny);

    sclx = ALG_MAX_X / width;
    scly = ALG_MAX_Y / height;

    _scaling = sclx > scly ? sclx : scly;

    _xOffset = (screenx - ALG_MAX_X / _scaling) / 2;
    _yOffset = (screeny - ALG_MAX_Y / _scaling) / 2;
}

#pragma mark - Key events

void Vec3XEmulator::Key(int vk, int pressed) {
    if (pressed) {
        switch (vk) {
            case PL1_LEFT:
                _soundRegisters[14] &= ~0x01;
                break;
            case PL1_RIGHT:
                _soundRegisters[14] &= ~0x02;
                break;
            case PL1_UP:
                _soundRegisters[14] &= ~0x04;
                break;
            case PL1_DOWN:
                _soundRegisters[14] &= ~0x08;
                break;
                
            case PL2_LEFT:
                alg_jch0 = 0x00;
                break;
            case PL2_RIGHT:
                alg_jch0 = 0xff;
                break;
            case PL2_UP:
                alg_jch1 = 0xff;
                break;
            case PL2_DOWN:
                alg_jch1 = 0x00;
                break;
        }
    }
    else {
        switch (vk) {
            case PL1_LEFT:
                _soundRegisters[14] |= 0x01;
                break;
            case PL1_RIGHT:
                _soundRegisters[14] |= 0x02;
                break;
            case PL1_UP:
                _soundRegisters[14] |= 0x04;
                break;
            case PL1_DOWN:
                _soundRegisters[14] |= 0x08;
                break;
                
            case PL2_LEFT:
                alg_jch0 = 0x80;
                break;
            case PL2_RIGHT:
                alg_jch0 = 0x80;
                break;
            case PL2_UP:
                alg_jch1 = 0x80;
                break;
            case PL2_DOWN:
                alg_jch1 = 0x80;
                break;
        }
    }
}

void Vec3XEmulator::Init(int width, int height) {
    ResizeScreen(width, height);
    _paused = false;
}

void Vec3XEmulator::Start(const char* romfile, const char* romName, const char* cartfile, const char* cartName) {
    LoadFile(romfile, romName, cartfile, cartName);

    ic8910.Start(&_soundRegisters[0]);
    audio_processor_start(&ic8910);

    Reset();
    
    _isInitialised = true;
}

void Vec3XEmulator::Frame() {
    if (!_isInitialised) {
        return;
    }
    
    if (_paused) {
        return;
    }

    Emulate((VECTREX_MHZ / 1000) * EMU_TIMER);
    vectrex_render_frame(_pixelBuffer);
    
    if (_liveUpdate) {
        vectrex_update_cpu_view(vectrex_get_register(VECTREX_PC), vectrex_get_register(VECTREX_USP), vectrex_get_register(VECTREX_HSP), vectrex_get_register(VECTREX_ACC_A), vectrex_get_register(VECTREX_ACC_B), vectrex_get_register(VECTREX_REG_X), vectrex_get_register(VECTREX_REG_Y), vectrex_get_register(VECTREX_REG_DP), vectrex_get_register(VECTREX_REG_CC), vector_draw_cnt);
    }
}

void Vec3XEmulator::Stop() {
    ic8910.Stop();
    audio_processor_stop();
    _isInitialised = 0;
}

void Vec3XEmulator::Pause(void) {
    _paused = 1;
}

void Vec3XEmulator::Resume(void) {
    _paused = 0;
}

void Vec3XEmulator::Command(int command, int parameter) {
    switch (command) {
        case DEBUG_LIVE_UPDATE:
            _liveUpdate = !_liveUpdate;
            break;
    }
}

#pragma mark - Internal bus handling

void Vec3XEmulator::SndUpdate() {
    switch (via_orb & 0x18) {
    case 0x00:
        /* the sound chip is disabled */
        break;
    case 0x08:
        /* the sound chip is sending data */
        break;
    case 0x10:
        /* the sound chip is recieving data */

        if (_soundSelect != 14) {
            _soundRegisters[_soundSelect] = via_ora;
            ic8910.Write(_soundSelect, via_ora);
        }

        break;
    case 0x18:
        /* the sound chip is latching an address */

        if ((via_ora & 0xf0) == 0x00) {
            _soundSelect = via_ora & 0x0f;
        }

        break;
    }
}

void Vec3XEmulator::AlgUpdate() {
    // update the various analog values when orb is written.
    switch (via_orb & 0x06) {
    case 0x00:
        alg_jsh = alg_jch0;

        if ((via_orb & 0x01) == 0x00) {
            /* demultiplexor is on */
            alg_ysh = alg_xsh;
        }

        break;
    case 0x02:
        alg_jsh = alg_jch1;

        if ((via_orb & 0x01) == 0x00) {
            /* demultiplexor is on */
            alg_rsh = alg_xsh;
        }

        break;
    case 0x04:
        alg_jsh = alg_jch2;

        if ((via_orb & 0x01) == 0x00) {
            /* demultiplexor is on */

            if (alg_xsh > 0x80) {
                alg_zsh = alg_xsh - 0x80;
            } else {
                alg_zsh = 0;
            }
        }

        break;
    case 0x06:
        /* sound output line */
        alg_jsh = alg_jch3;
        break;
    }

    /* compare the current joystick direction with a reference */

    if (alg_jsh > alg_xsh) {
        alg_compare = 0x20;
    } else {
        alg_compare = 0;
    }

    /* compute the new "deltas" */

    alg_dx = (long) alg_xsh - (long) alg_rsh;
    alg_dy = (long) alg_rsh - (long) alg_ysh;
}

void Vec3XEmulator::IntUpdate() {
    // update IRQ and bit-7 of the ifr register after making an adjustment to ifr
    if ((via_ifr & 0x7f) & (via_ier & 0x7f)) {
        via_ifr |= 0x80;
    }
    else {
        via_ifr &= 0x7f;
    }
}

unsigned char Vec3XEmulator::Read8(unsigned address) {
    unsigned char data = 0;

    if ((address & 0xe000) == 0xe000) {
        /* rom */

        data = _rom[address & 0x1fff];
    } else if ((address & 0xe000) == 0xc000) {
        if (address & 0x800) {
            /* ram */

            data = _ram[address & 0x3ff];
        } else if (address & 0x1000) {
            /* io */

            switch (address & 0xf) {
            case 0x0:
                /* compare signal is an input so the value does not come from
                 * via_orb.
                 */

                if (via_acr & 0x80) {
                    /* timer 1 has control of bit 7 */

                    data = (unsigned char) ((via_orb & 0x5f) | via_t1pb7 | alg_compare);
                } else {
                    /* bit 7 is being driven by via_orb */

                    data = (unsigned char) ((via_orb & 0xdf) | alg_compare);
                }

                break;
            case 0x1:
                /* register 1 also performs handshakes if necessary */

                if ((via_pcr & 0x0e) == 0x08) {
                    /* if ca2 is in pulse mode or handshake mode, then it
                     * goes low whenever ira is read.
                     */

                    via_ca2 = 0;
                }

                /* fall through */

            case 0xf:
                if ((via_orb & 0x18) == 0x08) {
                    /* the snd chip is driving port a */

                    data = (unsigned char) _soundRegisters[_soundSelect];
                } else {
                    data = (unsigned char) via_ora;
                }

                break;
            case 0x2:
                data = (unsigned char) via_ddrb;
                break;
            case 0x3:
                data = (unsigned char) via_ddra;
                break;
            case 0x4:
                /* T1 low order counter */

                data = (unsigned char) via_t1c;
                via_ifr &= 0xbf; /* remove timer 1 interrupt flag */

                via_t1on = 0; /* timer 1 is stopped */
                via_t1int = 0;
                via_t1pb7 = 0x80;

                IntUpdate ();

                break;
            case 0x5:
                /* T1 high order counter */

                data = (unsigned char) (via_t1c >> 8);

                break;
            case 0x6:
                /* T1 low order latch */

                data = (unsigned char) via_t1ll;
                break;
            case 0x7:
                /* T1 high order latch */

                data = (unsigned char) via_t1lh;
                break;
            case 0x8:
                /* T2 low order counter */

                data = (unsigned char) via_t2c;
                via_ifr &= 0xdf; /* remove timer 2 interrupt flag */

                via_t2on = 0; /* timer 2 is stopped */
                via_t2int = 0;

                IntUpdate ();

                break;
            case 0x9:
                /* T2 high order counter */

                data = (unsigned char) (via_t2c >> 8);
                break;
            case 0xa:
                data = (unsigned char) via_sr;
                via_ifr &= 0xfb; /* remove shift register interrupt flag */
                via_srb = 0;
                via_srclk = 1;

                IntUpdate ();

                break;
            case 0xb:
                data = (unsigned char) via_acr;
                break;
            case 0xc:
                data = (unsigned char) via_pcr;
                break;
            case 0xd:
                /* interrupt flag register */

                data = (unsigned char) via_ifr;
                break;
            case 0xe:
                /* interrupt enable register */

                data = (unsigned char) (via_ier | 0x80);
                break;
            }
        }
    } else if (address < 0x8000) {
        /* cartridge */

        data = _cartridge[address];
    } else {
        data = 0xff;
    }

    return data;
}

void Vec3XEmulator::Write8(unsigned address, unsigned char data) {
    if ((address & 0xe000) == 0xe000) {
        /* rom */
    } else if ((address & 0xe000) == 0xc000) {
        /* it is possible for both ram and io to be written at the same! */

        if (address & 0x800) {
            _ram[address & 0x3ff] = data;
        }

        if (address & 0x1000) {
            switch (address & 0xf) {
            case 0x0:
                via_orb = data;

                SndUpdate ();

                AlgUpdate ();

                if ((via_pcr & 0xe0) == 0x80) {
                    /* if cb2 is in pulse mode or handshake mode, then it
                     * goes low whenever orb is written.
                     */

                    via_cb2h = 0;
                }

                break;
            case 0x1:
                /* register 1 also performs handshakes if necessary */

                if ((via_pcr & 0x0e) == 0x08) {
                    /* if ca2 is in pulse mode or handshake mode, then it
                     * goes low whenever ora is written.
                     */

                    via_ca2 = 0;
                }

                /* fall through */

            case 0xf:
                via_ora = data;

                SndUpdate ();

                /* output of port a feeds directly into the dac which then
                 * feeds the x axis sample and hold.
                 */

                alg_xsh = data ^ 0x80;

                AlgUpdate ();

                break;
            case 0x2:
                via_ddrb = data;
                break;
            case 0x3:
                via_ddra = data;
                break;
            case 0x4:
                /* T1 low order counter */

                via_t1ll = data;

                break;
            case 0x5:
                /* T1 high order counter */

                via_t1lh = data;
                via_t1c = (via_t1lh << 8) | via_t1ll;
                via_ifr &= 0xbf; /* remove timer 1 interrupt flag */

                via_t1on = 1; /* timer 1 starts running */
                via_t1int = 1;
                via_t1pb7 = 0;

                IntUpdate ();

                break;
            case 0x6:
                /* T1 low order latch */

                via_t1ll = data;
                break;
            case 0x7:
                /* T1 high order latch */

                via_t1lh = data;
                break;
            case 0x8:
                /* T2 low order latch */

                via_t2ll = data;
                break;
            case 0x9:
                /* T2 high order latch/counter */

                via_t2c = (data << 8) | via_t2ll;
                via_ifr &= 0xdf;

                via_t2on = 1; /* timer 2 starts running */
                via_t2int = 1;

                IntUpdate ();

                break;
            case 0xa:
                via_sr = data;
                via_ifr &= 0xfb; /* remove shift register interrupt flag */
                via_srb = 0;
                via_srclk = 1;

                IntUpdate ();

                break;
            case 0xb:
                via_acr = data;
                break;
            case 0xc:
                via_pcr = data;


                if ((via_pcr & 0x0e) == 0x0c) {
                    /* ca2 is outputting low */

                    via_ca2 = 0;
                } else {
                    /* ca2 is disabled or in pulse mode or is
                     * outputting high.
                     */

                    via_ca2 = 1;
                }

                if ((via_pcr & 0xe0) == 0xc0) {
                    /* cb2 is outputting low */

                    via_cb2h = 0;
                } else {
                    /* cb2 is disabled or is in pulse mode or is
                     * outputting high.
                     */

                    via_cb2h = 1;
                }

                break;
            case 0xd:
                /* interrupt flag register */

                via_ifr &= ~(data & 0x7f);
                IntUpdate ();

                break;
            case 0xe:
                /* interrupt enable register */

                if (data & 0x80) {
                    via_ier |= data & 0x7f;
                } else {
                    via_ier &= ~(data & 0x7f);
                }

                IntUpdate ();

                break;
            }
        }
    } else if (address < 0x8000) {
        /* cartridge */
    }
}

void Vec3XEmulator::ViaSstep0() {
    // Perform a single cycle worth of via emulation via_sstep0 is the first postion of the emulation.
    unsigned t2shift;

    if (via_t1on) {
        via_t1c--;

        if ((via_t1c & 0xffff) == 0xffff) {
            /* counter just rolled over */

            if (via_acr & 0x40) {
                /* continuous interrupt mode */

                via_ifr |= 0x40;
                IntUpdate ();
                via_t1pb7 = 0x80 - via_t1pb7;

                /* reload counter */

                via_t1c = (via_t1lh << 8) | via_t1ll;
            } else {
                /* one shot mode */

                if (via_t1int) {
                    via_ifr |= 0x40;
                    IntUpdate ();
                    via_t1pb7 = 0x80;
                    via_t1int = 0;
                }
            }
        }
    }

    if (via_t2on && (via_acr & 0x20) == 0x00) {
        via_t2c--;

        if ((via_t2c & 0xffff) == 0xffff) {
            /* one shot mode */

            if (via_t2int) {
                via_ifr |= 0x20;
                IntUpdate ();
                via_t2int = 0;
            }
        }
    }

    /* shift counter */

    via_src--;

    if ((via_src & 0xff) == 0xff) {
        via_src = via_t2ll;

        if (via_srclk) {
            t2shift = 1;
            via_srclk = 0;
        } else {
            t2shift = 0;
            via_srclk = 1;
        }
    } else {
        t2shift = 0;
    }

    if (via_srb < 8) {
        switch (via_acr & 0x1c) {
        case 0x00:
            /* disabled */
            break;
        case 0x04:
            /* shift in under control of t2 */

            if (t2shift) {
                /* shifting in 0s since cb2 is always an output */

                via_sr <<= 1;
                via_srb++;
            }

            break;
        case 0x08:
            /* shift in under system clk control */

            via_sr <<= 1;
            via_srb++;

            break;
        case 0x0c:
            /* shift in under cb1 control */
            break;
        case 0x10:
            /* shift out under t2 control (free run) */

            if (t2shift) {
                via_cb2s = (via_sr >> 7) & 1;

                via_sr <<= 1;
                via_sr |= via_cb2s;
            }

            break;
        case 0x14:
            /* shift out under t2 control */

            if (t2shift) {
                via_cb2s = (via_sr >> 7) & 1;

                via_sr <<= 1;
                via_sr |= via_cb2s;
                via_srb++;
            }

            break;
        case 0x18:
            /* shift out under system clock control */

            via_cb2s = (via_sr >> 7) & 1;

            via_sr <<= 1;
            via_sr |= via_cb2s;
            via_srb++;

            break;
        case 0x1c:
            /* shift out under cb1 control */
            break;
        }

        if (via_srb == 8) {
            via_ifr |= 0x04;
            IntUpdate ();
        }
    }
}

void Vec3XEmulator::ViaSstep1() {
    // perform the second part of the via emulation
    if ((via_pcr & 0x0e) == 0x0a) {
        /* if ca2 is in pulse mode, then make sure
         * it gets restored to '1' after the pulse.
         */

        via_ca2 = 1;
    }

    if ((via_pcr & 0xe0) == 0xa0) {
        /* if cb2 is in pulse mode, then make sure
         * it gets restored to '1' after the pulse.
         */

        via_cb2h = 1;
    }
}

void Vec3XEmulator::AlgAddline(long x0, long y0, long x1, long y1, unsigned char color) {
    unsigned long key;
    long index;

    key = (unsigned long) x0;
    key = key * 31 + (unsigned long) y0;
    key = key * 31 + (unsigned long) x1;
    key = key * 31 + (unsigned long) y1;
    key %= VECTOR_HASH;

    /* first check if the line to be drawn is in the current draw list.
     * if it is, then it is not added again.
     */

    index = vector_hash[key];

    if (index >= 0 && index < vector_draw_cnt &&
        x0 == vectors_draw[index].x0 &&
        y0 == vectors_draw[index].y0 &&
        x1 == vectors_draw[index].x1 &&
        y1 == vectors_draw[index].y1) {
        vectors_draw[index].color = color;
    } else {
        /* missed on the draw list, now check if the line to be drawn is in
         * the erase list ... if it is, "invalidate" it on the erase list.
         */

        if (index >= 0 && index < vector_erse_cnt &&
            x0 == vectors_erse[index].x0 &&
            y0 == vectors_erse[index].y0 &&
            x1 == vectors_erse[index].x1 &&
            y1 == vectors_erse[index].y1) {
            vectors_erse[index].color = VECTREX_COLORS;
        }

        vectors_draw[vector_draw_cnt].x0 = x0;
        vectors_draw[vector_draw_cnt].y0 = y0;
        vectors_draw[vector_draw_cnt].x1 = x1;
        vectors_draw[vector_draw_cnt].y1 = y1;
        vectors_draw[vector_draw_cnt].color = color;
        vector_hash[key] = vector_draw_cnt;
        vector_draw_cnt++;
    }
}

void Vec3XEmulator::AlgSstep() {
    // perform a single cycle worth of analog emulation
    long sig_dx, sig_dy;
    unsigned sig_ramp;
    unsigned sig_blank;

    if ((via_acr & 0x10) == 0x10) {
        sig_blank = via_cb2s;
    } else {
        sig_blank = via_cb2h;
    }

    if (via_ca2 == 0) {
        /* need to force the current point to the 'orgin' so just
         * calculate distance to origin and use that as dx,dy.
         */

        sig_dx = ALG_MAX_X / 2 - alg_curr_x;
        sig_dy = ALG_MAX_Y / 2 - alg_curr_y;
    } else {
        if (via_acr & 0x80) {
            sig_ramp = via_t1pb7;
        } else {
            sig_ramp = via_orb & 0x80;
        }

        if (sig_ramp == 0) {
            sig_dx = alg_dx;
            sig_dy = alg_dy;
        } else {
            sig_dx = 0;
            sig_dy = 0;
        }
    }

    if (alg_vectoring == 0) {
        if (sig_blank == 1 &&
            alg_curr_x >= 0 && alg_curr_x < ALG_MAX_X &&
            alg_curr_y >= 0 && alg_curr_y < ALG_MAX_Y) {

            /* start a new vector */

            alg_vectoring = 1;
            alg_vector_x0 = alg_curr_x;
            alg_vector_y0 = alg_curr_y;
            alg_vector_x1 = alg_curr_x;
            alg_vector_y1 = alg_curr_y;
            alg_vector_dx = sig_dx;
            alg_vector_dy = sig_dy;
            alg_vector_color = (unsigned char) alg_zsh;
        }
    } else {
        /* already drawing a vector ... check if we need to turn it off */

        if (sig_blank == 0) {
            /* blank just went on, vectoring turns off, and we've got a
             * new line.
             */

            alg_vectoring = 0;

            AlgAddline (alg_vector_x0, alg_vector_y0,
                         alg_vector_x1, alg_vector_y1,
                         alg_vector_color);
        } else if (sig_dx != alg_vector_dx ||
                   sig_dy != alg_vector_dy ||
                   (unsigned char) alg_zsh != alg_vector_color) {

            /* the parameters of the vectoring processing has changed.
             * so end the current line.
             */

            AlgAddline (alg_vector_x0, alg_vector_y0,
                         alg_vector_x1, alg_vector_y1,
                         alg_vector_color);

            /* we continue vectoring with a new set of parameters if the
             * current point is not out of limits.
             */

            if (alg_curr_x >= 0 && alg_curr_x < ALG_MAX_X &&
                alg_curr_y >= 0 && alg_curr_y < ALG_MAX_Y) {
                alg_vector_x0 = alg_curr_x;
                alg_vector_y0 = alg_curr_y;
                alg_vector_x1 = alg_curr_x;
                alg_vector_y1 = alg_curr_y;
                alg_vector_dx = sig_dx;
                alg_vector_dy = sig_dy;
                alg_vector_color = (unsigned char) alg_zsh;
            } else {
                alg_vectoring = 0;
            }
        }
    }

    alg_curr_x += sig_dx;
    alg_curr_y += sig_dy;

    if (alg_vectoring == 1 &&
        alg_curr_x >= 0 && alg_curr_x < ALG_MAX_X &&
        alg_curr_y >= 0 && alg_curr_y < ALG_MAX_Y) {

        /* we're vectoring ... current point is still within limits so
         * extend the current vector.
         */

        alg_vector_x1 = alg_curr_x;
        alg_vector_y1 = alg_curr_y;
    }
}

#pragma mark - C-Bridging

extern "C" {

static Vec3XEmulator* vec3x = nullptr;

void vectrex_emulator_init(int width, int height) {
    if (vec3x == nullptr) {
        vec3x = new Vec3XEmulator();
    }
    
    vec3x->Init(width, height);
}

void vectrex_emulator_start(const char* romfile, const char* romName, const char* cartfile, const char* cartName) {
    vec3x->Start(romfile, romName, cartfile, cartName);
}

void vectrex_emulator_frame() {
    vec3x->Frame();
}

void vectrex_emulator_stop() {
    vec3x->Stop();
    delete vec3x;
    vec3x = nullptr;
}

void vectrex_emulator_pause(void) {
    vec3x->Pause();
}

void vectrex_emulator_resume(void) {
    vec3x->Resume();
}

void vectrex_emulator_key(int vk, int pressed) {
    vec3x->Key(vk, pressed);
}

void vectrex_emulator_debug_command(int command, int parameter) {
    vec3x->Command(command, parameter);
}

unsigned vectrex_get_register(int reg) {
    return vec3x->GetCPU().GetRegister(reg);
}

}
