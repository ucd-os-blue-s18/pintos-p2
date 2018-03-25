#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// User memory access functions

/* Returns true if UADDR is a valid, mapped user address,
   false otherwise. */
static bool verify_user (const void *uaddr) {
  return (uaddr < PHYS_BASE
          && pagedir_get_page (thread_current ()->pagedir, uaddr) != NULL);
}
 
/* Copies a byte from user address USRC to kernel address DST.
   USRC must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static inline bool get_user (uint8_t *dst, const uint8_t *usrc) {
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static inline bool put_user (uint8_t *udst, uint8_t byte) {
  int eax;
  asm ("movl $1f, %%eax; movb %b2, %0; 1:"
       : "=m" (*udst), "=&a" (eax) : "q" (byte));
  return eax != 0;
}

// Wrapper around get_user to get SIZE number of bytes
static bool get_user_data (void *dst, const void *usrc, size_t size) 
{
  uint8_t *dst_byte = (uint8_t *)dst;
  uint8_t *usrc_byte = (uint8_t *)usrc;

  for (unsigned i=0; i < size; i++)
  {
    if(!(verify_user(usrc_byte+i) && get_user(dst_byte+i, usrc_byte+i)))
    {
      return false;
    }
  }
  return true;
}

// System calls
// TODO: remove all the UNUSED descriptors as these are implemented.
//       they're only used to clear out all the compiler warnings
static int sys_write (int arg0, int arg1, int arg2)
{
  int fd = arg0;
  const void *buffer = (const void*)arg1;
  unsigned length = (unsigned)arg2;

  // writing to stdout
  if(fd == 1)
  {
    putbuf(buffer, length);
    return length;
  }
  // TODO: handle other fds
  else
  {
    return 0;
  }
}

static int sys_halt(int arg0 UNUSED, int arg1 UNUSED, int arg2 UNUSED)
{
  shutdown_power_off();
}

static int sys_exit (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{
  int status = arg0;

  thread_current()->p->exit_status = status;
  printf("%s: exit(%d)\n", thread_current()->name, status);

  // when running with USERPROG defined, thread_exit will also call
  // process_exit 
  // TODO: deal with freeing resources such as locks
  thread_exit();
}

static int sys_exec (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  const char *args = (const char*)arg0;

  return process_execute(args);
}

static int sys_wait (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED pid_t pid = arg0;

  return 0; 
}

static int sys_create (int arg0, int arg1, int arg2 UNUSED)
{ 
  UNUSED const char *file = (const char*)arg0;
  UNUSED unsigned initial_size = (unsigned)arg1;

  return 0; 
}

static int sys_remove (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED const char *file = (const char *)arg0;

  return 0;
}

static int sys_open (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED const char *file = (const char *)arg0;

  return 0; 
}

static int sys_filesize (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED int fd = arg0;
  
  return 0; 
}

static int sys_read (int arg0, int arg1, int arg2)
{ 
  UNUSED int fd = arg0;
  UNUSED void *buffer = (void*)arg1;
  UNUSED unsigned length = (unsigned)arg2;
  
  return 0; 
}

static int sys_seek (int arg0, int arg1, int arg2 UNUSED)
{ 
  UNUSED int fd = arg0;
  UNUSED unsigned position = (unsigned)arg1;
  
  return 0; 
}

static int sys_tell (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED int fd = arg0;
  
  return 0; 
}

static int sys_close (int arg0, int arg1 UNUSED, int arg2 UNUSED)
{ 
  UNUSED int fd = arg0;
  
  return 0; 
}

// Syscall dispatch table
typedef int syscall_func(int, int, int);
struct syscall
{
  int argc;
  syscall_func *func;
};

static struct syscall syscall_array[] =
{
  {0, sys_halt}, {1, sys_exit}, {1, sys_exec}, {1, sys_wait},
  {2, sys_create}, {1, sys_remove}, {1, sys_open}, {1, sys_filesize},
  {3, sys_read}, {3, sys_write}, {2, sys_seek}, {1, sys_tell}, {1, sys_close}
};

static void
syscall_handler (struct intr_frame *f) 
{
  int syscall_number = 0;
  int args[3] = {0,0,0};

  if(!get_user_data(&syscall_number, f->esp, 4))
    sys_exit(-1, 0, 0);

  // fallthrough here is intentional
  // TODO: pagefaults may require more handling than this
  struct syscall *sc = &syscall_array[syscall_number];
  switch(sc->argc)
  {
    case 3:
      if(!get_user_data(&args[2], f->esp+12, 4))
        sys_exit(-1, 0, 0);

    case 2:
      if(!get_user_data(&args[1], f->esp+8, 4))
        sys_exit(-1, 0, 0);

    case 1:
      if(!get_user_data(&args[0], f->esp+4, 4))
        sys_exit(-1, 0, 0);
  }
  f->eax = sc->func(args[0], args[1], args[2]);
  return;
}
