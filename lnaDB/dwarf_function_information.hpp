#pragma once
#include <libdwarf/libdwarf.h>

class Dwarf_function_information {
public:
  Dwarf_function_information(Dwarf_Addr addr, const char *name);
  Dwarf_Addr addr;
  const char *name;
};