/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h> 
#include <math.h> 
#include "stm32f4xx.h" 
#include "leds.h" 
#include "main.h" 
#include "dac_lowlevel.h" 

/* mm_dsp includes */
#include "mm_bus.h"
#include "mm_sample.h"
#include "mm_sampleplayer.h"
#include "mm_sigchain.h"
#include "mm_sigproc.h"
#include "mm_wavtab.h"
#include "mm_sigconst.h"

#define NUM_SAMPLE_PLAYER_SIG_PROCS 3 

extern uint16_t *curDMAData;
extern MMSample GrandPianoFileDataStart;
extern MMSample GrandPianoFileDataEnd;

MMSamplePlayerSigProc *spsps[NUM_SAMPLE_PLAYER_SIG_PROCS];
float spNotes[] = {49.,53.,56.};

int main(void)
{
    MMSample *sampleFileDataStart = &GrandPianoFileDataStart;
    MMSample *sampleFileDataEnd   = &GrandPianoFileDataEnd;
    /* Enable LEDs so we can toggle them */
    LEDs_Init();

    curDMAData = NULL;
    
    /* Enable DAC */
    DAC_Setup();

    /* Sample to write data to */
    MMSample sample;

    /* The bus the signal chain is reading/writing */
    MMBus outBus = &sample;

    /* a signal chain to put the signal processors into */
    MMSigChain sigChain;
    MMSigChain_init(&sigChain);

    /* A constant that zeros the bus each iteration */
    MMSigConst sigConst;
    MMSigConst_init(&sigConst);
    MMSigConst_setOutBus(&sigConst,outBus);

    /* A sample player */
    MMSamplePlayer samplePlayer;
    MMSamplePlayer_init(&samplePlayer);
    samplePlayer.outBus = outBus;
    /* puts its place holder at the top of the sig chain */
    MMSigProc_insertAfter(&sigChain.sigProcs, &samplePlayer.placeHolder);

    /* put sig constant at the top of the sig chain */
    MMSigProc_insertBefore(&samplePlayer.placeHolder,&sigConst);

    /* Give access to samples of sound as wave table */
    MMWavTab samples;
    samples.data = sampleFileDataStart;
    samples.length = sampleFileDataEnd - sampleFileDataStart;

    /* make a samplePlayerSigProc */
    size_t i;
    for (i = 0; i < NUM_SAMPLE_PLAYER_SIG_PROCS; i++) {
        spsps[i] = MMSamplePlayerSigProc_new();
        MMSamplePlayerSigProc_init(spsps[i]);
        spsps[i]->samples = &samples;
        spsps[i]->rate = pow(2,(spNotes[i] - 59)/12.0);
        spsps[i]->parent = &samplePlayer;
        spsps[i]->loop = 1;
        /* insert in signal chain */
        MMSigProc_insertAfter(&samplePlayer.placeHolder, spsps[i]);
    }

    while (1) {
        while (curDMAData == NULL);/* wait for request to fill with data */
        size_t numIters = DAC_DMA_BUF_SIZE;
        while (numIters--) {
            MMSigProc_tick(&sigChain);
            *(curDMAData++) = ((uint16_t)(0xfff * (*outBus + (1.))) >> 2);
        }
        curDMAData = NULL;
    }
}

void MIDI_process_byte(char byte)
{
    return; /* do nothing for now */
}
