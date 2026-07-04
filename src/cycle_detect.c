#include "cycle_detect.h"
#include <stdlib.h>
#include <stdbool.h>

typedef enum { WHITE, GRAY, BLACK } Color;

static bool has_cycle_dfs(int i, Color *color, Job *jobs) {
    color[i] = GRAY;
    for (int k = 0; k < jobs[i].n_deps; k++) {
        int d = jobs[i].dep_indices[k];
        if (color[d] == GRAY) {
            return true; /* back edge detected */
        }
        if (color[d] == WHITE) {
            if (has_cycle_dfs(d, color, jobs)) {
                return true;
            }
        }
    }
    color[i] = BLACK;
    return false;
}

bool graph_has_cycle(Job *jobs, int n) {
    Color *color = calloc(n, sizeof(Color)); /* initializes all to WHITE (0) */
    if (color == NULL) {
        /* Defensive check for allocation failure */
        return true; 
    }
    bool found = false;
    for (int i = 0; i < n && !found; i++) {
        if (color[i] == WHITE) {
            found = has_cycle_dfs(i, color, jobs);
        }
    }
    free(color);
    return found;
}
