		     +--------------------------+
       	       	     |		CS 140		|
		     | PROJECT 2: USER PROGRAMS	|
		     | 	   DESIGN DOCUMENT     	|
		     +--------------------------+

---- GROUP ----

>> Fill in the names and email addresses of your group members.

Aaron Roberts <public@document.omitted>
Teo Price-Broncucia <email@domain.example>

---- PRELIMINARIES ----

>> If you have any preliminary comments on your submission, notes for the
>> TAs, or extra credit, please give them here.

>> I don't believe we used any outside sources.

			   ARGUMENT PASSING
			   ================

---- DATA STRUCTURES ----

>> A1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

>> 'struct arg' was created. It contains char pointer to a value and a list_elem
>> 'struct process' was added to 'struct thread'. It contains a name pointer,
>> a list of args, as well as properties used for system calls and synchronization
>> discussed below.  

---- ALGORITHMS ----

>> A2: Briefly describe how you implemented argument parsing.  How do
>> you arrange for the elements of argv[] to be in the right order?
>> How do you avoid overflowing the stack page?

>> In 'process_execute(const char *args)' after allocating a new 'struct process'
>> a copy of the args is made using palloc_get_page(0) and then the args are
>> copied using strlcpy. Then the arguments including filename are tokenized using
>> strtok_r() and sotred in a struct arg, which is then pushed onto the args list
>> in the new struct process. This maintains the right order of arguments.Then
>> the pointer to the new struct process is passed as an argument to thread_create,
>> which calls start_process, this function being passed as an argument.
>> start_process then initializes the interrupt frame using memset and then
>> calls load, passing in the "struct process". "load" opens the executable and puts
>> it into the process struct. It verifies that the executable header matches
>> expected format, then reads in the headers, sets up the stack with the esp
>> register, and sets the eip register.

>> Not sure how we avoid overflowing the stack page. Possibly it refers to freeing the pages on failed
>> thread create or failed load.


---- RATIONALE ----

>> A3: Why does Pintos implement strtok_r() but not strtok()?

>> Because strtok() uses global data and is unsafe to use in threaded programs
>> such as kernel.

>> A4: In Pintos, the kernel separates commands into a executable name
>> and arguments.  In Unix-like systems, the shell does this
>> separation.  Identify at least two advantages of the Unix approach.

>> TODO Speed. This would allow the separation to happen without a context switch.
>> Works with fork -> exec design paradigm.

			     SYSTEM CALLS
			     ============

---- DATA STRUCTURES ----

>> B1: Copy here the declaration of each new or changed 'struct' or
>> 'struct' member, global or static variable, 'typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

>> 'struct list active_child processes' and 'struct list file_descriptors'
>> are added to 'struct thread'
>> In 'struct process', in addition to the items noted above, there are variables
>> for tid of child, bool parent_alive, semaphore on_load, semaphore on_exit, and
>> int exit_status. These handle synchronization and status passing between parent
>> and child.


>> B2: Describe how file descriptors are associated with open files.
>> Are file descriptors unique within the entire OS or just within a
>> single process?

>> There is a 'struct open_file' that stores a file descriptor (fd), a pointer
>> to the open file, and a list elem.
>> The file descriptors are unique within the whole OS, would eventually run out
>> of integers because they are not reused.

---- ALGORITHMS ----

>> B3: Describe your code for reading and writing user data from the
>> kernel.

>> The functions verify_user(), get_user(), and put_user() are wrapped in
>> access_user_data(). verify_user(*uaddr) checks that uaddr is below PHYS_BASE
>> and the page is not NULL. get_user(*dst, *usrc) writes byte at USRC and writes
>> it to kernel address dst. put_user instead writes a byte to a user address.
>> Each byte is checked verified and copied/written separately.

>> B4: Suppose a system call causes a full page (4,096 bytes) of data
>> to be copied from user space into the kernel.  What is the least
>> and the greatest possible number of inspections of the page table
>> (e.g. calls to pagedir_get_page()) that might result?  What about
>> for a system call that only copies 2 bytes of data?  Is there room
>> for improvement in these numbers, and how much?

>> TODO In our system each byte gets verified, and thus calls page_dir_page(), so I
>> think the answers here are 4,096 and 2. But it seems that there might be a
>> way to reduce this so only entries into a previously unchecked page gets checked.
>> so maybe they could both be reduced to a maximum of 2 but I'm not sure.

>> B5: Briefly describe your implementation of the "wait" system call
>> and how it interacts with process termination.

>> The wait call takes a child tid value. First it accesses the current threads'
>> list of child processes, and checks to see whether the child tid is in the list
>> if it is in the list the program then waits on the 'on_exit' semaphore for that
>> thread. In exit() a thread updates the exit value of the process struct before
>> calling sema_up(). This means even if the child process exited before the wait
>> the parent can still access the exit value, then free the process struct and
>> remove it from the child_list. If the parent is dead when the child calls
>> exit (known from parent_alive) the child will free the process struct itself.

>> B6: Any access to user program memory at a user-specified address
>> can fail due to a bad pointer value.  Such accesses must cause the
>> process to be terminated.  System calls are fraught with such
>> accesses, e.g. a "write" system call requires reading the system
>> call number from the user stack, then each of the call's three
>> arguments, then an arbitrary amount of user memory, and any of
>> these can fail at any point.  This poses a design and
>> error-handling problem: how do you best avoid obscuring the primary
>> function of code in a morass of error-handling?  Furthermore, when
>> an error is detected, how do you ensure that all temporarily
>> allocated resources (locks, buffers, etc.) are freed?  In a few
>> paragraphs, describe the strategy or strategies you adopted for
>> managing these issues.  Give an example.

>> I think our main strategy is to wrap all the address checking into one function
>> to improve clarity and avoid errors. This is verify_user. This returns a bool
>> indicating success or failure. Thus any call to verify_user should be paired with
>> code to free any dependent resources. This is often done by once again gathering
>> any resources together and freeing them as part of thread_exit. An example is
>> start_process. It checks for load_success after calling load, which handles
>> user verifying, if it failed it calls thread_exit,which frees resources.

---- SYNCHRONIZATION ----

>> B7: The "exec" system call returns -1 if loading the new executable
>> fails, so it cannot return before the new executable has completed
>> loading.  How does your code ensure this?  How is the load
>> success/failure status passed back to the thread that calls "exec"?

>> We have a semphore and int load_success variable in the process struct
>> that handle this. In start_process, if the status of load is set and sema_up
>> is called on the on_load semaphore. Thus thread_create won't continue until
>> load completes. Then thread_create returns the TID, which might be the TID_ERROR
>> which is then passed back from process_execute.

>> B8: Consider parent process P with child process C.  How do you
>> ensure proper synchronization and avoid race conditions when P
>> calls wait(C) before C exits?  After C exits?  How do you ensure
>> that all resources are freed in each case?  How about when P
>> terminates without waiting, before C exits?  After C exits?  Are
>> there any special cases?

>> From above: First it accesses the current threads'
>> list of child processes, and checks to see whether the child tid is in the list
>> if it is in the list the program then waits on the 'on_exit' semaphore for that
>> thread. In exit() a thread updates the exit value of the process struct before
>> calling sema_up(). This means even if the child process exited before the wait
>> the parent can still access the exit value, then free the process struct and
>> remove it from the child_list. If the parent is dead when the child calls
>> exit (known from parent_alive) the child will free the process struct itself.
>> Basically for freeing there are just a few cases. Parent frees a child that is
>> dead through the wait call, child frees itself when parent is dead, or parent
>> frees all dead children in its own exit.

---- RATIONALE ----

>> B9: Why did you choose to implement access to user memory from the
>> kernel in the way that you did?

>> TODO

>> B10: What advantages or disadvantages can you see to your design
>> for file descriptors?

>> Simplicity is an advantage but there are also limitations to the length of time
>> the system can run.


>> B11: The default tid_t to pid_t mapping is the identity mapping.
>> If you changed it, what advantages are there to your approach?

>> TODO Not changed.

			   SURVEY QUESTIONS
			   ================

Answering these questions is optional, but it will help us improve the
course in future quarters.  Feel free to tell us anything you
want--these questions are just to spur your thoughts.  You may also
choose to respond anonymously in the course evaluations at the end of
the quarter.

>> In your opinion, was this assignment, or any one of the three problems
>> in it, too easy or too hard?  Did it take too long or too little time?

>> Not too easy. Took significant time. We managed to finish but it seems like
>> the potential is high for an error early on that would make the rest of the
>> assignment really difficult or impossible.

>> Did you find that working on a particular part of the assignment gave
>> you greater insight into some aspect of OS design?

>> The system calls and argument passing seemed to give insight into a new concept.

>> Is there some particular fact or hint we should give students in
>> future quarters to help them solve the problems?  Conversely, did you
>> find any of our guidance to be misleading?

>> Do you have any suggestions for the TAs to more effectively assist
>> students, either for future quarters or the remaining projects?

>> huh?

>> Any other comments?
