#include "debugger.hpp"
#include "breakpoint.hpp"
#include "dwarf_function_information.hpp"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <list>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <syscall.h>
#include <unistd.h>

using namespace std;

Debugger::Debugger() {}

Dwarf_function_information Debugger::db_find_function(Dwarf_Die the_die) {
  char *die_name = 0;
  const char *tag_name = 0;
  Dwarf_Error err;
  Dwarf_Half tag;
  Dwarf_Attribute *attrs;
  Dwarf_Addr lowpc;
  list<Dwarf_Addr> addresses;
  Dwarf_Signed attrcount, i;
  int rc = dwarf_diename(the_die, &die_name, &err);
  Dwarf_function_information df = Dwarf_function_information(0, 0);

  if (rc == DW_DLV_ERROR)
    db_print("Error: DWARF DIE name");
  else if (rc == DW_DLV_NO_ENTRY) { //Return if no entry
    return df;
  }

  if (dwarf_tag(the_die, &tag, &err) != DW_DLV_OK)
    db_print("Error: DWARF TAG");

  //Return if it's not a subprogram(aka function)
  if (tag != DW_TAG_subprogram) {
    return df;
  }

  //Error with name of function
  if (dwarf_get_TAG_name(tag, &tag_name) != DW_DLV_OK)
    db_print("Error: DWARF TAG name");

  //Get the attributes of the DIE
  if (dwarf_attrlist(the_die, &attrs, &attrcount, &err) != DW_DLV_OK)
    db_print("Error: DWARF attrlist");

  //Determine the address of the function
  for (i = 0; i < attrcount; ++i) {
    Dwarf_Half attrcode;
    if (dwarf_whatattr(attrs[i], &attrcode, &err) != DW_DLV_OK)
      db_print("Error: DWARF whatattr");

    if (attrcode == DW_AT_low_pc) {
      dwarf_formaddr(attrs[i], &lowpc, &err);
    }
  }
  Dwarf_function_information dfi = Dwarf_function_information(lowpc, die_name);
  return dfi;
}

//List all functions in child
list<Dwarf_function_information> Debugger::db_list_functions(Dwarf_Debug dbg) {
  Dwarf_Unsigned cu_header_length, abbrev_offset, next_cu_header;
  Dwarf_Half version_stamp, address_size;
  Dwarf_Error err;
  Dwarf_Die no_die = 0, cu_die, child_die;
  list<Dwarf_function_information> addresses;

  //Compilation Unit header
  if (dwarf_next_cu_header(
          dbg,
          &cu_header_length,
          &version_stamp,
          &abbrev_offset,
          &address_size,
          &next_cu_header,
          &err) == DW_DLV_ERROR)
    db_print("Error: no DWARF CU header");

  //Find sibling of CU - the DIE
  if (dwarf_siblingof(dbg, no_die, &cu_die, &err) == DW_DLV_ERROR)
    db_print("Error: no sibling of CU");

  //Children of the CU DIE
  if (dwarf_child(cu_die, &child_die, &err) == DW_DLV_ERROR)
    db_print("Error: no child of CU DIE");

  //Check all child DIEs
  while (1) {
    int rc;
    addresses.push_back(db_find_function(child_die));

    rc = dwarf_siblingof(dbg, child_die, &child_die, &err);

    if (rc == DW_DLV_ERROR)
      db_print("Error: no sibling of DIE");
    else if (rc == DW_DLV_NO_ENTRY) { //no more siblings
      break;
    }
  }
  return addresses;
}

//Returns Relative Instruction Pointer of child (for x64)
unsigned long Debugger::get_rip(pid_t pid) {
  struct user_regs_struct registers;
  ptrace(PTRACE_GETREGS, pid, 0, &registers);
  return registers.rip;
}

// Print a message to stdout, prefixed by the process ID
void Debugger::db_print(const char *format, ...) {
  va_list ap;
  fprintf(stdout, "[%d] ", getpid());
  va_start(ap, format);
  vfprintf(stdout, format, ap);
  va_end(ap);
}

//Back up the original data at the address, then insert trap instruction
void Debugger::enable_bp(Breakpoint *bp) {
  assert(bp); //Ensure that bp exists
  long data = ptrace(PTRACE_PEEKTEXT, bp->get_pid(), bp->get_address(), 0);

  if (data == -1 && errno != 0) { //Check to see if data has any content
    db_print("Error: no data at address. Could not set breakpoint. Try another address.\n");
    exit(1);
  }
  bp->set_data(data);
  db_print("Data saved: 0x%lx, at 0x%08X\n", bp->get_data(), bp->get_address());
  ptrace(PTRACE_POKETEXT, bp->get_pid(), bp->get_address(), (bp->get_data() & 0xFFFFFF00) | 0xCC); //0xCC is int3 - sigtrap
}

//Replaces the trap instruction byte with the original data
void Debugger::disable_bp(Breakpoint *bp) {
  db_print("Disabling breakpoint\n");
  assert(bp); //Ensure that bp exists
  ptrace(PTRACE_POKETEXT, bp->get_pid(), bp->get_address(), bp->get_data());
  db_print("Data restored: 0x%lx, at 0x%08X\n", bp->get_data(), bp->get_address());
}

unsigned long Debugger::db_find_addr(Dwarf_Debug dbg) {
  unsigned long return_addr = 0;
  list<Dwarf_function_information> all_functions = db_list_functions(dbg);

  while (1) {
    string input;
    cout << "Please type the name of the function you wish to start at. Press e to exit." << endl;
    cin >> input;
    if (input.compare("e") == 0) {
      db_print("Exiting\n");
      exit(1);
    }
    for (Dwarf_function_information function : all_functions) {
      if (function.name != 0 && input.compare(function.name) == 0) {
        return function.addr;
      }
    }
    if (return_addr == 0) {
      db_print("Error: no such function.\n");
    }
  }
  db_print("Returning: 0x%lx\n", return_addr);
  return return_addr;
}

//Run debugger
void Debugger::db_run(pid_t child_pid, int step_type, Dwarf_Debug dbg) {
  int wait_status;
  db_print("Debugger started\n");
  Dwarf_Error err;

  if (step_type == 0) {
    db_run_single(child_pid);
  }

  //Child stops on first instruction and waits
  wait(&wait_status);
  db_print("Child started at: RIP = 0x%08x\n", get_rip(child_pid));

  //unsigned long addr = 0x400727; //0x400727 for main on debuggee
  //unsigned long addr = 0x400080; //first line in hello
  //unsigned long addr = 0x40079b; //40079b main in test.cpp

  unsigned long addr = db_find_addr(dbg);

  db_print("Original data at 0x%08X: 0x%lx\n", addr, ptrace(PTRACE_PEEKTEXT, child_pid, (void *)addr, 0) & 0xff);

  //Create and enable breakpoint
  Breakpoint *bp = new Breakpoint(child_pid, (void *)addr);
  enable_bp(bp);
  db_print("Breakpoint created at address = 0x%08X\n", bp->get_address());
  db_print("New data at 0x%08x: 0x%lx\n", bp->get_address(), (ptrace(PTRACE_PEEKTEXT, child_pid, bp->get_address(), 0)));

  //Continue running to breakpoint
  ptrace(PTRACE_CONT, child_pid, 0, 0);
  wait(&wait_status);
  if (WIFSTOPPED(wait_status)) {
    db_print("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
  } else {
    perror("wait");
    return;
  }

  db_print("Child stopped at breakpoint. RIP = 0x%08X\n", get_rip(child_pid));

  while (1) {
    cout << "Would you like to continue(c) or single-step(s) from here? Press e to exit." << endl;
    char input[1];
    cin >> input;
    if (strcmp(input, "s") == 0) {
      db_resume_from_bp(child_pid, bp, 0);
      db_run_single(child_pid);
    } else if (strcmp(input, "c") == 0) {
      int resumed = db_resume_from_bp(child_pid, bp, 1);
      if (resumed == 0) {
        db_print("0");
      } else if (resumed == 1) {
        db_print("1");
      } else {
        db_print("else");
      }
      return;
    } else if (strcmp(input, "e") == 0) {
      cout << "Exiting" << endl;
      if (dwarf_finish(dbg, &err) != DW_DLV_OK) {
        fprintf(stderr, "Failed DWARF finalization\n");
        exit(1);
      }
      exit(0);
    } else {
      cout << "Invalid input. Try again." << endl;
    }
  }
}

//Single-steps one line at a time from current RIP until program ends or user exits
void Debugger::db_run_single(pid_t child_pid) {
  int wait_status;

  cout << "Press n for next line and e to exit. Alternatively continue(c)" << endl;
  char input[1];
  while (1) { //is this needed?
    wait(&wait_status);
    db_print("Child is at: RIP = 0x%08x\n", get_rip(child_pid));
    if (WIFEXITED(wait_status)) {
      cout << "Program ended." << endl;
      exit(0);
    }

    //TODO:print out information about

    while (1) {
      cin >> input;
      if (strcmp(input, "n") == 0) {
        ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0);
        break;
      } else if (strcmp(input, "e") == 0) {
        cout << "Exiting." << endl;
        exit(0);
      } else if (strcmp(input, "c") == 0) {
        cout << "Continuing." << endl;
        if (ptrace(PTRACE_CONT, child_pid, 0, 0) < 0) {
          perror("ptrace");
          exit(-1);
        }

        wait(&wait_status);
        if (WIFEXITED(wait_status)) {
          db_print("Exited.\n");
          exit(0);
        }
      } else {
        cout << "Invalid input. Try again." << endl;
      }
    }
  }
}

//Disables breakpoint, deletes it, and sets RIP one back before continuing, either normally or as single-step
int Debugger::db_resume_from_bp(pid_t pid, Breakpoint *bp, int resume_type) {
  struct user_regs_struct regs;
  int wait_status;
  ptrace(PTRACE_GETREGS, pid, 0, &regs);

  //Ensure we stopped at the right RIP
  assert(regs.rip == (unsigned long)bp->get_address() + 1);

  disable_bp(bp);
  db_print("Deleted breakpoint\n");
  delete bp; //Faster than free(). Frees up memory after creating breakpoint

  //RIP is at one after the breakpoint. We need to set it back by one
  db_print("Current RIP = 0x%08X\n", regs.rip);
  regs.rip -= 1;
  //Update the registers and continue running
  ptrace(PTRACE_SETREGS, pid, 0, &regs);
  db_print("Continuing from RIP = 0x%08X\n", get_rip(pid));

  if (resume_type == 1) {
    if (ptrace(PTRACE_CONT, pid, 0, 0) < 0) {
      perror("ptrace");
      return -1;
    }
    //Program ends
    wait(&wait_status);
    if (WIFEXITED(wait_status))
      return 0;
    else if (WIFSTOPPED(wait_status)) {
      return 1;
    } else
      return -1;
  } else {
    ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
    return 1;
  }
}

//Run target process
void Debugger::db_run_target(const char *programname, char *const argv[]) {
  db_print("Target started. will run '%s'\n", programname);

  //Allows for tracing of target
  if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
    perror("ptrace");
    return;
  }

  // Replace this target's image with the given program
  execv(programname, argv);
}