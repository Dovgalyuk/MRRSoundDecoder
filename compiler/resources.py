from dataclasses import dataclass
from utils import *

resources = None
resources_map = {}

@dataclass
class Resource:
    name: str
    path: str
    volume: int = 128
    num: int = 0
    _used: bool = False

    def save(self, f):
        if self.num == 0:
            return
        type = self.path[-3:]
        w = open(self.path, 'rb')
        if type == 'wav':
            # read header
            header = w.read(44)
            samplerate = int.from_bytes(header[0x18:0x1c], "little")
            bits = int.from_bytes(header[0x22:0x23], "little")
            channels = int.from_bytes(header[0x16:0x18], "little")
            length = int.from_bytes(header[0x4:0x8], "little") - (44 - 8)
            if samplerate not in [15625, 31250]:
                raise RRException(f'Unsupported samplerate {samplerate} for {self.path}')
            if bits not in [8, 16]:
                raise RRException(f'Unsupported bits per sample ({bits}) for {self.path}')
            if channels != 1:
                raise RRException(f'Unsupported number of channels ({channels}) for {self.path}')
            write_byte(f, 0x03) #wave
            write_word(f, self.num)
            write_dword(f, length)
            write_word(f, samplerate)
            write_byte(f, bits)
            f.write(w.read(length))
        elif type == 'bmp':
            pass
        else:
            raise RRException(f'Unsupported resource type {self.path}')
