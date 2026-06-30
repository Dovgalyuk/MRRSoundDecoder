from utils import RRException
from resources import Resource

class IrInstruction():
    _used: bool = False

    def __str__(self):
        pass

    def size(self):
        return 0

    def bytes(self):
        return RRException(f'bytecode generation not implemented for {self}')

    def set_address(self, addr):
        return

    def get_address(self):
        raise RRException(f'get_address not implemented for {self}')

    def replace_labels(self, address):
        pass

    def is_used(self):
        return self._used

    def set_used(self):
        self._used = True

    def set_unused(self):
        self._used = False

    def is_passthrough(self):
        return True

    def get_var(self):
        return None

class IrComment(IrInstruction):
    _comment = None

    def __init__(self, comment):
        super().__init__()
        self._comment = comment

    def __str__(self):
        return f'# {self._comment}'

    def bytes(self):
        return b''

class IrLabel(IrInstruction):
    _address: int = 0

    def __str__(self):
        return f'Label_{hex(id(self))} # {self._address}'

    def size(self):
        return 0

    def bytes(self):
        return b''

    def set_address(self, addr):
        self._address = addr

    def get_address(self):
        return self._address

class IrNop(IrInstruction):
    def __str__(self):
        return '  nop'

    def size(self):
        return 1

    def bytes(self):
        return b'\x00'

class IrSwitch(IrInstruction):
    def __str__(self):
        return '  switch'

    def size(self):
        return 1

    def bytes(self):
        return b'\x01'

class IrRet(IrInstruction):
    def __str__(self):
        return '  ret'

    def size(self):
        return 1

    def bytes(self):
        return b'\x02'

    def is_passthrough(self):
        return False

class IrRand(IrInstruction):
    def __str__(self):
        return '  rand'

    def size(self):
        return 1

    def bytes(self):
        return b'\x03'

class IrAdd(IrInstruction):
    def __str__(self):
        return '  add'

    def size(self):
        return 1

    def bytes(self):
        return b'\x04'

class IrSub(IrInstruction):
    def __str__(self):
        return '  sub'

    def size(self):
        return 1

    def bytes(self):
        return b'\x05'

class IrDelay(IrInstruction):
    def __str__(self):
        return '  delay'

    def size(self):
        return 1

    def bytes(self):
        return b'\x06'

class IrPlay(IrInstruction):
    def __str__(self):
        return f'  play'

    def size(self):
        return 1

    def bytes(self):
        return b'\x07'

class IrJump(IrInstruction):
    _label: IrLabel = None
    _addr: int = None
    _flag: str = None

    def __init__(self, label, flag=''):
        super().__init__()
        self._label = label
        self._flag = flag

    def __str__(self):
        return f'  jump{self._flag} {self._label}'

    def size(self):
        return 3

    def replace_labels(self, address):
        self._addr = self._label.get_address() - address

    def bytes(self):
        match self._flag:
            case '':
                b = 0x08
            case 't':
                b = 0x09
            case 'f':
                b = 0x0a
        return int(b).to_bytes() + int(self._addr).to_bytes(2, 'little', signed=True)

    def set_used(self):
        super().set_used()
        self._label.set_used()

    def is_passthrough(self):
        return self._flag != ''

class IrLoadI(IrInstruction):
    # could be address, integer, variable or state reference
    _value = None
    _addr: int = None

    def __init__(self, value):
        super().__init__()
        self._value = value

    def __str__(self):
        return f'  loadi {self._value}'

    def size(self):
        return 4

    def replace_labels(self, address):
        match self._value:
            case int():
                self._addr = self._value
            case True:
                self._addr = 1
            case False:
                self._addr = 1
            case Resource(num=num):
                self._addr = num
            case _:
                self._addr = self._value.get_address()

    def bytes(self):
        return b'\x0b' + int(self._addr).to_bytes(3, 'little', signed=True)

    def set_used(self):
        super().set_used()
        if type(self._value) != int and type(self._value) != bool:
            self._value.set_used()

class IrLoad(IrInstruction):
    _var = None
    _addr: int = None

    def __init__(self, var):
        super().__init__()
        self._var = var

    def __str__(self):
        return f'  load {self._var}'

    def size(self):
        return 2

    def replace_labels(self, address):
        self._addr = self._var.get_address()

    def bytes(self):
        return b'\x0c' + int(self._addr).to_bytes()

    def get_var(self):
        return self._var

class IrStore(IrInstruction):
    _var = None
    _addr: int = None

    def __init__(self, var):
        super().__init__()
        self._var = var

    def __str__(self):
        return f'  store {self._var}'

    def size(self):
        return 2

    def replace_labels(self, address):
        self._addr = self._var.get_address()

    def bytes(self):
        return b'\x0d' + int(self._addr).to_bytes()

    def get_var(self):
        return self._var

class IrCall(IrInstruction):
    # _ref = None
    # _addr: int = None

    # def __init__(self, ref):
    #     super().__init__()
    #     self._ref = ref

    def __str__(self):
        return f'  call'# {self._ref}'

    def size(self):
        return 1#4

    # def replace_labels(self, address):
    #     self._addr = self._ref.get_address()

    def bytes(self):
        return b'\x0e'# + int(self._addr).to_bytes(3, 'little')

    # def set_used(self):
    #     super().set_used()
    #     self._ref.set_used()

class IrDec(IrInstruction):
    _var = None
    _addr: int = None

    def __init__(self, var):
        super().__init__()
        self._var = var

    def __str__(self):
        return f'  dec {self._var}'

    def size(self):
        return 2

    def replace_labels(self, address):
        self._addr = self._var.get_address()

    def bytes(self):
        return b'\x0f' + int(self._addr).to_bytes()

    def get_var(self):
        return self._var

class IrNext(IrInstruction):
    _count: int

    def __init__(self, count=1):
        super().__init__()
        self._count = count
        if count not in [1, 2, 4, 8]:
            raise RRException(f'Invalid number of cylinders in next: {count}')

    def __str__(self):
        return f'  next {self._count}'

    def size(self):
        return 1

    def bytes(self):
        return int(0x10 + self._count - 1).to_bytes()

    def is_passthrough(self):
        return False

class IrSwap(IrInstruction):
    def __str__(self):
        return f'  swap'

    def size(self):
        return 1

    def bytes(self):
        return b'\x18'

class IrSet(IrInstruction):
    _var = None
    _bit = None

    def __init__(self, var, bit):
        super().__init__()
        self._var = var
        self._bit = bit

    def __str__(self):
        return f'  set{self._bit} {self._var}'

    def size(self):
        return 2

    def bytes(self):
        code = 0x20 + self._bit
        var = self._var.get_address()
        return int.to_bytes(code) + int.to_bytes(var)

    def get_var(self):
        return self._var

class IrReset(IrInstruction):
    _var = None
    _bit = None

    def __init__(self, var, bit):
        super().__init__()
        self._var = var
        self._bit = bit

    def __str__(self):
        return f'  reset{self._bit} {self._var}'

    def size(self):
        return 2

    def bytes(self):
        code = 0x28 + self._bit
        var = self._var.get_address()
        return int.to_bytes(code) + int.to_bytes(var)

    def get_var(self):
        return self._var

class IrTest(IrInstruction):
    _var = None
    _bit = None

    def __init__(self, var, bit):
        super().__init__()
        self._var = var
        self._bit = bit

    def __str__(self):
        return f'  test{self._bit} {self._var}'

    def size(self):
        return 2

    def bytes(self):
        code = 0x30 + self._bit
        var = self._var.get_address()
        return int.to_bytes(code) + int.to_bytes(var)

class IrCond(IrInstruction):
    _cond = None

    def __init__(self, cond):
        super().__init__()
        self._cond = cond

    def __str__(self):
        return f'  cond{self._cond}'

    def size(self):
        return 1

    def bytes(self):
        code = 0x38 + {'eq': 0, 'ne': 1, 'gt': 2, 'ge': 3, 'lt': 4, 'le': 5}[self._cond]
        return int.to_bytes(code)

class IrInc(IrInstruction):
    _var = None
    _addr: int = None

    def __init__(self, var):
        super().__init__()
        self._var = var

    def __str__(self):
        return f'  inc {self._var}'

    def size(self):
        return 2

    def replace_labels(self, address):
        self._addr = self._var.get_address()

    def bytes(self):
        return b'\x3e' + int(self._addr).to_bytes()

    def get_var(self):
        return self._var
