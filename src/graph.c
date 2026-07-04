#include "graph.h"
#include <stdio.h>
#include <stdlib.h>

void build_children_lists(Job *jobs, int n_jobs) {
    if (jobs == NULL) {
        return;
    }
    for (int i = 0; i < n_jobs; i++) {
        for (int k = 0; k < jobs[i].n_deps; k++) {
            int d = jobs[i].dep_indices[k];
            ListNode *node = malloc(sizeof(ListNode));
            if (node == NULL) {
                fprintf(stderr, "Error: memory allocation failed in build_children_lists\n");
                exit(1);
            }
            node->value = i;
            node->next = jobs[d].children; /* prepend to the start of the list, O(1) */
            jobs[d].children = node;
            jobs[d].n_children++;
        }
    }
}
