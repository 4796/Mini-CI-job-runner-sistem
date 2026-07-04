#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Temporary node structure used during parsing */
typedef struct JobNode {
    char name[64];
    char command[256];
    char **dep_names;
    int n_deps;
    struct JobNode *next;

    int has_name;
    int has_command;
    int has_depends_on;
} JobNode;

/* Helper to duplicate string safely in C99/POSIX */
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (d != NULL) {
        memcpy(d, s, len);
    }
    return d;
}

/* Helper to free the temporary JobNode list */
static void free_job_nodes(JobNode *head) {
    JobNode *curr = head;
    while (curr != NULL) {
        JobNode *next = curr->next;
        if (curr->dep_names != NULL) {
            for (int k = 0; k < curr->n_deps; k++) {
                free(curr->dep_names[k]);
            }
            free(curr->dep_names);
        }
        free(curr);
        curr = next;
    }
}

int parse_config(const char *path, Job **jobs_out, int *n_jobs_out, int *num_workers_out) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: cannot open config file '%s'\n", path);
        return 1;
    }

    JobNode *head = NULL;
    JobNode *tail = NULL;
    JobNode *current_node = NULL;
    int n_jobs = 0;
    int has_workers = 0;
    int workers_val = 0;
    int in_jobs = 0;

    char line[1024];
    int line_num = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_num++;

        /* 1. Trim trailing spaces/newlines/carriage returns */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t' || 
                           line[len - 1] == '\r' || line[len - 1] == '\n')) {
            line[len - 1] = '\0';
            len--;
        }

        /* 2. Skip comments or empty lines */
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\0') {
            continue;
        }

        /* 3. Match keys */
        if (strncmp(line, "workers:", 8) == 0) {
            if (has_workers) {
                fprintf(stderr, "Error: duplicate 'workers' field on line %d\n", line_num);
                goto error;
            }
            const char *val = line + 8;
            while (*val == ' ' || *val == '\t') val++;
            char *endptr;
            long val_l = strtol(val, &endptr, 10);
            if (endptr == val || *endptr != '\0' || val_l < 1) {
                fprintf(stderr, "Error: invalid workers count on line %d: '%s'\n", line_num, val);
                goto error;
            }
            workers_val = (int)val_l;
            has_workers = 1;
        } 
        else if (strcmp(line, "jobs:") == 0) {
            in_jobs = 1;
        } 
        else if (strncmp(line, "  - name:", 9) == 0) {
            if (!in_jobs) {
                fprintf(stderr, "Error: jobs list must follow 'jobs:' header (line %d)\n", line_num);
                goto error;
            }
            const char *val = line + 9;
            while (*val == ' ' || *val == '\t') val++;

            /* Validate no spaces in name */
            for (int i = 0; val[i] != '\0'; i++) {
                if (val[i] == ' ' || val[i] == '\t') {
                    fprintf(stderr, "Error: job name contains spaces on line %d: '%s'\n", line_num, val);
                    goto error;
                }
            }
            if (strlen(val) == 0) {
                fprintf(stderr, "Error: empty job name on line %d\n", line_num);
                goto error;
            }
            if (strlen(val) > 63) {
                fprintf(stderr, "Error: job name exceeds 63 characters on line %d: '%s'\n", line_num, val);
                goto error;
            }

            /* Validate previous node */
            if (current_node != NULL) {
                if (!current_node->has_command) {
                    fprintf(stderr, "Error: job '%s' is missing a command (line %d)\n", current_node->name, line_num);
                    goto error;
                }
                if (!current_node->has_depends_on) {
                    fprintf(stderr, "Error: job '%s' is missing depends_on list (line %d)\n", current_node->name, line_num);
                    goto error;
                }
            }

            /* Check duplicate job names */
            JobNode *curr = head;
            while (curr != NULL) {
                if (strcmp(curr->name, val) == 0) {
                    fprintf(stderr, "Error: duplicate job name '%s' on line %d\n", val, line_num);
                    goto error;
                }
                curr = curr->next;
            }

            /* Allocate new JobNode */
            JobNode *new_node = calloc(1, sizeof(JobNode));
            if (new_node == NULL) {
                fprintf(stderr, "Error: memory allocation failed on line %d\n", line_num);
                goto error;
            }
            strcpy(new_node->name, val);
            new_node->has_name = 1;

            if (head == NULL) {
                head = new_node;
                tail = new_node;
            } else {
                tail->next = new_node;
                tail = new_node;
            }
            current_node = new_node;
            n_jobs++;
        } 
        else if (strncmp(line, "    command:", 12) == 0) {
            if (current_node == NULL) {
                fprintf(stderr, "Error: command specified outside of any job block on line %d\n", line_num);
                goto error;
            }
            if (current_node->has_command) {
                fprintf(stderr, "Error: duplicate command for job '%s' on line %d\n", current_node->name, line_num);
                goto error;
            }
            const char *val = line + 12;
            while (*val == ' ' || *val == '\t') val++;
            size_t val_len = strlen(val);
            if (val_len < 2 || val[0] != '"' || val[val_len - 1] != '"') {
                fprintf(stderr, "Error: command for job '%s' must be wrapped in double quotes on line %d: %s\n", 
                        current_node->name, line_num, val);
                goto error;
            }
            size_t cmd_len = val_len - 2;
            if (cmd_len == 0) {
                fprintf(stderr, "Error: command for job '%s' is empty on line %d\n", current_node->name, line_num);
                goto error;
            }
            if (cmd_len > 255) {
                fprintf(stderr, "Error: command for job '%s' exceeds 255 characters on line %d\n", current_node->name, line_num);
                goto error;
            }
            strncpy(current_node->command, val + 1, cmd_len);
            current_node->command[cmd_len] = '\0';
            current_node->has_command = 1;
        } 
        else if (strncmp(line, "    depends_on:", 15) == 0) {
            if (current_node == NULL) {
                fprintf(stderr, "Error: depends_on specified outside of any job block on line %d\n", line_num);
                goto error;
            }
            if (current_node->has_depends_on) {
                fprintf(stderr, "Error: duplicate depends_on for job '%s' on line %d\n", current_node->name, line_num);
                goto error;
            }
            const char *val = line + 15;
            while (*val == ' ' || *val == '\t') val++;
            size_t val_len = strlen(val);
            if (val_len < 2 || val[0] != '[' || val[val_len - 1] != ']') {
                fprintf(stderr, "Error: depends_on list for job '%s' must be in brackets [] on line %d: %s\n", 
                        current_node->name, line_num, val);
                goto error;
            }

            char dep_buf[512];
            size_t inner_len = val_len - 2;
            if (inner_len >= sizeof(dep_buf)) {
                fprintf(stderr, "Error: depends_on list too long for job '%s' on line %d\n", current_node->name, line_num);
                goto error;
            }
            strncpy(dep_buf, val + 1, inner_len);
            dep_buf[inner_len] = '\0';

            /* Check if list is empty or whitespace only */
            int is_empty = 1;
            for (size_t i = 0; i < inner_len; i++) {
                if (dep_buf[i] != ' ' && dep_buf[i] != '\t') {
                    is_empty = 0;
                    break;
                }
            }

            if (is_empty) {
                current_node->n_deps = 0;
                current_node->dep_names = NULL;
            } else {
                int commas = 0;
                for (size_t i = 0; i < inner_len; i++) {
                    if (dep_buf[i] == ',') {
                        commas++;
                    }
                }
                int n_deps = commas + 1;
                current_node->dep_names = calloc(n_deps, sizeof(char *));
                if (current_node->dep_names == NULL) {
                    fprintf(stderr, "Error: memory allocation failed on line %d\n", line_num);
                    goto error;
                }
                current_node->n_deps = n_deps;

                const char *curr_ptr = dep_buf;
                for (int d = 0; d < n_deps; d++) {
                    const char *comma = strchr(curr_ptr, ',');
                    char token[128];
                    if (comma != NULL) {
                        size_t token_len = comma - curr_ptr;
                        if (token_len >= sizeof(token)) {
                            token_len = sizeof(token) - 1;
                        }
                        strncpy(token, curr_ptr, token_len);
                        token[token_len] = '\0';
                        curr_ptr = comma + 1;
                    } else {
                        strncpy(token, curr_ptr, sizeof(token) - 1);
                        token[sizeof(token) - 1] = '\0';
                    }

                    /* Trim token */
                    const char *tok_start = token;
                    while (*tok_start == ' ' || *tok_start == '\t') tok_start++;
                    size_t tok_len = strlen(tok_start);
                    while (tok_len > 0 && (tok_start[tok_len - 1] == ' ' || tok_start[tok_len - 1] == '\t')) {
                        tok_len--;
                    }
                    char trimmed[64];
                    if (tok_len >= sizeof(trimmed)) {
                        tok_len = sizeof(trimmed) - 1;
                    }
                    strncpy(trimmed, tok_start, tok_len);
                    trimmed[tok_len] = '\0';

                    if (strlen(trimmed) == 0) {
                        fprintf(stderr, "Error: empty dependency name in list for job '%s' on line %d\n", 
                                current_node->name, line_num);
                        goto error;
                    }

                    /* Check duplicate in depends_on */
                    for (int prev = 0; prev < d; prev++) {
                        if (strcmp(current_node->dep_names[prev], trimmed) == 0) {
                            fprintf(stderr, "Error: duplicate dependency '%s' in job '%s' on line %d\n", 
                                    trimmed, current_node->name, line_num);
                            goto error;
                        }
                    }

                    current_node->dep_names[d] = my_strdup(trimmed);
                    if (current_node->dep_names[d] == NULL) {
                        fprintf(stderr, "Error: memory allocation failed on line %d\n", line_num);
                        goto error;
                    }
                }
            }
            current_node->has_depends_on = 1;
        } 
        else {
            fprintf(stderr, "Error: syntax error or unsupported YAML syntax on line %d: '%s'\n", line_num, line);
            goto error;
        }
    }

    fclose(fp);
    fp = NULL;

    /* Validate file-wide requirements */
    if (!has_workers) {
        fprintf(stderr, "Error: missing 'workers' field in config file\n");
        goto error;
    }
    if (n_jobs == 0) {
        fprintf(stderr, "Error: jobs list is empty in config file\n");
        goto error;
    }
    if (current_node != NULL) {
        if (!current_node->has_command) {
            fprintf(stderr, "Error: job '%s' is missing a command at the end of file\n", current_node->name);
            goto error;
        }
        if (!current_node->has_depends_on) {
            fprintf(stderr, "Error: job '%s' is missing depends_on list at the end of file\n", current_node->name);
            goto error;
        }
    }

    /* Convert temporary JobNode list to single dynamically allocated Job array */
    Job *jobs_arr = malloc(n_jobs * sizeof(Job));
    if (jobs_arr == NULL) {
        fprintf(stderr, "Error: memory allocation failed during job array conversion\n");
        goto error;
    }

    /* Zero/initialize all fields first */
    JobNode *curr = head;
    for (int i = 0; i < n_jobs; i++) {
        strncpy(jobs_arr[i].name, curr->name, sizeof(jobs_arr[i].name));
        jobs_arr[i].name[sizeof(jobs_arr[i].name) - 1] = '\0';

        strncpy(jobs_arr[i].command, curr->command, sizeof(jobs_arr[i].command));
        jobs_arr[i].command[sizeof(jobs_arr[i].command) - 1] = '\0';

        jobs_arr[i].n_deps = curr->n_deps;
        if (curr->n_deps > 0) {
            jobs_arr[i].dep_indices = malloc(curr->n_deps * sizeof(int));
            if (jobs_arr[i].dep_indices == NULL) {
                fprintf(stderr, "Error: memory allocation failed during job array conversion\n");
                /* Clean up array elements initialized so far, then free the rest */
                for (int clean = 0; clean < i; clean++) {
                    free(jobs_arr[clean].dep_indices);
                }
                free(jobs_arr);
                goto error;
            }
        } else {
            jobs_arr[i].dep_indices = NULL;
        }

        jobs_arr[i].children = NULL;
        jobs_arr[i].n_children = 0;
        jobs_arr[i].unresolved_deps = curr->n_deps;
        jobs_arr[i].status = PENDING;
        snprintf(jobs_arr[i].log_path, sizeof(jobs_arr[i].log_path), "logs/%s.log", jobs_arr[i].name);

        curr = curr->next;
    }

    /* Resolve dep_names to dep_indices using linear search */
    curr = head;
    for (int i = 0; i < n_jobs; i++) {
        for (int k = 0; k < jobs_arr[i].n_deps; k++) {
            const char *dep_name = curr->dep_names[k];
            int found_idx = -1;
            for (int j = 0; j < n_jobs; j++) {
                if (strcmp(jobs_arr[j].name, dep_name) == 0) {
                    found_idx = j;
                    break;
                }
            }
            if (found_idx == -1) {
                fprintf(stderr, "Error: job '%s' depends on non-existent job '%s'\n", jobs_arr[i].name, dep_name);
                /* Cleanup full job array */
                for (int clean = 0; clean < n_jobs; clean++) {
                    free(jobs_arr[clean].dep_indices);
                }
                free(jobs_arr);
                goto error;
            }
            jobs_arr[i].dep_indices[k] = found_idx;
        }
        curr = curr->next;
    }

    /* Free the temporary JobNode list immediately in parse_config */
    free_job_nodes(head);

    *jobs_out = jobs_arr;
    *n_jobs_out = n_jobs;
    *num_workers_out = workers_val;
    return 0;

error:
    free_job_nodes(head);
    if (fp != NULL) {
        fclose(fp);
    }
    return 1;
}
