#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <kern/fcntl.h>
#include <vfs.h>

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

int min_id = 2; 
void kill_kids (struct array * processes, int num_processes );
struct proc * gen_pid (pid_t pid);


struct proc * gen_pid (pid_t pid) {
  int array_location = pid - min_id;
  return array_get(allprocesses,array_location);
}
  
void kill_kids (struct array * processes, int num_processes ) {
  int count = 0;
  while (count < num_processes) {
     struct proc *current = array_get(processes, 0); // You can keep removing [0] since the array shifts entries down
     if (current == NULL) {
       //DO NOTHING
     }
     else {
       lock_release(current->exitinglock);
     }
     array_remove(processes, 0);
     count++;
  }
}

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);

  // -------------------------------------------------------------

  p->exitcode = _MKWAIT_EXIT(exitcode);
  p->exited = true;

  // Telling its kids they can die >:D

  struct array * cur_array = p->mykids;
  int num_kids = array_num(cur_array);

  kill_kids (cur_array, num_kids);

  cv_broadcast(p->mycv, p->waitinglock);
    
  lock_acquire(p->exitinglock);
  lock_release(p->exitinglock);
  // ---------------------------------------------------------------

  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
   *retval=curproc->myid;
   return 0; 
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }

  // -----------------------------------------

  lock_acquire(pidlock);
  struct proc *current = gen_pid(pid);
  lock_release(pidlock);

  lock_acquire(current->waitinglock);
  while(current->exited == false){
      cv_wait(current->mycv,current->waitinglock);
  }
  lock_release(current->waitinglock);

  // ----------------------------------------

  /* for now, just pretend the exitstatus is 0 */
  exitstatus=current->exitcode;
  
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

int sys_fork(struct trapframe *trap, pid_t *retval) {
   struct proc *myclone = proc_create_runprogram(curproc->p_name);

   as_copy(curproc_getas(),&(myclone->p_addrspace));

   struct trapframe *myclone_tf=kmalloc(sizeof(struct trapframe));

   memcpy(myclone_tf,trap,sizeof(struct trapframe));

   thread_fork(curthread->t_name, myclone, (void*)enter_forked_process, myclone_tf, 0);

   array_add(curproc->mykids,myclone,NULL);   
   
   // Abuse your child by even taking away its ability to truly die :)

   lock_acquire(myclone->exitinglock);
   *retval = myclone->myid;
    return 0;
}

int
sys_execv(char *program, char **args) {


  
    vaddr_t entry, stack;

    struct addrspace  * currentspace = curproc_getas();

    int result1, result2;
    int arguments = 0;

    while(args[arguments] != NULL){ // Determine number of arguments
      arguments = arguments + 1;
    }

    char **newargs = kmalloc(sizeof(char *) * (arguments + 1)); // Copy the args
    for (int i = 0; i < arguments; i++) {
      newargs[i] = kmalloc(sizeof(char) * (strlen(args[i]) + 1) );
      result1 = copyinstr((userptr_t)args[i], newargs[i], strlen(args[i]) + 1, NULL);
    }

    char *newprog = kmalloc(sizeof(char) * (strlen(program) + 1)); //Copy the program
    result2 = copyinstr((userptr_t)program, newprog, (strlen(program) + 1), NULL);

    newargs[arguments] = NULL; // NULL terminate array

    int outcome1, outcome2, outcome3; // Open file
    struct vnode * virtualfile;
    outcome1 = vfs_open(newprog, O_RDONLY, 0, &virtualfile);

    struct addrspace *newaddrspace; // New address space
    newaddrspace = as_create();

    curproc_setas(newaddrspace); // Switch to new space
    as_activate();
    
    outcome2 = load_elf(virtualfile, &entry); // Load elf and close file
    vfs_close(virtualfile);

    outcome3 = as_define_stack(newaddrspace, &stack); // Set stack of new process

    as_destroy(currentspace);

    // KASSE

    vaddr_t argstack[arguments + 1]; // 8 bit align 
    //if ((stack % 8) != 0) {
    //  vaddr_t substack = stack - 8;
    //  stack = ROUNDUP(substack, 8);
    //}

    int cut = arguments - 1;

    for (int i = cut; i > -1; i--) { //Strings to stack
      int len = strlen(newargs[i]) + 1; 
      stack = stack - len; 
      copyoutstr(newargs[i], (userptr_t)stack, len, NULL);
      argstack[i] = stack;
    }

    if ((stack % 4) != 0) { // Align strings
      vaddr_t substack = stack - 4;
      stack = ROUNDUP(substack, 4);
    }

    // stack = stack - 4; // For extra NULL

    //argstack[arguments] = NULL;
    argstack[arguments] = 0;

    for (int i = cut + 1; i > -1; i--) { // Put string pointers on stack
      int rounded =  ROUNDUP(sizeof(vaddr_t), 4); 
      stack = stack - rounded;
      copyout(&argstack[i], (userptr_t)stack, sizeof(vaddr_t));
    }
  
    

    enter_new_process(arguments, (userptr_t)stack, stack, entry); 
panic("enter_new_process returned\n");
	return EINVAL;

}
