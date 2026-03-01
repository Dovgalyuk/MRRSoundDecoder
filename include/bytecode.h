#ifndef BYTECODE_H
#define BYTECODE_H

#define W(x) ((uint16_t)(x)) % 0x100, ((uint16_t)(x)) / 0x100
#define D(x) W(((uint32_t)(x)) % 0x10000), W(((uint32_t)(x)) / 0x10000)

#define C_EQ    0x00
#define C_NE    0x01
#define C_GT    0x02
#define C_GE    0x03
#define C_LT    0x04
#define C_LE    0x05

#define I_TEST0     0x00
#define I_TEST1     (I_TEST0 + 1)
#define I_TEST2     (I_TEST0 + 2)
#define I_TEST3     (I_TEST0 + 3)
#define I_TEST4     (I_TEST0 + 4)
#define I_TEST5     (I_TEST0 + 5)
#define I_TEST6     (I_TEST0 + 6)
#define I_TEST7     (I_TEST0 + 7)
#define I_LOADV     0x08
#define I_LOADI     0x09
#define I_JUMP      0x0a
#define I_JUMPF     0x0b
#define I_JUMPT     0x0c
#define I_COND      0x0d
#define I_CONDEQ    (I_COND + C_EQ)
#define I_CONDNE    (I_COND + C_NE)
#define I_CONDGT    (I_COND + C_GT)
#define I_CONDGE    (I_COND + C_GE)
#define I_CONDLT    (I_COND + C_LT)
#define I_CONDLE    (I_COND + C_LE)
#define I_NEXT      0x13
#define I_WAIT      0x14
#define I_PLAY      0x15
#define I_FUNC      0x16
#define I_STOREV    0x17
#define I_SET0      0x18
#define I_SET1      (I_SET0 + 1)
#define I_SET2      (I_SET0 + 2)
#define I_SET3      (I_SET0 + 3)
#define I_SET4      (I_SET0 + 4)
#define I_SET5      (I_SET0 + 5)
#define I_SET6      (I_SET0 + 6)
#define I_SET7      (I_SET0 + 7)
#define I_RESET0    0x20
#define I_RESET1    (I_RESET0 + 1)
#define I_RESET2    (I_RESET0 + 2)
#define I_RESET3    (I_RESET0 + 3)
#define I_RESET4    (I_RESET0 + 4)
#define I_RESET5    (I_RESET0 + 5)
#define I_RESET6    (I_RESET0 + 6)
#define I_RESET7    (I_RESET0 + 7)
#define I_ADD       0x28
#define I_SUB       0x29
#define I_SWITCH    0x2a
#define I_RET       0x2b

#define FUNC_RAND   0x00

#endif
