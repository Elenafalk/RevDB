#pragma once
#include "breakpoint.hpp"
#include <libdwarf/libdwarf.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/user.h>

class Debugger {
public:
  Debugger();
  unsigned long get_rip(pid_t pid);
  void db_print(const char *format, ...);
  void db_run(pid_t child_pid, int step_type);
  void db_run_target(const char *programname, char *const argv[]);
  Breakpoint create_bp(pid_t pid, void *address);
  void enable_bp(Breakpoint *bp);
  void disable_bp(Breakpoint *bp);
  int db_resume_from_bp(pid_t pid, Breakpoint *bp, int resume_type);
  void db_run_single(pid_t child_pid);
};