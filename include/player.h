#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Slot Slot;

void player_init(void);
void player_clear(void);

void player_abort_slot(Slot *slot, uint8_t subslot);
void play_slot_delay(Slot *slot, uint8_t subslot, uint8_t delay);
void play_slot_sound(Slot *slot, uint8_t subslot, uint16_t id,
                     uint8_t volmin, uint8_t volmax);

bool player_is_on(void);
void player_set_onoff(bool onoff);

#endif
