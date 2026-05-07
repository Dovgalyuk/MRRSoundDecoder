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

@dataclass
class Function:
    num: int
    inputs: list[str | SlotVariable]
    not_inputs: list[str | SlotVariable]
    logic: list[str | SlotVariable]
    slots: list[int]
    phy: list[int]
    _used: bool = True

    def compile(self):
        self.inputs = [SlotVariable(v) for v in self.inputs]
        self.not_inputs = [SlotVariable(v) for v in self.not_inputs]
        self.logic = [SlotVariable(v) for v in self.logic]

    def save(self, f):
        if not self._used:
            return
        write_byte(f, 0x04)
        write_byte_array(f, [v.get_address() for v in self.inputs])
        write_byte_array(f, [v.get_address() for v in self.not_inputs])
        write_byte_array(f, [v.get_address() for v in self.logic])
        write_byte_array(f, self.slots)
        write_byte_array(f, self.phy)

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


###################################################
# script parsers
###################################################

def parse_config(tree):
    for op in tree.body:
        match op:
            case ast.Assign(targets=[ast.Name(id=name)], value=ast.Constant(value=value)):
                global_context.set_var(name, value)
            case _:
                raise RRException(f'Invalid operation in config: {ast.dump(op)}')

###################################################
# project_data
###################################################

locomotive = None
functions = None
function_keys = None
function_keys_map = {}
slots = None
slots_map = {}

###################################################
# main
###################################################

if len(sys.argv) < 3:
    print(f'Usage: {sys.argv[0]} <project directory> <output directory>')
    sys.exit(0)

project_dir = sys.argv[1]
output_dir = sys.argv[2]
os.makedirs(output_dir, exist_ok=True)

output_name = os.path.join(output_dir, 'project.bin')

# load all data
with open(os.path.join(project_dir, 'locomotive.json'), 'r') as f:
    locomotive = Locomotive(**json.load(f))

with open(os.path.join(project_dir, 'functions.json'), 'r') as f:
    functions = [Function(**d) for d in json.load(f)]

with open(os.path.join(project_dir, 'function_keys.json'), 'r') as f:
    function_keys = [FunctionKey(**d) for d in json.load(f)]
    for f in function_keys:
        function_keys_map[f.num] = f

with open(os.path.join(project_dir, 'slots.json'), 'r') as f:
    slots = [Slot(SlotInfo(**d)) for d in json.load(f)]
    for s in slots:
        slots_map[s._num] = s

with open(os.path.join(project_dir, 'resources.json'), 'r') as f:
    resources = [Resource(**d) for d in json.load(f)]
    for r in resources:
        r.path = os.path.join(project_dir, 'resources', r.path)
        resources_map[r.name] = r

# load config
with open(os.path.join(project_dir, 'project.cfg'), 'r') as f:
    context = parse_config(ast.parse(f.read()))

# check unused entities
for f in functions:
    if not f._used:
        continue
    f.compile()
    for i in itertools.chain(f.inputs, f.not_inputs):
        k = i.get_key()
        if k is not None:
            function_keys_map[k]._used = True
    for s in f.slots:
        slot = slots_map[s]
        slot._used = True

for s in slots:
    if 'force' in s._info.flags:
        s._used = True

# load slots
for s in slots:
    if not s._used:
        continue
    with open(os.path.join(project_dir, f'{s._num}.slot'), 'r') as f:
        s.parse(ast.parse(f.read()))

rn = 1
for r in resources:
    if r._used:
        r.num = rn
        rn += 1

# compile slots

for s in slots:
    s.compile()

# write temporary files
for s in slots:
    if not s._used:
        continue
    with open(os.path.join(output_dir, f'{s._num}.asm'), 'w') as f:
        s.dump(f)

# write compiled project
output_file = open(output_name, 'wb')

output_file.write(b'MRRD')
write_byte(output_file, 0x10) #version

locomotive.save(output_file)

for f in functions:
    f.save(output_file)

for f in function_keys:
    f.save(output_file)

for s in slots:
    s.save(output_file)

for r in resources:
    r.save(output_file)
