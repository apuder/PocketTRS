
#ifndef __SOUND_H__
#define __SOUND_H__

typedef unsigned long long tstate_t;

void init_sound();
void transition_out(int value, tstate_t z80_state_t_count);

#endif
