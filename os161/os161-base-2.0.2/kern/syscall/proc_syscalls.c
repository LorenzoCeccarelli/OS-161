#include "syscall.h"
#include "thread.h"
#include "proc.h"
#include "addrspace.h"
#include "lib.h"
#include "current.h"


void sys__exit(int status){
    #if WAIT_PROCESS
    struct proc *proc = curproc;
    proc->exit_code=status;
    proc_remthread(curthread);
    //kprintf("Processo %p svegliato\n",proc);
    V(proc->sem_wait_proc);
    #else
    struct addrspace *as=proc_getas();
    as_destroy(as);
    #endif
    thread_exit();
    (void) status; 

}
#if WAIT_PROCESS
int sys_waitpid(int pid){
    int status;
    struct proc *proc=get_proc_by_pid(pid);
	KASSERT(proc!=NULL);
    kprintf("Process %p in attesa\n",proc);
	P(proc->sem_wait_proc);
	status=proc->exit_code;
	kprintf("exit code- proc_wait %d\n",status);
	kprintf("-----Process destroy-------\n");
	proc_destroy(proc);
	kprintf("-----End of destroy-----\n");
	return status;
	
}

int sys_getpid(struct proc* p){
    
    return p->pid;
}
#endif