/* Code sample: using ptrace for simple tracing of a child process.
**
** Note: this was originally developed for a 32-bit x86 Linux system; some
** changes may be required to port to x86-64.
**
** Eli Bendersky (http://eli.thegreenplace.net)
** This code is in the public domain.
*/
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

/* Print a message to stdout, prefixed by the process ID
*/
void procmsg(const char *format, ...) {
  va_list ap;
  fprintf(stdout, "[%d] ", getpid());
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
}

void run_target(const char *programname, char *const argv[]) {
  procmsg("target started. will run '%s'\n", programname);

  /* Allow tracing of this process */
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    perror("ptrace");
    return;
  }

  /* Replace this process's image with the given program */
  execv(programname, argv);
}

void run_debugger(pid_t child_pid) {
  int wait_status;
  unsigned icounter = 0;
  procmsg("debugger started\n");

  /* Wait for child to stop on its first instruction */
  wait(&wait_status);

  while (WIFSTOPPED(wait_status)) {
    icounter++;
    /* Make the child execute another instruction */
    if (ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) < 0) {
      perror("ptrace");
      return;
    }

    /* Wait for child to stop on its next instruction */
    wait(&wait_status);
  }

  procmsg("the child executed %u instructions\n", icounter);
}

int main(int argc, char **argv) {
  pid_t child_pid;

  if (argc < 2) {
    fprintf(stderr, "Expected a program name as argument\n");
    return -1;
  }

  child_pid = fork();
  if (child_pid == 0)
    run_target(argv[1], argv);
  //run_target();
  else if (child_pid > 0)
    run_debugger(child_pid);
  else {
    perror("fork");
    return -1;
  }

  return 0;
}