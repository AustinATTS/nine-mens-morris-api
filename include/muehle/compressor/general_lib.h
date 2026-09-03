#ifndef MUEHLE_COMPRESSOR_GENERAL_LIB_H_
#define MUEHLE_COMPRESSOR_GENERAL_LIB_H_

#include <iosfwd>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif /* NOMINMAX */
#include <windows.h>
#endif /* _WIN32 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace muehle {
namespace compressor {

/* Base class for all compression libraries, providing a common interface */
class GeneralLib {
 public:
  /* List of all available compression libraries */
  enum class LibId {
    undefined,
    uncompressed,
    win_comp_api,
    bzip2,
  };

 protected:
  /* Variables */

  /* Steam for output. Default is cout */
  std::wostream* os_print;

  /* Output detail level. Default is 2 */
  int verbosity;

  /* Name of the compression library */
  std::wstring name;

  /* ID of the compression library */
  LibId id;

  /* Constructor/Destructor */
  GeneralLib();
  ~GeneralLib();

 public:
  bool Print(std::wstringstream& ss, int level);
  bool SetVerbosity(int new_verbosity) {
    verbosity = new_verbosity;
    return true;
  };
  std::wstring const& GetName() {
    return name;
  };
  LibId const& GetLibId() {
    return id;
  };
  virtual bool Compress(void* compressed_data, void* source_data,
                        unsigned int n_bytes_to_compress,
                        unsigned int& n_bytes_compressed) {
    return false;
  };
  virtual bool Decompress(void* dest_data, void* compressed_data,
                          unsigned int n_bytes_compressed,
                          unsigned int& n_bytes_decompressed) {
    return false;
  };
  virtual long long EstimateMaxSizeOfCompressedData(
      long long amount_uncompressed_data) {
    return 0;
  };
};

} /* namespace compressor */
} /* namespace muehle */

#endif /* MUEHLE_COMPRESSOR_GENERAL_LIB_H_ */
