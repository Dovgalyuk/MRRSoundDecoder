#ifndef VARIABLES_H
#define VARIABLES_H

// Out of the address spaces, always read as 0
#define F_NULL          0x00

#define VAR_LOCAL_START 0x01
/* Local flags and variables */
#define F_PLAYING       0x01
#define V_TIMER_1_256MS 0x02
#define V_TIMER_2_256MS 0x03
#define V_USER_1        0x04
#define V_USER_2        0x05
#define V_USER_3        0x06
#define V_USER_4        0x07
#define R_ACCUM         0x08
#define R_ACCUM2        0x09
/* Shadow playing flag for switching between two sounds */
#define F_PLAYING2      0x0a
#define V_TIMER_1S      0x0b
#define VAR_LOCAL_SIZE  0x10

#define VAR_GLOBAL_START 0x40
/* Global flags and variables */
#define F_REVERSE       0x40
#define F_BRAKE1        0x41
#define F_BRAKE2        0x42
#define F_BRAKE3        0x43
#define F_TRIGGER       0x44
#define F_SHIFT1        0x45
#define F_SHIFT2        0x46
#define F_SHIFT3        0x47
#define F_SHIFT4        0x48
#define F_SHIFT5        0x49
#define F_SHIFT6        0x4a
#define V_SPEED         0x4b
#define V_SPEED_REQUEST 0x4c
#define V_SPEED_CURRENT 0x4d
//#define V_SELECT        0x4e /* Affects volume */
#define V_SHARE1        0x4f
#define V_SHARE2        0x50
#define F_DRIVING       0x51
#define F_DISABLE_BRAKE 0x52
#define F_LOAD1         0x53
#define F_LOAD2         0x54
#define V_CYLINDER      0x55
#define C_SWITCHING     0x56
#define C_DISABLE_ACCEL 0x57
#define C_SMOKE         0x58
#define F_BRAKING       0x59
#define F_EXECUTING     0x5a
#define F_RANDOM        0x5b
// dimmer - reduce brightness by 60%
#define C_DIMMER        0x5c

#define C_KEY0          0x60
#define C_KEY1          0x61
#define C_KEY2          0x62
#define C_KEY3          0x63
#define C_KEY4          0x64
#define C_KEY5          0x65
#define C_KEY6          0x66
#define C_KEY7          0x67
#define C_KEY8          0x68
#define C_KEY9          0x69
#define C_KEY10         0x6a
#define C_KEY11         0x6b
#define C_KEY12         0x6c
#define C_KEY13         0x6d
#define C_KEY14         0x6e
#define C_KEY15         0x6f
#define C_KEY16         0x70
#define C_KEY17         0x71
#define C_KEY18         0x72
#define C_KEY19         0x73
#define C_KEY20         0x74
#define C_KEY21         0x75
#define C_KEY22         0x76
#define C_KEY23         0x77
#define C_KEY24         0x78
#define C_KEY25         0x79
#define C_KEY26         0x7a
#define C_KEY27         0x7b
#define C_KEY28         0x7c
#define C_KEY29         0x7d
#define C_KEY30         0x7e
#define C_KEY31         0x7f

#define C_SLOT1         0x80
#define C_SLOT2         0x81
#define C_SLOT3         0x82
#define C_SLOT4         0x83
#define C_SLOT5         0x84
#define C_SLOT6         0x85
#define C_SLOT7         0x86
#define C_SLOT8         0x87
#define C_SLOT9         0x88
#define C_SLOT10        0x89
#define C_SLOT11        0x8a
#define C_SLOT12        0x8b
#define C_SLOT13        0x8c
#define C_SLOT14        0x8d
#define C_SLOT15        0x8e
#define C_SLOT16        0x8f
#define C_SLOT17        0x90
#define C_SLOT18        0x91
#define C_SLOT19        0x92
#define C_SLOT20        0x93
#define C_SLOT21        0x94
#define C_SLOT22        0x95
#define C_SLOT23        0x96
#define C_SLOT24        0x97
#define C_SLOT25        0x98
#define C_SLOT26        0x99
#define C_SLOT27        0x9a
#define C_SLOT28        0x9b
#define C_SLOT29        0x9c
#define C_SLOT30        0x9d
#define C_SLOT31        0x9e
#define C_SLOT32        0x9f
#define C_SLOT33        0xa0
#define C_SLOT34        0xa1
#define C_SLOT35        0xa2
#define C_SLOT36        0xa3
#define C_SLOT37        0xa4
#define C_SLOT38        0xa5
#define C_SLOT39        0xa6
#define C_SLOT40        0xa7
#define C_SLOT41        0xa8
#define C_SLOT42        0xa9
#define C_SLOT43        0xaa
#define C_SLOT44        0xab
#define C_SLOT45        0xac
#define C_SLOT46        0xad
#define C_SLOT47        0xae
#define C_SLOT48        0xaf
/* Motor speed is locked: can't start or change speed */
#define C_DRIVELOCK     0xb0

/* Reserved addresses 0xd0-0xdf for 0x10 outputs */
#define LOG_OUTPUTS     0x10
#define C_OUT1          0xd0
#define C_OUT2          0xd1
#define C_OUT3          0xd2
#define C_OUT4          0xd3
#define C_OUT5          0xd4
#define C_OUT6          0xd5
#define C_OUT7          0xd6
#define C_OUT8          0xd7
#define C_OUT9          0xd8
#define C_OUT10         0xd9
#define C_OUT11         0xda
#define C_OUT12         0xdb
#define C_OUT13         0xdc
#define C_OUT14         0xdd
#define C_OUT15         0xde
#define C_OUT16         0xdf

#define VAR_GLOBAL_SIGNED_START 0xF0
/* Signed global variables */
#define V_ACCEL         0xF0

#define VAR_END                 0x100
#define VAR_GLOBAL_SIZE         (VAR_END - VAR_GLOBAL_START)

#endif
