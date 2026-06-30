from utils import RRException
from bytecode import *
from project import *

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
        self._used = False

    def get_name(self):
        return self._name

    def is_function(self):
        return self._name in ['_exit', '_immediate', '_on_exit', '_exit_processor']

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
    
    def get_level(self):
        if self._parent:
            return self._parent.get_level() + 1
        return 0

    def set_used(self):
        self._used = True

    def is_used(self):
        return self._used

    def add_instruction(self, instr):
        self._instructions.append(instr)

    def taint_control_flow(self):
        changed = False
        if self._instructions and not self._instructions[0].is_used():
            self._instructions[0].set_used()
            changed = True

        init = self.get_state('_init')
        if not init and self._substates:
            # for the main context
            init = self._substates[0]
        if init and not init.is_used():
            init.set_used()
            changed = True

        s = self.get_state('_immediate')
        if s and not s.is_used():
            s.set_used()
            changed = True

        s = self.get_state('_exit_processor')
        if s and not s.is_used():
            s.set_used()
            changed = True

        s = self.get_state('_exit')
        if s and not s.is_used():
            s.set_used()
            changed = True

        while True:
            ch = False
            for i in range(len(self._instructions) - 1):
                cur = self._instructions[i]
                next = self._instructions[i + 1]
                if cur.is_used() and cur.is_passthrough() and not next.is_used():
                    next.set_used()
                    ch = True
            for s in self._substates:
                if s.is_used():
                    ch = s.taint_control_flow() or ch
            changed = changed or ch
            if not ch:
                break
        return changed

    def optimize_control_flow(self):
        # peephole optimization
        while True:
            # delete dead branches at first pass
            self._instructions = [i for i in self._instructions if i.is_used()]
            changed = False
            for i in range(len(self._instructions) - 1):
                cur = self._instructions[i]
                next = self._instructions[i + 1]
                match cur:
                    case IrJump(_label=lab, _flag='') if lab == next:
                        cur.set_unused()
                        changed = True
            if not changed:
                break

        self._substates = [s for s in self._substates if s.is_used()]
        for s in self._substates:
            s.optimize_control_flow()

    def optimize_jumps(self):
        labels = {}
        for i in range(len(self._instructions) - 1):
            cur = self._instructions[i]
            next = self._instructions[i + 1]
            if type(cur) == IrLabel and type(next) == IrJump and next._flag == '':
                labels[cur] = next._label

        for cur in self._instructions:
            if type(cur) == IrJump and cur._label in labels:
                cur._label = labels[cur._label]

        for s in self._substates:
            s.optimize_jumps()

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

    def process_deps(self):
        changed = False
        # IrLoad check own slot (C_SLOT)
        # IrInc/IrDec control other slots (C_SLOT)
        # IrLoad use keys (C_KEY)
        # IrInc/IrDec control keys (C_KEY)
        # IrInc/IrDec control physical outputs (C_OUT)
        for instr in self._instructions:
            var = instr.get_var()
            if var and var.is_cond():
                x = var.get_key()
                if x is not None and not function_keys_map[x].is_used():
                    function_keys_map[x].set_used()
                    changed = True
                x = var.get_slot()
                if x is not None and not slots_map[x].is_used():
                    slots_map[x].set_used()
                    changed = True
                x = var.get_out()
                if x is not None and x in outputs_map and not outputs_map[x].is_used():
                    outputs_map[x].set_used()
                    changed = True
        for s in self._substates:
            changed = s.process_deps() or changed
        return changed

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
