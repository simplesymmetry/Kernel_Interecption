/*
*  Part2.c
*  Operating Systems (CS 3013)
*  Project 2: Part 2.
*
*  Copyright © 2018 Thomas Graham. All rights reserved.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <asm/current.h>
#include <asm-generic/cputime.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/cputime.h>
#include <linux/slab.h>

/* Setup a variable to keep store a pointer to the call table. */ 
unsigned long **sys_call_table;

/* Store a reference to the old syscall2 */
asmlinkage long (*ref_sys_cs3013_syscall2)(void);

/* This will be the structure where the information will be stored. */
struct processinfo{
    long state;
    pid_t pid;
    pid_t parent_pid;
    pid_t youngest_child;
    pid_t younger_sibling;
    pid_t older_sibling;
    uid_t uid;
    long long start_time;
    long long user_time;
    long long sys_time;
    long long cutime;
    long long cstime;
};

/* This function is used to overwrite the system call sys_cs3013_syscall2 to 
 * return back some information about the current process and it's sibilings.
 * @info - a ptr to the structure that will we copy the information to afterwards.
 * @return - 0 if the copy process works correctly (safely).
 */
asmlinkage long new_sys_cs3013_syscall2(struct processinfo *info)
{
    /* Setup variables to be used */
    /* just using c for ease */
    struct task_struct *c = current;
    struct list_head *head;
    struct processinfo pi;

    /* Setup variables */
    pi.pid = c->pid;
    pi.state = c->state;
    pi.parent_pid = c->parent->pid;
    pi.cutime = 0;
    pi.cstime = 0;
    pi.sys_time = cputime_to_usecs(&c->stime);
    pi.user_time = cputime_to_usecs(&c->utime);
    pi.start_time = timespec_to_ns(&c->start_time);
    //Set them to -1 instead of returning them to make code flow better
    pi.youngest_child = -1;
    pi.younger_sibling = -1;
    pi.older_sibling = -1;

    //This is how I figured out to get uid.
    pi.uid = (c->cred)->uid.val;

    /* The first thing to do is to loop through the children by creating a temp
     * child to use as a holder, then increment the time, and finally set the
     * youngest child variable in the struct to the temp.
     * (&c->children) - list of children processes 
     */
    if (!(list_empty(&c->children))) {
        list_for_each(head, &(c->children)){

            struct task_struct tempChild = *list_entry(head, struct task_struct, sibling);
            pi.cutime += cputime_to_usecs(&tempChild.utime);
            pi.cstime += cputime_to_usecs(&tempChild.stime);

            if (timespec_to_ns(&tempChild.start_time) > pi.start_time){
                pi.youngest_child = tempChild.pid;
            }
        }
    }

    /* The next thing is basically the same for the siblings but we have to check
     * the comparison each time the loop runs. Then set the variable appropriately.
     * (&c->sibling) - list of sibling processes 
     */ 
    if (!(list_empty(&c->sibling))){
        list_for_each(head, &(c->sibling)){
            struct task_struct tempSib = *list_entry(head, struct task_struct, sibling);

            //Check the age of the processes 
            if (timespec_to_ns(&tempSib.start_time) < pi.start_time && (&tempSib > 0)){
                pi.older_sibling = tempSib.pid;
            }
            
            if (timespec_to_ns(&tempSib.start_time) < pi.start_time && (&tempSib > 0)){
                pi.younger_sibling = tempSib.pid;
            }
        }
    }


    /* copy_to_user(info, &pi, sizeof(pi) - Attempt to safely copy our struct 
     * from kernel back to the user space. If sucess ret 0 else ret size of 
     * struct that didn't write. Also check that 
     * info is not empty.
     */
    if (copy_to_user(info, &pi, sizeof(pi)))
        return EFAULT;
    else if (!info)
        return -1;
}

/* This function is used to find the system call table.
 * This has been untouched as per directions. 
 */
static unsigned long **find_sys_call_table(void) {
    unsigned long int offset = PAGE_OFFSET;
    unsigned long **sct;
    
    while (offset < ULLONG_MAX) {
        sct = (unsigned long **)offset;

        if (sct[__NR_close] == (unsigned long *) sys_close) {
            printk(KERN_INFO "Table at memory address: 0x%02lX",
            (unsigned long) sct);
            
            return sct;
        }
    
    offset += sizeof(void *);
    }
    return NULL;
}

/* These functions are used to disable and enable page protection respectively.
 * They basically allow me to write to memory even if it's "read only". This is 
 * the reason the entire program writes/works.
 */
static void disable_page_protection(void) {
    write_cr0 (read_cr0 () & (~ 0x10000));
}
static void enable_page_protection(void) {
    write_cr0 (read_cr0 () | 0x10000);
}

/* This function is used to start the injection/interceptor. */
static int __init interceptor_start(void) {
    // If can't find the table, ret //
    if(!(sys_call_table = find_sys_call_table())) {
        return -1;
    }

    // Saving copies of the current functions. //
    ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];
    
    // Begin protected area //
    disable_page_protection();
    sys_call_table[__NR_cs3013_syscall2] = (unsigned long *) new_sys_cs3013_syscall2;
    enable_page_protection();

    // Prints information for debug //
    printk(KERN_INFO "Loaded module!");
    return 0;
}

/* This function is used to stop the injection and return back to normal. */
static void __exit interceptor_end(void) {

    /* If no table found, then just ret. */
    if(!sys_call_table)
        return;

    /* Begin protected area again, but this time to put the calls back to what they were. */
    disable_page_protection();
    sys_call_table[__NR_cs3013_syscall2] = (unsigned long *) ref_sys_cs3013_syscall2;
    enable_page_protection();

    /* Print information for debug. */
    printk(KERN_INFO "Unloaded module!");
}

/* Included module license, and the calls to actually start the interceptor. */
MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);