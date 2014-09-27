/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h> 
#include <math.h> 
#include "stm32f4xx.h" 
#include "leds.h" 
#include "main.h" 
#include "dac_lowlevel.h" 
#include "midi_lowlevel.h"

/* mmmidi includes */
#include "mm_midimsgbuilder.h"
#include "mm_midirouter_standard.h" 

/* mm_dsp includes */
#include "mm_bus.h"
#include "mm_sample.h"
#include "mm_sampleplayer.h"
#include "mm_sigchain.h"
#include "mm_sigproc.h"
#include "mm_wavtab.h"
#include "mm_sigconst.h"

#define MIDI_BOTTOM_NOTE 24
#define MIDI_TOP_NOTE    96 
#define MIDI_NUM_NOTES   (MIDI_TOP_NOTE - MIDI_BOTTOM_NOTE)

extern uint16_t *curDMAData;

extern MMSample GrandPianoFileDataStart;
extern MMSample GrandPianoFileDataEnd;

static MIDIMsgBuilder_State_t lastState;
static MIDIMsgBuilder midiMsgBuilder;
static MIDI_Router_Standard midiRouter;

MMSamplePlayerSigProc *spsps[MIDI_NUM_NOTES];

void MIDI_note_on_do(void *data, MIDIMsg *msg)
{
    if ((msg->data[1] < MIDI_TOP_NOTE) && (msg->data[1] >= MIDI_BOTTOM_NOTE)) {
        MMSamplePlayerSigProc *sp = 
            ((MMSamplePlayerSigProc**)data)[msg->data[1] - MIDI_BOTTOM_NOTE];
        ((MMSigProc*)sp)->state = MMSigProc_State_PLAYING;
        ((MMSamplePlayerSigProc*)sp)->index = 0;
        ((MMSamplePlayerSigProc*)sp)->rate = pow(2.,
            ((msg->data[1] - 59) / 12.));
    }
    MIDIMsg_free(msg);
}

void MIDI_note_off_do(void *data, MIDIMsg *msg)
{
    if ((msg->data[1] < MIDI_TOP_NOTE) && (msg->data[1] >= MIDI_BOTTOM_NOTE)) {
        MMSamplePlayerSigProc *sp = 
            ((MMSamplePlayerSigProc**)data)[msg->data[1] - MIDI_BOTTOM_NOTE];
        ((MMSigProc*)sp)->state = MMSigProc_State_DONE;
    }
    MIDIMsg_free(msg);
}

int main(void)
{
    MMSample *sampleFileDataStart = &GrandPianoFileDataStart;
    MMSample *sampleFileDataEnd   = &GrandPianoFileDataEnd;
    size_t i;


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

    /* Enable MIDI hardware */
    MIDI_low_level_setup();

    /* Initialize MIDI Message builder */
    MIDIMsgBuilder_init(&midiMsgBuilder);

    /* set up the MIDI router to trigger samples */
    MIDI_Router_Standard_init(&midiRouter);
    for (i = 0; i < MIDI_NUM_NOTES; i++) {
        spsps[i] = MMSamplePlayerSigProc_new();
        MMSamplePlayerSigProc_init(spsps[i]);
        spsps[i]->samples = &samples;
        spsps[i]->parent = &samplePlayer;
        spsps[i]->loop = 1;
        ((MMSigProc*)spsps[i])->state = MMSigProc_State_DONE;
        /* insert in signal chain */
        MMSigProc_insertAfter(&samplePlayer.placeHolder, spsps[i]);
        MIDI_Router_addCB(&midiRouter.router, MIDIMSG_NOTE_ON, 1, MIDI_note_on_do, spsps);
        MIDI_Router_addCB(&midiRouter.router, MIDIMSG_NOTE_OFF, 1, MIDI_note_off_do, spsps);
    }

    while (1) {
        while (curDMAData == NULL);/* wait for request to fill with data */
        MIDI_process_buffer(); /* process MIDI at most every audio block */
        size_t numIters = DAC_DMA_BUF_SIZE;
        while (numIters--) {
            MMSigProc_tick(&sigChain);
            *(curDMAData++) = ((uint16_t)(0xfff * (*outBus + (1.))) >> 2);
        }
        curDMAData = NULL;
    }
}

void do_stuff_with_msg(MIDIMsg *msg)
{
    MIDI_Router_handleMsg(&midiRouter.router, msg);
}

void MIDI_process_byte(char byte)
{
    switch (MIDIMsgBuilder_update(&midiMsgBuilder,byte)) {
        case MIDIMsgBuilder_State_WAIT_STATUS:
            break;
        case MIDIMsgBuilder_State_WAIT_DATA:
            break;
        case MIDIMsgBuilder_State_COMPLETE:
            do_stuff_with_msg(midiMsgBuilder.msg);
            MIDIMsgBuilder_init(&midiMsgBuilder); /* reset builder */
            break;
    }
    lastState = midiMsgBuilder.state;
}
