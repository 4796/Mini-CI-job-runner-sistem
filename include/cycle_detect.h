#ifndef CYCLE_DETECT_H
#define CYCLE_DETECT_H

#include <stdbool.h>
#include "job.h"

bool graph_has_cycle(Job *jobs, int n);

#endif /* CYCLE_DETECT_H */
