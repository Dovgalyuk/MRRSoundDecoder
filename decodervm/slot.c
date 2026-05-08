#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <inttypes.h>
#include "slot.h"
#include "schedule.h"
#include "bytecode.h"
#include "player.h"
#include "vm.h"
#include "logger.h"

#define DEBUG 0
#define DPRINTF(fmt, ...) \
    do if (DEBUG) printf(fmt, ## __VA_ARGS__); while(0)

#define PUSH(type, d)   do { \
                            slot->type##stack[slot->type##sp] = (d); \
                            slot->type##sp = (slot->type##sp + 1) % STACK_SIZE; \
                        } while (0)
#define PUSH_DATA(d) PUSH(data, d)
#define PUSH_CALL(d) PUSH(call, d)

#define POP(type) ((slot->type##sp = (slot->type##sp - 1 + STACK_SIZE) % STACK_SIZE), \
                   (slot->type##stack[slot->type##sp]))
#define POP_DATA() POP(data)
#define POP_CALL() POP(call)

static uint16_t read_word(const Schedule *sch, uint32_t *pc)
{
    uint16_t res = sch->script[*pc] | (sch->script[*pc + 1] << 8);
    *pc += 2;
    return res;
}

// static uint32_t read_dword(const Schedule *sch, uint32_t *pc)
// {
//     uint32_t w1 = read_word(sch, pc);
//     uint32_t w2 = read_word(sch, pc);
//     return w1 | (w2 << 16);
// }

static uint32_t read_3byte(const Schedule *sch, uint32_t *pc)
{
    uint32_t w1 = read_word(sch, pc);
    uint32_t w2 = sch->script[(*pc)++];
    return w1 | (w2 << 16);
}

uint8_t slot_get_var(Slot *slot, uint16_t addr)
{
    addr -= VAR_LOCAL_START;
    if (addr >= VAR_LOCAL_SIZE) {
        return 0;
    }
    return slot->locals[addr];
}

void slot_set_var(Slot *slot, uint16_t addr, uint8_t val)
{
    addr -= VAR_LOCAL_START;
    if (addr < VAR_LOCAL_SIZE) {
        slot->locals[addr] = val;
    }
}

static int16_t slot_read_mem(Slot *slot, uint16_t addr)
{
    if (addr >= VAR_END) {
        return 0;
    }
    if (addr - VAR_LOCAL_START < VAR_LOCAL_SIZE) {
        return slot_get_var(slot, addr);
    }
    uint8_t v = vm_get_var(addr);
    if (addr >= VAR_GLOBAL_SIGNED_START) {
        return (int8_t)v;
    }
    return v;
}

static void slot_write_mem(Slot *slot, uint16_t addr, uint8_t val)
{
    if (addr >= VAR_END) {
        return;
    }
    if (addr - VAR_LOCAL_START < VAR_LOCAL_SIZE) {
        slot_set_var(slot, addr, val);
    } else {
        vm_set_var(addr, val);
    }
}

static void slot_set_pc(Slot *slot, uint32_t pc)
{
    if (!slot->schedule || pc >= slot->schedule->script_size) {
        slot->error = true;
        return;
    }
    slot->pc = pc;
}

static void slot_next_state(Slot *slot, uint32_t addr)
{
    slot->pc = addr;
    /* Stop sound if state switch was caused by immediate transition */
    if (slot_get_var(slot, F_PLAYING)) {
        player_abort_slot(slot, slot->subslot);
    }
    /* Reset drive-related variables */
    slot_set_var(slot, F_DRIVELOCK, 0);
    /* Reset sound-related variables */
    //slot_set_var(slot, F_RESTORE, 0);
}

static void slot_delay(Slot *slot, uint8_t delay)
{
    if (slot_get_var(slot, F_PLAYING)) {
        player_abort_slot(slot, slot->subslot);
    }
    play_slot_delay(slot, slot->subslot, delay);
}

static void slot_play(Slot *slot, uint16_t id, uint8_t volmin, uint8_t volmax)
{
    if (slot_get_var(slot, F_PLAYING)) {
        player_abort_slot(slot, slot->subslot);
    }
    play_slot_sound(slot, slot->subslot, id, volmin, volmax);
}

void slot_started_delay(Slot *slot, uint8_t subslot)
{
    uint8_t var = subslot == slot->subslot ? F_PLAYING : F_PLAYING2;
    slot_set_var(slot, var, 1);
}

void slot_started_sound(Slot *slot, uint8_t subslot)
{
    slot_started_delay(slot, subslot);
    vm_reset_trigger();
}

void slot_finished_sound(Slot *slot, uint8_t subslot)
{
    uint8_t var = subslot == slot->subslot ? F_PLAYING : F_PLAYING2;
    slot_set_var(slot, var, 0);
}

void slot_switch(Slot *slot)
{
    uint8_t pl1 = slot_get_var(slot, F_PLAYING);
    uint8_t pl2 = slot_get_var(slot, F_PLAYING2);
    slot_set_var(slot, F_PLAYING, pl2);
    slot_set_var(slot, F_PLAYING2, pl1);
    slot->subslot = 1 - slot->subslot;
}

void slot_init(Slot *slot, Schedule *schedule)
{
    slot->schedule = schedule;
    slot_reset(slot);
}

void slot_clear(Slot *slot)
{
    free(slot->schedule);
    slot->schedule = NULL;
}

void slot_reset(Slot *slot)
{
    if (!slot->schedule) {
        return;
    }
    slot->error = false;
    slot->datasp = 0;
    slot->callsp = 0;
    slot->flag = 0;
    slot->subslot = 0;
    memset(slot->locals, 0, sizeof(slot->locals));
    slot->pc = 0;

    slot_set_var(slot, F_FUNCTION,
        !!(slot->schedule->flags & SCHEDULE_FLAG_FORCE));
}

void slot_step(Slot *slot)
{
    if (!slot->schedule || slot->error) {
        return;
    }
    int32_t first = slot->pc;
    uint8_t op = slot->schedule->script[slot->pc++];
    uint8_t oparg;
    uint8_t arg8;
    uint16_t arg16;
    uint32_t arg32;
    DPRINTF("%"PRId32":\t0x%x\t", first, op);
    switch (op) {
    case I_NOP:
        DPRINTF("NOP\n");
        break;
    case I_SWITCH:
        DPRINTF("SWITCH\n");
        slot_switch(slot);
        break;
    case I_RET:
        DPRINTF("RET\n");
        slot->pc = POP_CALL();
        break;
    case I_RAND:
        {
            DPRINTF("RAND\n");
            uint8_t min = POP_DATA();
            uint8_t max = POP_DATA();
            int x = min + rand() % (max + 1 - min);
            PUSH_DATA(x);
        }
        break;
    case I_ADD:
        {
            DPRINTF("ADD\n");
            uint8_t op1 = POP_DATA();
            uint8_t op2 = POP_DATA();
            PUSH_DATA(op1 + op2);
        }
        break;
    case I_SUB:
        {
            DPRINTF("SUB\n");
            uint8_t op1 = POP_DATA();
            uint8_t op2 = POP_DATA();
            PUSH_DATA(op2 - op1);
        }
        break;
    case I_DELAY:
        {
            DPRINTF("DELAY\n");
            uint8_t delay = POP_DATA();
            slot_delay(slot, delay);
        }
        break;
    case I_PLAY:
        {
            uint16_t sample = POP_DATA();
            uint8_t volmin = POP_DATA();
            uint8_t volmax = POP_DATA();
            DPRINTF("PLAY %d\n", sample);
            slot_play(slot, sample, volmin, volmax);
        }
        break;
    case I_JUMP:
        arg16 = read_word(slot->schedule, &slot->pc);
        DPRINTF("JUMP %d\n", arg16);
        slot->pc = first + (int16_t)arg16;
        break;
    case I_JUMPT:
        arg16 = read_word(slot->schedule, &slot->pc);
        DPRINTF("JUMPT %d\n", arg16);
        if (slot->flag) {
            slot->pc = first + (int16_t)arg16;
        }
        break;
    case I_JUMPF:
        arg16 = read_word(slot->schedule, &slot->pc);
        DPRINTF("JUMPF %d\n", arg16);
        if (!slot->flag) {
            slot->pc = first + (int16_t)arg16;
        }
        break;
    case I_LOADI:
        arg32 = read_3byte(slot->schedule, &slot->pc);
        DPRINTF("LOADI %d\n", (int)arg32);
        PUSH_DATA(arg32);
        break;
    case I_LOAD:
        arg8 = slot->schedule->script[slot->pc++];
        DPRINTF("LOAD %d\n", arg8);
        PUSH_DATA(slot_read_mem(slot, arg8));
        break;
    case I_STORE:
        arg8 = slot->schedule->script[slot->pc++];
        DPRINTF("STORE %d\n", arg8);
        slot_write_mem(slot, arg8, POP_DATA());
        break;
    case I_CALL:
        arg32 = read_3byte(slot->schedule, &slot->pc);
        DPRINTF("CALL %d\n", (int)arg32);
        PUSH_CALL(slot->pc);
        slot_set_pc(slot, arg32);
        break;
    case I_NEXT...(I_NEXT + 7):
        {
            oparg = op - I_NEXT + 1;
            DPRINTF("NEXT %d\n", oparg);
            uint8_t cyl = vm_get_var(V_CYLINDER) % oparg;
            uint32_t next = 0;
            for (int i = 0 ; i < oparg ; ++i) {
                arg32 = POP_DATA();
                if (i == cyl) {
                    next = arg32;
                }
            }
            slot_next_state(slot, next);
        }
        break;
    case I_SET0...I_SET7:
        {
            oparg = op - I_SET0;
            arg8 = slot->schedule->script[slot->pc++];
            DPRINTF("SET%d %d\n", oparg, arg8);
            uint8_t tmp = slot_read_mem(slot, arg8);
            tmp |= 1 << oparg;
            slot_write_mem(slot, arg8, tmp);
        }
        break;
    case I_RESET0...I_RESET7:
        {
            oparg = op - I_RESET0;
            arg8 = slot->schedule->script[slot->pc++];
            DPRINTF("RESET%d %d\n", oparg, arg8);
            uint8_t tmp = slot_read_mem(slot, arg8);
            tmp &= ~(1 << oparg);
            slot_write_mem(slot, arg8, tmp);
        }
        break;
    case I_TEST0...I_TEST7:
        oparg = op - I_TEST0;
        arg8 = slot->schedule->script[slot->pc++];
        DPRINTF("TEST%d %d\n", oparg, arg8);
        slot->flag = slot_read_mem(slot, arg8) & (1 << oparg);
        break;
    case I_CONDEQ...I_CONDLE:
        oparg = op - I_COND;
        {
            int16_t right = POP_DATA();
            int16_t left = POP_DATA();
            DPRINTF("COND%d %d %d\n", oparg, left, right);
            switch (oparg) {
            case C_EQ:
                slot->flag = left == right;
                break;
            case C_NE:
                slot->flag = left != right;
                break;
            case C_GT:
                slot->flag = left > right;
                break;
            case C_GE:
                slot->flag = left >= right;
                break;
            case C_LT:
                slot->flag = left < right;
                break;
            case C_LE:
                slot->flag = left <= right;
                break;
            default:
                /* Error */
                DPRINTF("ERROR cond\n");
                break;
            }
        }
        break;
    default:
        /* Error */
        DPRINTF("ERROR instr\n");
        break;
    }
}
