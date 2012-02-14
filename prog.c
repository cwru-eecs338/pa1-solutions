/* Solutions for EECS 338 : PA 1 Spring 2012
 * Author : Tim Henderson 
 * Email : tim.tadh@gmail.com
 * License : Placed in the Public Domain
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CHILDFAIL 1
#define WAITFAIL 2
#define FORKFAIL 3
#define HOSTFAIL 4
#define UNREACHFAIL 5

/* wait_child(pid, exit_f)
 *  @param pid : the pid to wait for
 *  @param exit_f : a function pointer to the function to call should there be
 *                  an error. Takes an integer error code. Normall &exit.
 *
 *  uses waitpid to wait for the child to exit. checks for errors calls exit_f
 *  on error. 
 */
void wait_child(pid_t pid, void (*exit_f)(int)) {
  int status;
  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid() failed");
    exit_f(WAITFAIL);
  } else if (WEXITSTATUS(status) != 0) {
    fprintf(stderr, "a child (pid %d) failed (%d)\n", pid, WEXITSTATUS(status));
    exit_f(CHILDFAIL);
  }
}

/* pid_t checked_fork(exit_f)
 *  @param exit_f : a function pointer to the function to call should there be
 *                  an error. Takes an integer error code. Normall &exit.
 *  @returns : the pid of the forked process
 *
 *  Forks a new process and returns the pid. The pid is checked to see if it is
 *  negative indicating a failed fork. If it is it calls the error function
 *  (exit_f) with a code of 3 and returns with -1. Otherwise, it returns the new
 *  pid.
 */
pid_t checked_fork(void (*exit_f)(int)) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork() failed!");
    exit_f(FORKFAIL);
    return -1;
  }
  return pid;
}

/* print_proc_info()
 *  prints information on the current process.
 */
void print_proc_info() {
  char name[256]; // posix 2003 spec, MAX_HOST_NAME is undef on Linux
  if (gethostname(name, sizeof(name)) < 0){
    perror("gethostname failed");
    exit(HOSTFAIL);
  }
  name[sizeof(name)-1] = 0; // safety first!
  printf("Hostname:   %s\n", name);
  printf("Group ID:   %d\n", getgid());
  printf("User ID:    %d\n", getuid());
  printf("Process ID: %d\n", getpid());
  fflush(stdout);
}

/* countdown_child(i, counter)
 *  @param i : The current loop index.
 *  @param counter : The current value of the counter variable.
 *
 *  A helper function for countdown() contains code for the child.
 *  Executes execl() to call echo to print out a formatted info string
 *  containing i and counter
 */
int countdown_child(int i, int counter) {
  char str[1024];
  snprintf(str, 1023, "Index: %d Counter: %d", i, counter);
  execl("/bin/echo", "echo", str, (char *)NULL);
  return UNREACHFAIL; // should be unreachable
}

/* countdown()
 *  Performs the "countdown" section of the assignment
 *  Makes use of "wait_child" and "countdown_child"
 */
void countdown() {
  printf("Starting countdown...\n");
  fflush(stdout);

  int counter = 6;
  int i;
  pid_t pid;
  for (i = 5; i > 0; i--) {
    pid = checked_fork(&exit);
    if (pid == 0) {
      counter--;
      _exit(countdown_child(i, counter));
    } else {
      wait_child(pid, &exit);
    }
  }
}

/* int child_1()
 *  @returns : The return code for the child process
 *  Does the first half of the coversation
 */
int child_1() {
  pid_t pid = getpid();
  printf("Child %d: Hello\n", pid); fflush(stdout);
  sleep(2);
  printf("Child %d: What's new.\n", pid); fflush(stdout);
  sleep(2);
  printf("Child %d: Nothing new here either. Gotta Go!\n", pid); fflush(stdout);
  return 0;
}

/* int child_2()
 *  @returns : The return code for the child process
 *  Does the second half of the coversation
 */
int child_2() {
  pid_t pid = getpid();
  sleep(1);
  printf("Child %d: Hiya\n", pid); fflush(stdout);
  sleep(2);
  printf("Child %d: Nothing. You?\n", pid); fflush(stdout);
  sleep(2);
  printf("Child %d: Bye.\n", pid); fflush(stdout);
  return 0;
}

/* fork_kids(cpid1, cpid2)
 *  @output_param cpid1 : a pointer to pid_t. 
 *  @output_param cpid2 : a pointer to pid_t. 
 *
 *  Creates two child processes and stores their resulting pids in cpid1 and
 *  cpid2 respectively. If fork fails it will exit with error code 2. For cpid1
 *  it call child_1, for cpid2 it calls child_2 to execute the child code.
 */
void fork_kids(pid_t *cpid1, pid_t *cpid2, void (*exit_f)(int)) {
  (*cpid1) = -1; (*cpid2) = -1; // in case a something wierd happens they are set
                                // to a safe value of "fork failed"
  (*cpid1) = checked_fork(exit_f);
  if ((*cpid1) > 0) {
    (*cpid2) = checked_fork(exit_f);
    if ((*cpid2) == 0) {
      _exit(child_2());
    }
  } else if ((*cpid1) == 0) {
    _exit(child_1());
  }
}

/* _convo_exit(exit_code)
 *  @param exit_code : The exit error code
 *
 *  This is a specialized exit function (replacing the standard exit) for the
 *  'convo' function to use. Since convo has multiple children running at the
 *  same time, a failure of 'wait_child' should not cause the parent process to
 *  fail immediately. Instead, it should wait for the other child processes to
 *  finish before failing.
 *  
 *  This function makes use of the (int*)FAILURE global to store whether a
 *  function has failed. It is not thread safe.
 */
int *FAILURE = (int *)NULL;
void _convo_exit(int exit_code) {
  if (FAILURE == NULL) {
    fprintf(stderr, "FAILURE pointer not set. Defaulting to exit.\n");
    exit(exit_code);
  } else if ((*FAILURE) < exit_code) {
    (*FAILURE) = exit_code;
  }
}

/* convo()
 *  Forks to kids which have a conversation.
 */
void convo() {
  // Allocate (on the stack) a variable to store whether either child has failed
  int failure = 0;
  FAILURE = &failure; // cause the (int*)FAILURE global point at it

  // Allocate vars to store pids of the kids.
  pid_t first, second;
  fork_kids(&first, &second, &_convo_exit);

  // Use wait_child to wait for each child, using the custom exit function 
  // 'convo_exit' which logs in failure whether either child failed.
  // first or second could be -1 if checked_fork failed since _convo_exit
  // will cause checked_fork to return a -1. We don't want them to try and
  // wait in that situation.
  if (first > 0) { wait_child(first, &_convo_exit); }
  if (second > 0) { wait_child(second, &_convo_exit); }

  // If either child failed exit with failure as the status code (will be the
  // highest error code generated).
  if (failure != 0) {
    exit(failure);
  }

  // Finally, cleanup the global pointer.
  FAILURE = (int *)NULL;
}

/* main(argc, argv)
 *  @params std main params
 *
 *  kicks off the program.
 */
int main(int argc, char **argv) {
  print_proc_info();
  countdown();
  convo();
  printf("Parent Termination\n"); fflush(stdout);
  return 0;
}

