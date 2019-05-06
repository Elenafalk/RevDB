#pragma once
#include <sys/types.h>

class Breakpoint {
private:
  unsigned long orig_data;
  pid_t pid;
  void *address;

public:
  Breakpoint(pid_t pid_, void *address_);
  void set_data(unsigned long newData);
  pid_t get_pid();
  void *get_address();
  unsigned long get_data();
};