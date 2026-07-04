#ifndef PARSER_H
#define PARSER_H

#include "job.h"

int parse_config(const char *path, Job **jobs_out, int *n_jobs_out, int *num_workers_out);

#endif /* PARSER_H */
