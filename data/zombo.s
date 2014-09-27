.syntax unified
.cpu cortex-m4
.fpu softvfp
.thumb

.global ZomboFileDataStart
.global ZomboFileDataEnd

.align 2 /* align to 4 byte boundary */
ZomboFileDataStart:
    .incbin "data/zombo.raw"
.align 2
ZomboFileDataEnd:
    .word 0xBABE
