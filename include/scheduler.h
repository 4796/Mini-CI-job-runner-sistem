#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "job.h"

void* scheduler_loop(void *arg);
void  propagate(int finished_idx);

#endif /* SCHEDULER_H */
