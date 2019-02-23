/*
*  mod.c
*  Operating Systems (CS 3013)
*  Project 2: Part one.
*
*  Copyright © 2018 Thomas Graham. All rights reserved.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>

/* Setup variables to be used. */
unsigned long **sys_call_table;
asmlinkage long (*ref_sys_open)(const char *fn, int flags, umode_t user);
asmlinkage long (*ref_sys_close)(unsigned int f);

asmlinkage long (new_sys_open)(const char *fn, int flags, umode_t user);
asmlinkage long (new_sys_close)(unsigned int f);

/* This function is used to find the system call table in memory. */
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

/* These functions are used to disable and enable page protection respectively. */
static void disable_page_protection(void) { 
    write_cr0 (read_cr0 () & (~ 0x10000));
}

static void enable_page_protection(void) {
    write_cr0 (read_cr0 () | 0x10000);
}

/* This function is used to start the action injection. */
static int __init interceptor_start(void) {
    // Find the table //
    if(!(sys_call_table = find_sys_call_table())) {
        return -1;
    }

    // Saving copies of the current functions. //
    ref_sys_open = (void *)sys_call_table[__NR_open];
    ref_sys_close = (void *)sys_call_table[__NR_close];
    
    // Begin protected area //
    disable_page_protection();
    sys_call_table[__NR_open] = (unsigned long *)new_sys_open;
    sys_call_table[__NR_close] = (unsigned long *)new_sys_close;
    enable_page_protection();

    // Prints information for debug //
    printk(KERN_INFO "Loaded module!");
    return 0;
}

/* This function is used to stop the injection and return back to normal. */
static void __exit interceptor_end(void) {
    // If no table found, then ret. //
    if(!sys_call_table)
        return;

    // Begin protected area to put the calls back to what they were. //
    disable_page_protection();
    sys_call_table[__NR_open] = (unsigned long *)ref_sys_open;
    sys_call_table[__NR_close] = (unsigned long *)ref_sys_close;
    enable_page_protection();

    // Print information for debug //
    printk(KERN_INFO "Unloaded module!");
}

/* This function is used to overwrite the system call sys_open to display a 
 * string of text to the syslog/demsg when a user opens a file.
 */
asmlinkage long new_sys_open(const char *fn, int flags, umode_t user) { 
    if (current_uid().val >= 1000){
        printk(KERN_INFO "User %d is opening file: %s \n", current_uid().val, fn);
    }
    //call the original system call, making to look inconspicuous. 
    return ref_sys_open(fn, flags, user);
}

/* This function is used to overwrite the system call sys_close to display a 
 * string of text to the syslog/demsg when a user closes a file.
 */
asmlinkage long new_sys_close(unsigned int f) {   
    if (current_uid().val >= 1000){
        printk(KERN_INFO "User %d is closing file: %u \n", current_uid().val, f);
    }
    //call the original system call, again allowing the user to continue.
    return ref_sys_close(f);
}

// Included module license and actually starting the interceptor.
MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);