#include "breakpoint.hpp"

Breakpoint::Breakpoint(pid_t pid_, void *address_) {
  pid = pid_;
  address = address_;
}

void Breakpoint::set_data(unsigned long newData) {
  orig_data = newData;
}
pid_t Breakpoint::get_pid() {
  return pid;
}
void *Breakpoint::get_address() {
  return address;
}
unsigned long Breakpoint::get_data() {
  return orig_data;
}