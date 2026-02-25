#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>

typedef struct Slot Slot;

void player_init(void);
void player_clear(void);

void player_abort_slot(Slot *slot);
void play_slot_sound(Slot *slot, uint16_t id, uint8_t priority);

#endif
