#!/usr/bin/python3

from dataclasses import dataclass
import sys
import os
import json
import ast
import itertools
from state import global_context
from slot import Slot, SlotInfo
from utils import *
from resources import *
from variables import SlotVariable
from project import *

@dataclass
class FunctionKey:
    num: int
    name: str
    _used: bool = False

    def save(self, f):
        if not self._used:
            return
        write_byte(f, 0x06)
        write_byte(f, self.num)
        write_string(f, self.name)

    def is_used(self):
        return self._used

    def set_used(self):
        self._used = True

@dataclass
class PhysicalOutput:
    id: int
    name: str
    mode: str
    delay_on: int
    delay_off: int
    timeout: int
    attrib: dict
    real_output: int
    _used: bool = False

    def save(self, f):
        if not self._used:
            return
        write_byte(f, 0x07)
        write_byte(f, self.real_output)
        var = SlotVariable(f'C_OUT{self.id}')
        write_byte(f, var.get_address())
        write_byte(f, self.delay_on)
        write_byte(f, self.delay_off)

    def is_used(self):
        return self._used

    def set_used(self):
        self._used = True

@dataclass
class Locomotive:
    type: str
    name: str
    description: str
    icon: str

    def save(self, f):
        write_byte(f, 0x01)
        # Type not needed yet
        write_byte(f, 0x00)
        write_string(f, self.name)
        write_string(f, self.description)

scv = {}

###################################################
# script parsers
###################################################

def parse_config(tree):
    for op in tree.body:
        match op:
            case ast.Assign(targets=[ast.Name(id=name)], value=ast.Constant(value=value)):
                if 'V_SOUNDCV' in name:
                    #define CV_SOUND1 155
                    scv[155 + int(name[9:]) - 1] = int(value)
                else:
                    global_context.set_var(name, value)
            case _:
                raise RRException(f'Invalid operation in config: {ast.dump(op)}')


###################################################
# main
###################################################

if len(sys.argv) < 3:
    print(f'Usage: {sys.argv[0]} <project directory> <output directory>')
    sys.exit(0)

project_dir = sys.argv[1]
output_dir = sys.argv[2]
os.makedirs(output_dir, exist_ok=True)

output_name = os.path.join(output_dir, 'sound.prj')

# load all data
with open(os.path.join(project_dir, 'locomotive.json'), 'r') as f:
    locomotive = Locomotive(**json.load(f))

# with open(os.path.join(project_dir, 'functions.cfg'), 'r') as f:
#     tree = ast.parse(f.read())
#     functions = [Function(**eval(ast.unparse(d))) for d in tree.body]

with open(os.path.join(project_dir, 'function_keys.cfg'), 'r') as f:
    tree = ast.parse(f.read())
    for t in tree.body:
        k = FunctionKey(**eval(ast.unparse(t)))
        function_keys.append(k)
        function_keys_map[k.num] = k

with open(os.path.join(project_dir, 'slots.cfg'), 'r') as f:
    tree = ast.parse(f.read())
    for t in tree.body:
        slot = Slot(SlotInfo(**eval(ast.unparse(t))))
        slots.append(slot)
        slots_map[slot._num] = slot

with open(os.path.join(project_dir, 'resources.json'), 'r') as f:
    resources = [Resource(**d) for d in json.load(f)]
    for r in resources:
        r.path = os.path.join(project_dir, 'resources', r.path)
        resources_map[r.name] = r

with open(os.path.join(project_dir, 'outputs.cfg'), 'r') as f:
    tree = ast.parse(f.read())
    for t in tree.body:
        p = PhysicalOutput(**eval(ast.unparse(t)))
        outputs.append(p)
        outputs_map[p.id] = p

# load config
with open(os.path.join(project_dir, 'project.cfg'), 'r') as f:
    parse_config(ast.parse(f.read()))

# check unused entities
for s in slots:
    if s._info.enable in ['F_EXECUTING', 'F_RANDOM', 'F_BRAKING']:
        s._used = True

# TODO: find unused function keys and physical outputs?

# load slots
for s in slots:
    with open(os.path.join(project_dir, f'{s._num}.slot'), 'r') as f:
        s.parse(ast.parse(f.read()))

# compile slots
for s in slots:
    s.compile()

# find dependencies:
while True:
    changed = False
    for s in slots:
        if s.is_used():
            changed = s.process_deps() or changed
    if not changed:
        break

# renumber resources
rn = 1
for r in resources:
    if r._used:
        r.num = rn
        rn += 1

# substitute addresses and id's into bytecode
for s in slots:
    s.finalize()

# write temporary files
for s in slots:
    if not s._used:
        continue
    with open(os.path.join(output_dir, f'{s._num}.asm'), 'w') as f:
        s.dump(f)

# write compiled project
output_file = open(output_name, 'wb')

output_file.write(b'MRRD')
write_byte(output_file, 0x12) #version

locomotive.save(output_file)

for f in function_keys:
    f.save(output_file)

for s in slots:
    s.save(output_file)

for r in resources:
    r.save(output_file)

for p in outputs:
    p.save(output_file)

if scv:
    write_byte(output_file, 5)
    write_word(output_file, len(scv))
    for cv, v in scv.items():
        write_word(output_file, cv)
        write_byte(output_file, v)

# TODO: physical output props
