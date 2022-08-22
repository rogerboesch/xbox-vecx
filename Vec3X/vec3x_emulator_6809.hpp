
#pragma once

#include <stdio.h>

class Vec3XEmulator;

class Vec3XEmulator6809 {
public:
    Vec3XEmulator6809(Vec3XEmulator* emulator);
    
public:
    void Reset();
    unsigned Step(unsigned irq_i, unsigned irq_f);

public:
    unsigned GetRegister(int reg);
    
private:
    Vec3XEmulator* vectrex;
    
private:
    unsigned read8(unsigned address);
    void write8(unsigned address, unsigned data);
    unsigned read16(unsigned address);
    void write16(unsigned address, unsigned data);

    unsigned get_cc(unsigned flag);
    void set_cc(unsigned flag, unsigned value);
    unsigned test_c(unsigned i0, unsigned i1, unsigned r, unsigned sub);
    unsigned test_n(unsigned r);
    unsigned test_z8(unsigned r);
    unsigned test_z16(unsigned r);
    unsigned test_v(unsigned i0, unsigned i1, unsigned r);
    unsigned get_reg_d(void);
    void set_reg_d(unsigned value);
    void push8(unsigned *sp, unsigned data);
    unsigned pull8(unsigned *sp);
    void push16(unsigned *sp, unsigned data);
    unsigned pull16(unsigned *sp);
    unsigned pc_read8(void);
    unsigned pc_read16(void);
    unsigned sign_extend(unsigned data);
    unsigned ea_direct(void);
    unsigned ea_extended(void);
    unsigned ea_indexed(unsigned *cycles);
    unsigned inst_neg(unsigned data);
    unsigned inst_com(unsigned data);
    unsigned inst_lsr(unsigned data);
    unsigned inst_ror(unsigned data);
    unsigned inst_asr(unsigned data);
    unsigned inst_asl(unsigned data);
    unsigned inst_rol(unsigned data);
    unsigned inst_dec(unsigned data);
    unsigned inst_inc(unsigned data);
    void inst_tst8(unsigned data);
    void inst_tst16(unsigned data);
    void inst_clr(void);
    unsigned inst_sub8(unsigned data0, unsigned data1);
    unsigned inst_sbc(unsigned data0, unsigned data1);
    unsigned inst_and(unsigned data0, unsigned data1);
    unsigned inst_eor(unsigned data0, unsigned data1);
    unsigned inst_adc(unsigned data0, unsigned data1);
    unsigned inst_or(unsigned data0, unsigned data1);
    unsigned inst_add8(unsigned data0, unsigned data1);
    unsigned inst_add16(unsigned data0, unsigned data1);
    unsigned inst_sub16(unsigned data0, unsigned data1);
    void inst_bra8(unsigned test, unsigned op, unsigned *cycles);
    void inst_bra16(unsigned test, unsigned op, unsigned *cycles);
    void inst_psh(unsigned op, unsigned *sp, unsigned data, unsigned *cycles);
    void inst_pul(unsigned op, unsigned *sp, unsigned *osp, unsigned *cycles);
    unsigned exgtfr_read(unsigned reg);
    void exgtfr_write(unsigned reg, unsigned data);
    void inst_exg(void);
    void inst_tfr(void);
    
private:
    unsigned reg_x;      // index registers
    unsigned reg_y;
    unsigned reg_u;      // stack pointer
    unsigned reg_s;      // hardware stack pointer
    unsigned reg_pc;     // program counter
    unsigned reg_a;      // accumulators
    unsigned reg_b;
    unsigned reg_dp;     // direct page register
    unsigned reg_cc;     // condition codes
    unsigned irq_status; // flag to see if interrupts should be handled (sync/cwait)
    
    unsigned *rptr_xyus[4] = {0, 0, 0, 0};
};
