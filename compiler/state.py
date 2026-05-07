from utils import RRException
from bytecode import *

class State:
    _name = None
    _variables = None
    _substates = None
    _parent = None
    _instructions = None
    _entry: IrLabel = None
    _address: int = None
    # name to state map
    _staterefs = None

    def __init__(self, name, parent=None):
        self._name = name
        self._variables = {}
        self._parent = parent
        self._substates = []
        self._staterefs = {}
        self._entry = IrLabel()
        self._instructions = [ self._entry ]

    def get_name(self):
        return self._name

    def is_function(self):
        return self._name in ['_exit', '_immediate']

    def set_var(self, name, value):
        self._variables[name] = value

    def get_var(self, name):
        if name in self._variables:
            return self._variables[name]
        if self._parent:
            return self._parent.get_var(name)
        return None

    def add_state(self, state):
        self._substates.append(state)
        if state._name in self._staterefs:
            raise RRException(f'State {state._name} already exists in {self}')
        self._staterefs[state._name] = state

    def get_state(self, name):
        if name not in self._staterefs:
            return None
        return self._staterefs[name]

    def get_parent(self):
        return self._parent

    def add_instruction(self, instr):
        self._instructions.append(instr)

    def calculate_addresses(self, address=0):
        self._address = address
        for instr in self._instructions:
            instr.set_address(address)
            address += instr.size()
        for s in self._substates:
            address = s.calculate_addresses(address)
        return address

    def replace_labels(self):
        address = self._address
        for instr in self._instructions:
            instr.replace_labels(address)
            address += instr.size()
        for s in self._substates:
            s.replace_labels()

    def dump(self, f, depth=0, address=0):
        tabs = '  ' * depth
        state = 'Function' if self.is_function() else 'State'
        f.write(f'       {tabs}# {state} {self._name}\n')
        for instr in self._instructions:
            b = instr.bytes()
            bs = ' '.join([f'{i:02x}' for i in b])
            f.write(f'{address:6} {bs:16}{tabs}{instr}\n')
            address += instr.size()
        for s in self._substates:
            address = s.dump(f, depth + 1, address)
        return address

    def write_bytecode(self, f):
        for instr in self._instructions:
            f.write(instr.bytes())
        for s in self._substates:
            s.write_bytecode(f)

    def get_address(self):
        return self._address

    def __str__(self):
        prefix = ''
        if self._parent:
            prefix = str(self._parent) + ':'
        return prefix + self._name

global_context = State('')
