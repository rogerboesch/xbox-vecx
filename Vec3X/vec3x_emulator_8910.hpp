
#pragma once

#include "vec3x_emulator_types.hpp"

typedef struct _AY8910 {
    int32_t index;
    int32_t ready;
    uint32_t *Regs;
    int32_t lastEnable;
    int32_t PeriodA,PeriodB,PeriodC,PeriodN,PeriodE;
    int32_t CountA,CountB,CountC,CountN,CountE;
    uint32_t VolA,VolB,VolC,VolE;
    uint8_t EnvelopeA,EnvelopeB,EnvelopeC;
    uint8_t OutputA,OutputB,OutputC,OutputN;
    int8_t CountEnv;
    uint8_t Hold,Alternate,Attack,Holding;
    int32_t RNG;
    uint32_t VolTable[32];
} AY8910;

class Vec3XEmulator8910 {
public:
    Vec3XEmulator8910();
    
public:
    void Start(uint32_t* soundRegisters);
    void Stop();
    void Write(int r, int v);

    void GetSoundBufferData(Uint8 *stream, int length);

private:
    void BuildMixerTable();

private:
    AY8910 PSG;
};
