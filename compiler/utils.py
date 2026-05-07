
class RRException(Exception):
    def __init__(self, message):
        super().__init__(message)

############################################################
# File utilities
############################################################

def write_byte(f, i):
    f.write(int.to_bytes(i, 1, 'little'))

def write_word(f, i):
    f.write(int.to_bytes(i, 2, 'little'))

def write_dword(f, i):
    f.write(int.to_bytes(i, 4, 'little'))

def write_byte_array(f, a):
    write_byte(f, len(a))
    for b in a:
        write_byte(f, b)

def write_string(f, s):
    if s is None:
        f.write(b'\x00\x00')
    else:
        buf = s.encode('utf-8')
        write_word(f, len(buf))
        f.write(buf)
