from utils import RRException

slot_variables = {
    'F_NULL':          0x00,
    'F_PLAYING':       0x01,
    'V_TIMER_1_256MS': 0x02,
    'V_TIMER_2_256MS': 0x03,
    'V_USER_1':        0x04,
    'V_USER_2':        0x05,
    'V_USER_3':        0x06,
    'V_USER_4':        0x07,
    'R_ACCUM':         0x08,
    'R_ACCUM2':        0x09,
    'V_TIMER_1S':      0x0b,

    'F_REVERSE':       0x40,
    'C_BRAKE1':        0x41,
    'C_BRAKE2':        0x42,
    'C_BRAKE3':        0x43,
    'F_TRIGGER':       0x44,
    'C_SHIFT1':        0x45,
    'C_SHIFT2':        0x46,
    'C_SHIFT3':        0x47,
    'C_SHIFT4':        0x48,
    'C_SHIFT5':        0x49,
    'C_SHIFT6':        0x4a,
    'V_SPEED':         0x4b,
    'V_SPEED_REQUEST': 0x4c,
    'V_SPEED_CURRENT': 0x4d,
    #'V_SELECT':        0x4e,
    'V_SHARE1':        0x4f,
    'V_SHARE2':        0x50,
    'F_DRIVING':       0x51,
    'F_DISABLE_BRAKE': 0x52,
    'C_LOAD1':         0x53,
    'C_LOAD2':         0x54,
    'V_CYLINDER':      0x55,
    'C_SWITCHING':     0x56,
    'C_DISABLE_ACCEL': 0x57,
    'C_SMOKE':         0x58,
    'F_BRAKING':       0x59,
    'F_EXECUTING':     0x5a,
    'F_RANDOM':        0x5b,

    'V_ACCEL':         0xF0,

    'C_KEY0':          0x60,
    'C_KEY1':          0x61,
    'C_KEY2':          0x62,
    'C_KEY3':          0x63,
    'C_KEY4':          0x64,
    'C_KEY5':          0x65,
    'C_KEY6':          0x66,
    'C_KEY7':          0x67,
    'C_KEY8':          0x68,
    'C_KEY9':          0x69,
    'C_KEY10':         0x6a,
    'C_KEY11':         0x6b,
    'C_KEY12':         0x6c,
    'C_KEY13':         0x6d,
    'C_KEY14':         0x6e,
    'C_KEY15':         0x6f,
    'C_KEY16':         0x70,
    'C_KEY17':         0x71,
    'C_KEY18':         0x72,
    'C_KEY19':         0x73,
    'C_KEY20':         0x74,
    'C_KEY21':         0x75,
    'C_KEY22':         0x76,
    'C_KEY23':         0x77,
    'C_KEY24':         0x78,
    'C_KEY25':         0x79,
    'C_KEY26':         0x7a,
    'C_KEY27':         0x7b,
    'C_KEY28':         0x7c,
    'C_KEY29':         0x7d,
    'C_KEY30':         0x7e,
    'C_KEY31':         0x7f,

    'C_SLOT1':         0x80,
    'C_SLOT2':         0x81,
    'C_SLOT3':         0x82,
    'C_SLOT4':         0x83,
    'C_SLOT5':         0x84,
    'C_SLOT6':         0x85,
    'C_SLOT7':         0x86,
    'C_SLOT8':         0x87,
    'C_SLOT9':         0x88,
    'C_SLOT10':        0x89,
    'C_SLOT11':        0x8a,
    'C_SLOT12':        0x8b,
    'C_SLOT13':        0x8c,
    'C_SLOT14':        0x8d,
    'C_SLOT15':        0x8e,
    'C_SLOT16':        0x8f,
    'C_SLOT17':        0x90,
    'C_SLOT18':        0x91,
    'C_SLOT19':        0x92,
    'C_SLOT20':        0x93,
    'C_SLOT21':        0x94,
    'C_SLOT22':        0x95,
    'C_SLOT23':        0x96,
    'C_SLOT24':        0x97,
    'C_SLOT25':        0x98,
    'C_SLOT26':        0x99,
    'C_SLOT27':        0x9a,
    'C_SLOT28':        0x9b,
    'C_SLOT29':        0x9c,
    'C_SLOT30':        0x9d,
    'C_SLOT31':        0x9e,
    'C_SLOT32':        0x9f,
    'C_SLOT33':        0xa0,
    'C_SLOT34':        0xa1,
    'C_SLOT35':        0xa2,
    'C_SLOT36':        0xa3,
    'C_SLOT37':        0xa4,
    'C_SLOT38':        0xa5,
    'C_SLOT39':        0xa6,
    'C_SLOT40':        0xa7,
    'C_SLOT41':        0xa8,
    'C_SLOT42':        0xa9,
    'C_SLOT43':        0xaa,
    'C_SLOT44':        0xab,
    'C_SLOT45':        0xac,
    'C_SLOT46':        0xad,
    'C_SLOT47':        0xae,
    'C_SLOT48':        0xaf,
    'C_DRIVELOCK':     0xb0,

    'C_OUT1':          0xd0,
    'C_OUT2':          0xd1,
    'C_OUT3':          0xd2,
    'C_OUT4':          0xd3,
    'C_OUT5':          0xd4,
    'C_OUT6':          0xd5,
    'C_OUT7':          0xd6,
    'C_OUT8':          0xd7,
    'C_OUT9':          0xd8,
    'C_OUT10':         0xd9,
    'C_OUT11':         0xda,
    'C_OUT12':         0xdb,
    'C_OUT13':         0xdc,
    'C_OUT14':         0xdd,
    'C_OUT15':         0xde,
    'C_OUT16':         0xdf,
}


# def cond_to_var(c):
#     if c == 'drive_lock':
#         return 'C_DRIVELOCK'
#     elif c[:4] == 'slot':
#         return 'C_SLOT' + c[4:]
#     raise RRException(f'Unknown condition {c}')

class SlotVariable():
    _name: str = None

    def __init__(self, name, slot=None):
        if name == 'F_FUNCTION':
            if not slot:
                raise RRException(f'Unknown slot while reading F_FUNCTION')
            name = f'C_SLOT{slot}'
        if name not in slot_variables:
            raise RRException(f'Unknown variable {name}')
        self._name = name

    def __str__(self):
        return self._name

    def get_address(self):
        return slot_variables[self._name]

    def get_key(self):
        if self._name[:5] == 'C_KEY':
            return int(self._name[5:])
        return None

    def get_slot(self):
        if self._name[:6] == 'C_SLOT':
            return int(self._name[6:])
        return None

    def get_out(self):
        if self._name[:5] == 'C_OUT':
            return int(self._name[5:])
        return None

    def is_cond(self):
        return self._name[:2] == 'C_'
