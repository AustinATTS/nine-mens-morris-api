#include "muehle/compressor/general_lib.h"

namespace muehle {

/* GeneralLib class constructor */
compressor::GeneralLib::GeneralLib() {
  /* Init default values */
  os_print = &std::wcout;
  verbosity = 3;
}

/* GeneralLib class destructor */
compressor::GeneralLib::~GeneralLib() {}

bool compressor::GeneralLib::Print(std::wstringstream& ss, int level) {
  if (verbosity > level) {
    *os_print << ss.str();
    return true;
  } else {
    return false;
  }
}

}  // namespace muehle
