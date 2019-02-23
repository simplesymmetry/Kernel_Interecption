#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall2 378

/* The setup of the structure */
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

/* The function used to make the system call. */
long test(struct processinfo *pf) {
        return (long) syscall(__NR_cs3013_syscall2, pf);
}

/* The main function creating a child process and testing it. */
int main(){
    struct processinfo *pf = malloc(sizeof(struct processinfo));
    struct processinfo *p2 = malloc(sizeof(struct processinfo));
    int pid;
    if ((pid = fork()) < 0){
        printf("Child not created");
    }
    if (pid == 0){
        printf("Child Process: \n");
        if (test(pf))
            printStuff(pf);
            exit(0);
    }
    else{
        sleep(6);
        printf("\nParent Process: \n");
            test(p2);
            printStuff(p2);
            return 0;
    }
}

void printStuff(struct processinfo *pf)
{
    printf("\nThe state of the current process:\t\t %li\n", pf->state);
    printf("The pid of the current process:\t\t\t %i\n", pf->pid);
    printf("The parent pid of the current process:\t\t %i\n", pf->parent_pid);
    printf("The youngest child of the current process:       %i\n", pf->youngest_child);
    printf("The younger silbing of the current process:      %i\n", pf->younger_sibling);
    printf("The older sibiling of the current process:       %i\n", pf->older_sibling);
    printf("The uid of the current process:\t\t\t %i\n", pf->uid);
    printf("The size of the struct is:\t\t\t %i \n", sizeof(pf));
    printf("The start_time of the current process:           %li\n", pf->start_time);
    printf("The user_time of the current process:            %li\n", pf->user_time);
    printf("The sys_time of the current process:             %li\n", pf->sys_time);
    printf("The cutime of the current process:               %li\n", pf->cutime);
    printf("The cstime of the current process:               %li\n", pf->cstime);

    free(pf);
}
