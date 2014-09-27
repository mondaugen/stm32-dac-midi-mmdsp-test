.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.global GrandPianoFileDataStart
.global GrandPianoFileDataEnd

.align 2 /* align to 4 byte boundary */
GrandPianoFileDataStart:
    .incbin "data/grand_piano_59.raw"
.align 2
GrandPianoFileDataEnd:
    .word 0xBABE
