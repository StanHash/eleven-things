    @ this has something to do with PACA files
    @ I think NDS reverse LZSS more or less (except size addend is 3 instead of 2?)

    @ INPUT:
    @ R0 = addr
    @ R1 = size
    @ R2 = ???? some other size?

fun_00502038:
    STMFD   SP!, {R4-R9}

    ADD     R0, R0, R1

    LDR     R3, [R0,#-8]
    BIC     R12, R3, #0xFF000000

    CMP     R12, R1
    BHI     end

    @ ????
    @ according some doc I am reading, if I understand it correctly, this is R5 = WORD[R0 - 0x04] ROR 8
    @ with syscmn.pack, this is 000A7138 ROR 8
    LDR     R5, [R0,#-3]
    LDRB    R4, [R0,#-4]
    LDRB    R3, [R0,#-5]
    ORR     R4, R4, R5,LSL#8

    @ R4 = ((WORD[R0 - 0x04] ROR 8) LSL 8) | BYTE[R0 - 0x04]
    @ which is just WORD[R0 - 0x04] truncated to 24bits... I think?

    ADD     R1, R1, R4

    CMP     R1, R2
    BHI     end

    SUB     R7, R0, R12 @ r7 = lower bound of packed area?
    SUB     R3, R0, R3  @ r3 = upper bound of packed area?
    CMP     R7, R3
    ADD     R12, R0, R4 @ r12 = end of unpacked area?
    MOVCC   R8, #3
    BCS     end

lop:
    LDRB    R5, [R3,#-1]!
    MOV     R6, #0

lop_bits:
    TST     R5, #0x80
    BEQ     put_byte

    @ this is like GBA LZSS!
    @ even more like NDS LZSS!

    LDRB    R0, [R3,#-1]
    LDRB    R1, [R3,#-2]!
    AND     R2, R0, #0xF
    ADDS    R4, R8, R0,LSR#4 @ r4 = len = ((hi & 0xF0) >> 4) + 3
    ORR     R0, R1, R2,LSL#8 @ r0 = off = lo | ((hi & 0x0F) << 8)
    ADD     R0, R0, #3       @ r0 = off = off + 3
    ADD     R1, R12, R0
    BEQ     continue_bits

    TST     R4, #1
    MOV     R0, R12
    BEQ     loc_005020CC
    LDRB    R2, [R1,#-1]!
    SUB     R0, R12, #1
    STRB    R2, [R12,#-1]

loc_005020CC:
    MOVS    R2, R4,LSR#1
    BEQ     loc_005020EC

loc_005020D4:
    LDRB    R9, [R1,#-1]
    SUBS    R2, R2, #1
    STRB    R9, [R0,#-1]
    LDRB    R9, [R1,#-2]!
    STRB    R9, [R0,#-2]!
    BNE     loc_005020D4

loc_005020EC:
    SUB     R12, R12, R4

continue_bits:
    @ r5 = r5 << 1
    MOV     R0, R5,LSL#25
    CMP     R3, R7
    MOV     R5, R0,LSR#24
    BLS     continue
    ADD     R6, R6, #1
    CMP     R6, #8
    BLT     lop_bits

continue:
    CMP     R7, R3
    BCC     lop

end:
    LDMFD   SP!, {R4-R9}
    BX      LR

put_byte:
    LDRB    R0, [R3,#-1]!
    STRB    R0, [R12,#-1]!
    B       continue_bits
