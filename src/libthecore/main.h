#pragma once

extern volatile int	tics;
extern volatile int shutdowned;

#include "heart.h"

extern LPHEART	thecore_heart;

enum ENUM_PROFILER
{
	PF_IDLE,
	PF_HEARTBEAT,
	NUM_PF
};

extern unsigned int		thecore_profiler[NUM_PF];

int			thecore_init(int fps, HEARTFUNC heartbeat_func);
int			thecore_idle(void);
void		thecore_shutdown(void);
void		thecore_destroy(void);
int			thecore_pulse(void);
float		thecore_time(void);
float		thecore_pulse_per_second(void);
int			thecore_is_shutdowned(void);

void		thecore_tick(void); // tics ¡ı∞°

