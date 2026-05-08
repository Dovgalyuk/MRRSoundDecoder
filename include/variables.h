#ifndef VARIABLES_H
#define VARIABLES_H

#define VAR_LOCAL_START 0x00
/* Local flags and variables */
#define F_FUNCTION      0x00
#define F_PLAYING       0x01
#define V_TIMER_1_256MS 0x02
#define V_TIMER_2_256MS 0x03
#define V_USER_1        0x04
#define V_USER_2        0x05
#define V_USER_3        0x06
#define V_USER_4        0x07
/* Set to prevent movement while playing */
#define F_DRIVELOCK     0x08
/* Restore sound when re-enabling the slot in case of priority conflict */
//#define F_RESTORE       0x09
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
#define V_SELECT        0x4e /* Affects volume */
#define V_SHARE1        0x4f
#define V_SHARE2        0x50
#define F_DRIVING       0x51
#define F_DISABLE_BRAKE 0x52
#define F_LOAD1         0x53
#define F_LOAD2         0x54
#define V_CYLINDER      0x55

#define F_KEY0          0x60
#define F_KEY1          0x61
#define F_KEY2          0x62
#define F_KEY3          0x63
#define F_KEY4          0x64
#define F_KEY5          0x65
#define F_KEY6          0x66
#define F_KEY7          0x67
#define F_KEY8          0x68
#define F_KEY9          0x69
#define F_KEY10         0x6a
#define F_KEY11         0x6b
#define F_KEY12         0x6c
#define F_KEY13         0x6d
#define F_KEY14         0x6e
#define F_KEY15         0x6f
#define F_KEY16         0x70
#define F_KEY17         0x71
#define F_KEY18         0x72
#define F_KEY19         0x73
#define F_KEY20         0x74
#define F_KEY21         0x75
#define F_KEY22         0x76
#define F_KEY23         0x77
#define F_KEY24         0x78
#define F_KEY25         0x79
#define F_KEY26         0x7a
#define F_KEY27         0x7b
#define F_KEY28         0x7c
#define F_KEY29         0x7d
#define F_KEY30         0x7e
#define F_KEY31         0x7f

/* Reserved addresses 0x80-0xa0 for 0x20 outputs */
#define PHYS_OUTPUTS       7
#define PHYS_OUTPUT0    0x80
#define PHYS_OUTPUT1    0x81
#define PHYS_OUTPUT2    0x82
#define PHYS_OUTPUT3    0x83
#define PHYS_OUTPUT4    0x84
#define PHYS_OUTPUT5    0x85
#define PHYS_OUTPUT6    0x86

#define VAR_GLOBAL_SIGNED_START 0xE0
/* Signed global variables */
#define V_ACCEL         0xE0

#define VAR_END                 0x100
#define VAR_GLOBAL_SIZE         (VAR_END - VAR_GLOBAL_START)

#endif
