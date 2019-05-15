#include "debugger.hpp"
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

//Launches the debugger, with or without breakpoints
void launch(char **argv, int launch_type, Dwarf_Debug dbg) {
  Debugger db;
  pid_t child_pid;

  child_pid = fork();
  if (child_pid == 0) {
    db.db_run_target(argv[1], argv); //child
  } else if (child_pid > 0) {
    db.db_run(child_pid, launch_type, dbg);
  } else {
    perror("fork");
    exit(-1);
  }
}

//TODO: an own dwarf init function
int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "Expected a program name as argument\n");
    return -1;
  }

  int fd = -1;
  Dwarf_Debug dbg = 0;
  Dwarf_Error err;
  const char *progname;

  progname = argv[1];
  if ((fd = open(progname, O_RDONLY)) < 0) {
    perror("open");
    return 1;
  }

  if (dwarf_init(fd, DW_DLC_READ, 0, 0, &dbg, &err) != DW_DLV_OK) {
    fprintf(stderr, "Failed DWARF initialization\n");
    exit(1);
  }

  while (1) {
    char input[1];
    cout << "Welcome to lnaDb. Would you like to single-step(s) or go to breakpoint(b)? Press e to exit." << endl;
    cin >> input;

    if (strcmp(input, "s") == 0) {
      launch(argv, 0, dbg);
      return 0;
    } else if ((strcmp(input, "b") == 0)) {
      launch(argv, 1, dbg);
      return 0;
    } else if ((strcmp(input, "e") == 0)) {
      cout << "Exiting" << endl;
      return 0;
    } else {
      cout << "Invalid input. Try again." << endl;
    }
  }
}
