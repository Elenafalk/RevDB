#include "debugger.hpp"
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv) {
  pid_t child_pid;
  Debugger db;

  if (argc < 2) {
    fprintf(stderr, "Expected a program name as argument\n");
    return -1;
  }

  while (1) {
    char input[1];
    cout << "Welcome to lnaDb. Would you like to single-step(s) or go to breakpoint(b)? Press e to exit." << endl;
    cin >> input;

    if (strcmp(input, "s") == 0) {
      child_pid = fork();
      if (child_pid == 0) {
        db.db_run_target(argv[1], argv); //child
      } else if (child_pid > 0) {
        db.db_run(child_pid, 0);
      } else {
        perror("fork");
        return -1;
      }
      return 0;
    } else if ((strcmp(input, "b") == 0)) {
      child_pid = fork();
      if (child_pid == 0) {
        db.db_run_target(argv[1], argv); //child
      } else if (child_pid > 0) {
        db.db_run(child_pid, 1);
      } else {
        perror("fork");
        return -1;
      }
      return 0;
    } else if ((strcmp(input, "e") == 0)) {
      cout << "Exiting" << endl;
      return 0;
    } else {
      cout << "Invalid input. Try again." << endl;
    }
  }
}
