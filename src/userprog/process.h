#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct arg
{
  char *value;
  struct list_elem elem;
};
struct process
{
  char *name;
  struct list args;
  struct parent_child_synch *pc_synch;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
