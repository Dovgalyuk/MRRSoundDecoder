from utils import RRException

slot_variables = {
    'F_FUNCTION':      0x00,
    'F_PLAYING':       0x01,
    'V_TIMER_1_256MS': 0x02,
    'V_TIMER_2_256MS': 0x03,
    'V_USER_1':        0x04,
    'V_USER_2':        0x05,
    'V_USER_3':        0x06,
    'V_USER_4':        0x07,
    'F_DRIVELOCK':     0x08,
    'F_RESTORE':       0x09,
    'V_TIMER_1S':      0x0b,
    'F_REVERSE':       0x40,
    'F_BRAKE1':        0x41,
    'F_BRAKE2':        0x42,
    'F_BRAKE3':        0x43,
    'F_TRIGGER':       0x44,
    'F_SHIFT1':        0x45,
    'F_SHIFT2':        0x46,
    'F_SHIFT3':        0x47,
    'F_SHIFT4':        0x48,
    'F_SHIFT5':        0x49,
    'F_SHIFT6':        0x4a,
    'V_SPEED':         0x4b,
    'V_SPEED_REQUEST': 0x4c,
    'V_SPEED_CURRENT': 0x4d,
    'V_SELECT':        0x4e,
    'V_SHARE1':        0x4f,
    'V_SHARE2':        0x50,
    'F_DISABLE_BRAKE': 0x52,
    'F_LOAD1':         0x53,
    'F_LOAD2':         0x54,
    'V_ACCEL':         0xE0,

    'F_DRIVING':       0x51,

    'F_KEY0':          0x60,
    'F_KEY1':          0x61,
    'F_KEY2':          0x62,
    'F_KEY3':          0x63,
    'F_KEY4':          0x64,
    'F_KEY5':          0x65,
    'F_KEY6':          0x66,
    'F_KEY7':          0x67,
    'F_KEY8':          0x68,
    'F_KEY9':          0x69,
    'F_KEY10':         0x6a,
    'F_KEY11':         0x6b,
    'F_KEY12':         0x6c,
    'F_KEY13':         0x6d,
    'F_KEY14':         0x6e,
    'F_KEY15':         0x6f,
    'F_KEY16':         0x70,
    'F_KEY17':         0x71,
    'F_KEY18':         0x72,
    'F_KEY19':         0x73,
    'F_KEY20':         0x74,
    'F_KEY21':         0x75,
    'F_KEY22':         0x76,
    'F_KEY23':         0x77,
    'F_KEY24':         0x78,
    'F_KEY25':         0x79,
    'F_KEY26':         0x7a,
    'F_KEY27':         0x7b,
    'F_KEY28':         0x7c,
    'F_KEY29':         0x7d,
    'F_KEY30':         0x7e,
    'F_KEY31':         0x7f,
}

class SlotVariable():
    _name: str = None
    _bit: int = None

    def __init__(self, name):
        if name not in slot_variables:
            raise RRException(f'Unknown variable {name}')
        self._name = name

    def __str__(self):
        return self._name

    def get_address(self):
        return slot_variables[self._name]

    def get_key(self):
        if self._name[:5] == 'F_KEY':
            return int(self._name[5:])
        return None