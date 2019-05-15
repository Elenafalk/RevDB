#include "dwarf_function_information.hpp"
#include <libdwarf/libdwarf.h>

Dwarf_function_information::Dwarf_function_information(Dwarf_Addr m_addr, const char *m_name) {
  addr = m_addr;
  name = m_name;
}