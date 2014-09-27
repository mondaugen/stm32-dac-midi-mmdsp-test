.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.global SoundFileDataStart
.global SoundFileDataEnd

.align 2 /* align to 4 byte boundary */
SoundFileDataStart:
    .incbin "data/win-low.raw"
.align 2
SoundFileDataEnd:
    .word 0xBEEF
