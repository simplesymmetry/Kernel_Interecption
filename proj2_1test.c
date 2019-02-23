/* Test case for system call 1.
 * Created by Thomas Graham on 9/12/18.
 */
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall1 377

// Data-struture to be used
struct processinfo{
    long state;
    pid_t pid;
    pid_t parent_pid;
    pid_t youngest_child;
    pid_t younger_sibling;
    pid_t older_sibling;
    uid_t uid;
};

// The acutal system call.
long testCall1 (void){
    return (long) syscall(__NR_cs3013_syscall1);
}

// The main just calling the testcall.
int main () {
    printf("\t Test Call 1: %ld\n", testCall1());
}
