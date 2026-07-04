#include "job.h"
#include <stdlib.h>

void free_jobs(Job *jobs, int n) {
    if (jobs == NULL) {
        return;
    }
    for (int i = 0; i < n; i++) {
        free(jobs[i].dep_indices);
        ListNode *cur = jobs[i].children;
        while (cur != NULL) {
            ListNode *next = cur->next;
            free(cur);
            cur = next;
        }
    }
    free(jobs);
}
