from dataclasses import dataclass
from utils import *
from state import *
from bytecode import *
from variables import *
from resources import resources_map
import ast

@dataclass
class SlotInfo:
    num: int
    name: str
    volume: int
    flags: list[str]

def calculate(expr, context):
    match expr:
        case ast.Constant(value=value):
            return value
        case ast.UnaryOp(op=ast.USub(), operand=op):
            return -calculate(op, context)
        case _:
            raise RRException(f'Invalid operation in expression: {ast.dump(expr)}')

class Slot:
    _info: SlotInfo = None
    _used: bool = False
    # may be used for renumbering
    _num: int
    # AST from source
    _tree = None
    # Global slot context, inherited from project
    _context: State = None
    _states: list[State]
    _length: int

    def __init__(self, info):
        self._info = info
        self._num = info.num
        self._states = []

    def _process_node(self, node, context):
        match node:
            case ast.Assign(targets=[ast.Name(id='_prefix')], value=ast.Constant(value=prefix)):
                context.set_var('_prefix', prefix)
            case ast.Assign(targets=[ast.Name(id='_next')]):
                pass
            case ast.Assign(targets=[ast.Name(id=name)]):
                if name not in slot_variables:
                    context.set_var(name, calculate(node.value, context))
            case ast.AugAssign(target=ast.Name(id=name)):
                #context.set_var(name, calculate(node.value, context))
                pass
            case ast.Assign(targets=[ast.Subscript(value=ast.Name(id=name), slice=ast.Constant(value=bit))]):
                #context.set_var(name, calculate(node.value, context))
                pass
            case ast.While(test=ast.Constant(value=True)):
                for node in node.body:
                    self._process_node(node, context)
            case ast.If():
                # TODO: process node.test
                for node in node.body:
                    self._process_node(node, context)
                # else is not supported yet
            case ast.Expr(value=ast.Call(func=ast.Name(id='goto'))):
                # remember state references
                pass
            case ast.Return(value=ast.Name(id=name)):
                # remeber state references
                pass
            case ast.Return(value=ast.Constant(value=0)):
                pass
            case ast.Return(value=ast.Call(func=ast.Name(id='_exit'))):
                pass
            case ast.Expr(value=ast.Call(func=ast.Name(id='switch'))):
                pass
            case ast.Expr(value=ast.Await(value=ast.Call(func=ast.Name(id='delay')))):
                pass
            case ast.Expr(value=ast.Await(value=ast.Call(func=ast.Name(id='play'),
                                                        args=args))):
                # check resource references
                sample = args[0].value
                res = resources_map[context.get_var('_prefix') + '/' + sample]
                res._used = True
            case ast.Pass():
                pass
            case ast.FunctionDef(name=name):
                s = State(name, context)
                context.add_state(s)
                for node in node.body:
                    self._process_node(node, s)
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def _generate_expr(self, node, context):
        match node:
            case ast.UnaryOp(op=ast.USub()):
                context.add_instruction(IrLoadI(0))
                self._generate_expr(node.operand, context)
                context.add_instruction(IrSub())
            case ast.BinOp():
                self._generate_expr(node.left, context)
                self._generate_expr(node.right, context)
                match node.op:
                    case ast.Add():
                        context.add_instruction(IrAdd())
                    case ast.Sub():
                        context.add_instruction(IrSub())
                    case _:
                        raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
            case ast.Constant(value=value):
                context.add_instruction(IrLoadI(value))
            case ast.Name(id=name):
                if name in slot_variables:
                    context.add_instruction(IrLoad(SlotVariable(name)))
                else:
                    context.add_instruction(IrLoadI(context.get_var(name)))
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def _generate_cond(self, node, context, label_then, label_else):
        match node:
            case ast.Name(id='_next'):
                # TODO: jump should work with stack?
                context.add_instruction(IrLoadI(0))
                context.add_instruction(IrCond('eq'))
                context.add_instruction(IrJump(label_else, 't'))
            case ast.Compare(ops=[ast.Eq() | ast.NotEq()], comparators=[ast.Constant(value=True) | ast.Constant(value=False)]):
                match node.left:
                    case ast.Name(id=name):
                        context.add_instruction(IrTest(SlotVariable(name), 0))
                    case ast.Subscript(value=ast.Name(id=name), slice=ast.Constant(value=bit)):
                        context.add_instruction(IrTest(SlotVariable(name), bit))
                    case _:
                        raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                flag = 'f'
                if node.ops[0] is ast.NotEq:
                    flag = 't'
                context.add_instruction(IrJump(label_else, flag))
                # label_then is expected to be here
            case ast.Compare(ops=[op], comparators=[right]):
                self._generate_expr(node.left, context)
                self._generate_expr(right, context)
                match op:
                    case ast.Eq():
                        context.add_instruction(IrCond('eq'))
                    case ast.NotEq():
                        context.add_instruction(IrCond('ne'))
                    case ast.Gt():
                        context.add_instruction(IrCond('gt'))
                    case ast.GtE():
                        context.add_instruction(IrCond('ge'))
                    case ast.Lt():
                        context.add_instruction(IrCond('lt'))
                    case ast.LtE():
                        context.add_instruction(IrCond('le'))
                    case _:
                        raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                context.add_instruction(IrJump(label_else, 'f'))
                # label_then is expected to be here
            case ast.BoolOp(op=ast.And()):
                for cond in node.values:
                    cond_then = IrLabel()
                    self._generate_cond(cond, context, cond_then, label_else)
                    context.add_instruction(cond_then)
                # label_then is expected to be here
            case ast.BoolOp(op=ast.Or()):
                for cond in node.values:
                    cond_else = IrLabel()
                    self._generate_cond(cond, context, label_then, cond_else)
                    context.add_instruction(cond_else)
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def _generate_imm_call(self, code, context):
        p = context.get_parent()
        imm = p.get_state('_immediate')
        while p and not imm:
            p = p.get_parent()
            if p:
                imm = p.get_state('_immediate')
        if imm:
            code.add_instruction(IrCall(imm))

    def _generate_ir(self, node, context):
        match node:
            case ast.Assign(targets=[ast.Name(id='_prefix')]):
                # not needed here
                pass
            case ast.Assign(targets=[ast.Name(id='_next')], value=ast.Call(func=ast.Name(id=name))):
                # _exit is not a state, but just a function, need to go deeper
                if context.is_function():
                    ref = context.get_parent().get_parent().get_state(name)
                else:
                    ref = context.get_parent().get_state(name)
                context.add_instruction(IrCall(ref))
            case ast.Assign(targets=[ast.Name(id=name)]):
                if name in slot_variables:
                    context.add_instruction(IrStore(SlotVariable(name)))
            case ast.AugAssign(target=ast.Name(id=name)):
                context.add_instruction(IrLoad(SlotVariable(name)))
                self._generate_expr(node.value, context)
                match node.op:
                    case ast.Add():
                        context.add_instruction(IrAdd())
                    case ast.Sub():
                        context.add_instruction(IrSub())
                    case _:
                        raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                context.add_instruction(IrStore(SlotVariable(name)))
                pass
            case ast.Assign(targets=[ast.Subscript(value=ast.Name(id=name), slice=ast.Constant(value=bit))], value=ast.Constant(value=value)):
                if value is not True and value is not False:
                    raise RRException(f'Invalid value "{value}" for bit assignment for "{name}"')
                if value:
                    context.add_instruction(IrSet(SlotVariable(name), bit))
                else:
                    context.add_instruction(IrReset(SlotVariable(name), bit))
            case ast.While(test=ast.Constant(value=True)):
                start = IrLabel()
                context.add_instruction(start)
                if not context.is_function():
                    self._generate_imm_call(context, context)
                for node in node.body:
                    self._generate_ir(node, context)
                context.add_instruction(IrJump(start))
            case ast.If():
                label_then = IrLabel()
                label_else = IrLabel()
                self._generate_cond(node.test, context, label_then, label_else)
                context.add_instruction(label_then)
                for node in node.body:
                    self._generate_ir(node, context)
                context.add_instruction(label_else)
                # else is not supported yet
            case ast.Expr(value=ast.Call(func=ast.Name(id='goto'), args=[ast.Name(id='_next')])):
                # _next is on top of the stack
                context.add_instruction(IrNext())
            case ast.Expr(value=ast.Call(func=ast.Name(id='goto'), args=[ast.Name(id=name)])):
                p = context.get_parent()
                if context.is_function():
                    p = p.get_parent()
                context.add_instruction(IrLoadI(p.get_state(name)))
                context.add_instruction(IrNext())
            case ast.Expr(value=ast.Call(func=ast.Name(id='goto'), args=args)):
                if len(args) != 8:
                    raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                p = context.get_parent()
                if context.is_function():
                    p = p.get_parent()
                for arg in reversed(args):
                    next = p.get_state(arg.id)
                    context.add_instruction(IrLoadI(next))
                context.add_instruction(IrNextCyl())
            case ast.Return(value=ast.Name(id=name)):
                ref = context.get_parent().get_parent().get_state(name)
                context.add_instruction(IrLoadI(ref))
                context.add_instruction(IrRet())
            case ast.Return(value=ast.Constant(value=0)):
                context.add_instruction(IrLoadI(0))
                context.add_instruction(IrRet())
            case ast.Return(value=ast.Call(func=ast.Name(id='_exit'))):
                if context.is_function():
                    ref = context.get_parent().get_parent().get_state('_exit')
                else:
                    ref = context.get_parent().get_state('_exit')
                context.add_instruction(IrCall(ref))
                context.add_instruction(IrRet())
            case ast.Expr(value=ast.Call(func=ast.Name(id='switch'))):
                context.add_instruction(IrSwitch())
            case ast.Expr(value=ast.Await(value=ast.Call(func=ast.Name(id='delay'), args=[ast.Constant(value=time), ast.Constant(value=drivelock)]))):
                context.add_instruction(IrLoadI(drivelock))
                context.add_instruction(IrLoadI(time))
                context.add_instruction(IrDelay())
                label = IrLabel()
                context.add_instruction(label)
                if not context.is_function():
                    self._generate_imm_call(context, context)
                context.add_instruction(IrTest(SlotVariable('F_PLAYING'), 0))
                context.add_instruction(IrJump(label, 't'))
            case ast.Expr(value=ast.Await(value=ast.Call(func=ast.Name(id='play'),
                                                        args=[ast.Constant(value=sample),
                                                              ast.Constant(value=volume_min),
                                                              ast.Constant(value=volume_max),
                                                              ast.Constant(value=drivelock)]))):
                context.add_instruction(IrLoadI(drivelock))
                res = resources_map[context.get_var('_prefix') + '/' + sample]
                context.add_instruction(IrLoadI(volume_max * self._info.volume * res.volume // 128 // 128))
                context.add_instruction(IrLoadI(volume_min * self._info.volume * res.volume // 128 // 128))
                context.add_instruction(IrComment(sample))
                context.add_instruction(IrLoadI(res.num))
                context.add_instruction(IrPlay())
                label = IrLabel()
                context.add_instruction(label)
                if not context.is_function():
                    self._generate_imm_call(context, context)
                context.add_instruction(IrTest(SlotVariable('F_PLAYING'), 0))
                context.add_instruction(IrJump(label, 't'))
            case ast.Pass():
                # do nothing
                pass
            case ast.FunctionDef(name=name):
                s = context.get_state(name)
                for node in node.body:
                    self._generate_ir(node, s)
                if name == '_immediate':
                    self._generate_imm_call(s, context)
                # needed for immediate and backup for others
                s.add_instruction(IrRet())
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def parse(self, tree):
        if not self._used:
            return

        self._tree = tree
        self._context = State('', global_context)
        for node in tree.body:
            self._process_node(node, self._context)

    def dump(self, f):
        self._context.dump(f)

    def compile(self):
        if not self._used:
            return

        # TODO: substitute variables and optimize IR/States

        # address 0 is reserved, place nop there
        self._context.add_instruction(IrNop())
        for node in self._tree.body:
            self._generate_ir(node, self._context)

        # get label addresses
        self._length = self._context.calculate_addresses()

        # replace labels with addresses
        self._context.replace_labels()

    def save(self, f):
        if not self._used:
            return
        write_byte(f, 0x02)
        write_byte(f, self._num)
        #write_string(f, self._info.name)
        #write_byte(f, self._info.volume)
        flags = 0
        for flag in self._info.flags:
            flags += {'force': 1, 'brake': 4}[flag]
        write_byte(f, flags)
        #write_dword(f, 0) # start
        write_dword(f, self._length) # bytecode len
        # write bytecode
        self._context.write_bytecode(f)
