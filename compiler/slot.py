from dataclasses import dataclass
from utils import *
from state import *
from bytecode import *
from variables import *
from project import *
import ast
import os

@dataclass
class SlotInfo:
    name: str
    volume: int
    function: str
    num: int = None
    enable: str = 'F_EXECUTING'
    file: str = None

def calculate(expr, context):
    match expr:
        case ast.Constant(value=value):
            return value
        case ast.UnaryOp(op=ast.USub(), operand=op):
            return -calculate(op, context)
        case _:
            raise RRException(f'Invalid operation in expression: {ast.dump(expr)}')

class Optimizer(ast.NodeTransformer):
    _context: State = None

    def __init__(self, context):
        super().__init__()
        self._context = context

    def visit(self, node):
        self.generic_visit(node)

        match node:
            case ast.Compare(ops=[op], left=ast.Name(id=name), comparators=[ast.Constant(value=val)]):
                v = self._context.get_var(name)
                if v is not None:
                    res = None
                    match op:
                        case ast.Eq():
                            res = v == val
                        case ast.NotEq():
                            res = v != val
                        case ast.Gt():
                            res = v > val
                        case ast.GtE():
                            res = v >= val
                        case ast.Lt():
                            res = v < val
                        case ast.LtE():
                            res = v <= val
                    if res is not None:
                        return ast.Constant(value=res)
            case ast.BoolOp(op=ast.And()):
                new_values = []
                for v in node.values:
                    match v:
                        case ast.Constant(value=True):
                            pass
                        case ast.Constant(value=False):
                            return v
                        case _:
                            new_values.append(v)
                if not new_values:
                    return ast.Constant(value=True)
                node.values = new_values
                return node
            case ast.BoolOp(op=ast.Or()):
                new_values = []
                for v in node.values:
                    match v:
                        case ast.Constant(value=True):
                            return v
                        case ast.Constant(value=False):
                            pass
                        case _:
                            new_values.append(v)
                if not new_values:
                    return ast.Constant(value=False)
                node.values = new_values
                return node
            case ast.If(test=ast.Constant(value=True)):
                return node.body
            case ast.If(test=ast.Constant(value=False)):
                return None

        return node

slot_next = 1

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
        global slot_next
        self._info = info
        if info.num:
            self._num = info.num
        else:
            self._num = slot_next
        slot_next = max(slot_next + 1, self._num + 1)
        self._states = []

    def open_file(self, project_dir):
        fname = f'{self._num}.slot'
        if self._info.file:
            fname = self._info.file
        return open(os.path.join(project_dir, fname), 'r')

    def _process_node(self, node, context):
        match node:
            # global stuff
            case ast.Assign(targets=[ast.Name(id='_prefix')], value=ast.Constant(value=prefix)):
                context.set_var('_prefix', prefix + '/')
            case ast.FunctionDef(name=name):
                s = State(name, context)
                context.add_state(s)
                for node in node.body:
                    self._process_node(node, s)
            # local stuff
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
            case ast.While():
                # TODO: process test
                for node in node.body:
                    self._process_node(node, context)
            case ast.If():
                # Maybe will need some day
                # self._process_node(node.test, context)
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
                sample = args[0].value
                if (context.get_var('_prefix') + sample) not in resources_map:
                    raise RRException(f'Invalid resource reference in: {ast.dump(node)}')
            case ast.Expr(value=ast.Call(args=args)):
                # TODO: process_expression?
                # for a in args:
                #     self._process_node(a, context)
                pass
            case ast.Pass():
                pass
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
                    context.add_instruction(IrLoad(SlotVariable(name, self._info.function)))
                else:
                    var = context.get_var(name)
                    if var is None:
                        raise Exception(f'Unknown variable {name}')
                    context.add_instruction(IrLoadI(var))
            case ast.Call(func=ast.Name(id='rand'), args=[left, right]):
                self._generate_expr(right, context);
                self._generate_expr(left, context);
                context.add_instruction(IrRand())
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def _generate_cond(self, node, context, label_then, label_else):
        match node:
            case ast.Name(id='_next'):
                # duplicate _next in stack to allow following goto or ret
                context.add_instruction(IrDup())
                context.add_instruction(IrLoadI(0))
                context.add_instruction(IrCond('eq'))
                context.add_instruction(IrJump(label_then, 'f'))
                # pop unused _next which was duplicated
                context.add_instruction(IrStore(SlotVariable('R_ACCUM')))
                context.add_instruction(IrJump(label_else))
            case ast.Compare(ops=[ast.Eq() | ast.NotEq()], comparators=[ast.Constant(value=True) | ast.Constant(value=False)]):
                match node.left:
                    case ast.Name(id=name):
                        var = SlotVariable(node.left.id, self._info.function)
                        if var.is_cond():
                            context.add_instruction(IrLoadI(0))
                            context.add_instruction(IrLoad(var))
                            context.add_instruction(IrCond('ne'))
                        else:
                            context.add_instruction(IrTest(var, 0))
                    case ast.Subscript(value=ast.Name(id=name), slice=ast.Constant(value=bit)):
                        context.add_instruction(IrTest(SlotVariable(name), bit))
                    case _:
                        raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                flag = 'f'
                if node.ops[0] is ast.NotEq:
                    flag = 't'
                if not node.comparators[0].value:
                    flag = {'f': 't', 't': 'f'}[flag]
                context.add_instruction(IrJump(label_else, flag))
                context.add_instruction(IrJump(label_then))
            case ast.Name(id=name):
                var = SlotVariable(name, self._info.function)
                if var.is_cond():
                    context.add_instruction(IrLoadI(0))
                    context.add_instruction(IrLoad(var))
                    context.add_instruction(IrCond('ne'))
                else:
                    context.add_instruction(IrTest(var, 0))
                context.add_instruction(IrJump(label_else, 'f'))
                context.add_instruction(IrJump(label_then))
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
                context.add_instruction(IrJump(label_then))
            case ast.BoolOp(op=ast.And()):
                for cond in node.values[:-1]:
                    cond_then = IrLabel()
                    self._generate_cond(cond, context, cond_then, label_else)
                    context.add_instruction(cond_then)
                self._generate_cond(node.values[-1], context, label_then, label_else)
            case ast.BoolOp(op=ast.Or()):
                for cond in node.values[:-1]:
                    cond_else = IrLabel()
                    self._generate_cond(cond, context, label_then, cond_else)
                    context.add_instruction(cond_else)
                self._generate_cond(node.values[-1], context, label_then, label_else)
            case ast.UnaryOp(op=ast.Not(), operand=operand):
                self._generate_cond(operand, context, label_else, label_then)
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def _generate_imm_call(self, code, context):
        p = context
        imm = p.get_state('_immediate')
        while p and not imm:
            p = p.get_parent()
            if p:
                imm = p.get_state('_immediate')
        if imm:
            code.add_instruction(IrLoadI(imm))
            code.add_instruction(IrCall())

    def _generate_on_exit_function(self):
        st = self._context.get_state('_exit_processor')
        accum = SlotVariable('R_ACCUM')
        accum2 = SlotVariable('R_ACCUM2')
        retry = IrLabel()
        ret = IrLabel()
        st.add_instruction(retry)
        # remember the top value
        # current level
        st.add_instruction(IrStore(accum))
        # prev level
        st.add_instruction(IrStore(accum2))
        st.add_instruction(IrLoad(accum2))
        st.add_instruction(IrLoad(accum))
        st.add_instruction(IrCond('ge'))
        st.add_instruction(IrJump(ret, 'f'))
        st.add_instruction(IrLoad(accum))
        st.add_instruction(IrSwap())
        st.add_instruction(IrCall())
        st.add_instruction(IrJump(retry))
        st.add_instruction(ret)
        # put unused prev level back
        st.add_instruction(IrLoad(accum2))
        st.add_instruction(IrRet())

    def _generate_ir(self, node, context):
        match node:
            case ast.FunctionDef(name=name):
                s = context.get_state(name)
                # generate prolog
                if not s.is_function():
                    on_exit = 0
                    on_exit_func = s.get_state('_on_exit')
                    if on_exit_func:
                        on_exit = on_exit_func
                    s.add_instruction(IrLoadI(s.get_level()))
                    s.add_instruction(IrLoadI(self._context.get_state('_exit_processor')))
                    s.add_instruction(IrCall())
                    s.add_instruction(IrLoadI(on_exit))
                    s.add_instruction(IrLoadI(s.get_level()))

                # generate body
                for node in node.body:
                    self._generate_ir(node, s)
                if name == '_immediate':
                    self._generate_imm_call(s, context.get_parent())
                # goto _init state after running own (entry) code
                init = s.get_state('_init')
                if init:
                    s.add_instruction(IrLoadI(init))
                    s.add_instruction(IrNext())
                else:
                    # needed for immediate and backup for others
                    s.add_instruction(IrRet())
            case ast.Assign(targets=[ast.Name(id='_prefix')]):
                # not needed here
                pass
            case ast.Assign(targets=[ast.Name(id='_next')], value=ast.Call(func=ast.Name(id=name))):
                # _exit is not a state, but just a function, need to go deeper
                if context.is_function():
                    ref = context.get_parent().get_parent().get_state(name)
                else:
                    ref = context.get_parent().get_state(name)
                context.add_instruction(IrLoadI(ref))
                context.add_instruction(IrCall())
            case ast.Expr(value=ast.Call(func=ast.Name(id='_exit'))):
                # _exit is not a state, but just a function, need to go deeper
                if context.is_function():
                    ref = context.get_parent().get_parent().get_state('_exit')
                else:
                    ref = context.get_parent().get_state('_exit')
                context.add_instruction(IrLoadI(ref))
                context.add_instruction(IrCall())
            case ast.Assign(targets=[ast.Subscript(value=ast.Name(id=name), slice=ast.Constant(value=bit))], value=ast.Constant(value=value)):
                if value is not True and value is not False:
                    raise RRException(f'Invalid value "{value}" for bit assignment for "{name}"')
                if type(bit) != int or bit < 0 or bit > 7:
                    raise RRException(f'Invalid bit "{bit}" for bit assignment for "{name}"')
                if value:
                    context.add_instruction(IrSet(SlotVariable(name), bit))
                else:
                    context.add_instruction(IrReset(SlotVariable(name), bit))
            case ast.Assign(targets=[ast.Name(id=name)], value=ast.Constant(value=bool())):
                if node.value.value:
                    context.add_instruction(IrSet(SlotVariable(name), 0))
                else:
                    context.add_instruction(IrReset(SlotVariable(name), 0))
            case ast.Assign(targets=[ast.Name(id=name)]):
                if name in slot_variables:
                    self._generate_expr(node.value, context)
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
            case ast.While(test=ast.Constant(value=True)):
                start = IrLabel()
                context.add_instruction(start)
                if not context.is_function():
                    self._generate_imm_call(context, context)
                for node in node.body:
                    self._generate_ir(node, context)
                context.add_instruction(IrJump(start))
            case ast.While(test=cond):
                start = IrLabel()
                end = IrLabel()
                body = IrLabel()
                context.add_instruction(start)
                self._generate_cond(node.test, context, body, end)
                context.add_instruction(body)
                if not context.is_function():
                    self._generate_imm_call(context, context)
                for node in node.body:
                    self._generate_ir(node, context)
                context.add_instruction(IrJump(start))
                context.add_instruction(end)
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
                context.add_instruction(IrNext(1))
            case ast.Expr(value=ast.Call(func=ast.Name(id='goto'), args=args)):
                if len(args) not in [1, 2, 4, 8]:
                    raise RRException(f'Invalid operation in slot: {ast.dump(node)}')
                p = context.get_parent()
                if context.is_function():
                    p = p.get_parent()
                for arg in reversed(args):
                    next = p.get_state(arg.id)
                    context.add_instruction(IrLoadI(next))
                context.add_instruction(IrNext(len(args)))
            case ast.Return(value=ast.Name(id='_next')):
                # _next is already in the stack
                context.add_instruction(IrRet())
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
                context.add_instruction(IrLoadI(ref))
                context.add_instruction(IrCall())
                context.add_instruction(IrRet())
            case ast.Expr(value=ast.Call(func=ast.Name(id='switch'))):
                context.add_instruction(IrSwitch())
            case ast.Expr(value=ast.Call(func=ast.Name(id='inc'), args=[ast.Name(id=cond)])):
                var = SlotVariable(cond)
                context.add_instruction(IrInc(var))
            case ast.Expr(value=ast.Call(func=ast.Name(id='dec'), args=[ast.Name(id=cond)])):
                var = SlotVariable(cond)
                context.add_instruction(IrDec(var))
            case ast.Expr(value=ast.Await(value=ast.Call(func=ast.Name(id='delay'), args=[ast.Constant(value=time)]))):
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
                                                              ast.Constant(value=volume_max)]))):
                res = resources_map[context.get_var('_prefix') + sample]
                context.add_instruction(IrLoadI(volume_max * self._info.volume * res.volume // 128 // 128))
                context.add_instruction(IrLoadI(volume_min * self._info.volume * res.volume // 128 // 128))
                context.add_instruction(IrLoadI(res))
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
            case _:
                raise RRException(f'Invalid operation in slot: {ast.dump(node)}')

    def set_used(self):
        self._used = True

    def is_used(self):
        return self._used

    def parse(self, tree):
        if self._tree:
            return

        self._tree = tree
        self._context = State('', global_context)

        # process the source
        for node in tree.body:
            self._process_node(node, self._context)

        # create global exit processor as the last function
        self._context.add_state(State('_exit_processor', self._context))

    def dump(self, f):
        self._context.dump(f)

    def compile(self):
        # substitute known variables and optimize conditions
        self._tree = ast.fix_missing_locations(Optimizer(self._context).visit(self._tree))

        # Load 0 into stack for exit_processor guard
        self._context.add_instruction(IrLoadI(0))

        self._generate_on_exit_function()

        # generate all functions and states
        for node in self._tree.body:
            self._generate_ir(node, self._context)

        # optimize jump-to-jump
        self._context.optimize_jumps()

        # optimize control flow
        try:
            self._context.set_used()
            while self._context.taint_control_flow():
                pass
            self._context.optimize_control_flow()
        except Exception as e:
            print(f'Error of taint optimization in slot {self._num}: {e}')
            pass

    def finalize(self):
        if not self._used:
            return

        # get label addresses
        self._length = self._context.calculate_addresses()

        # replace labels with addresses
        self._context.replace_labels()

    def process_deps(self):
        return self._context.process_deps()

    def save(self, f):
        if not self._used:
            return
        write_byte(f, 0x02)
        write_byte(f, self._num)
        var = SlotVariable(self._info.enable)
        write_byte(f, var.get_address())
        write_dword(f, self._length) # bytecode len
        # write bytecode
        self._context.write_bytecode(f)
