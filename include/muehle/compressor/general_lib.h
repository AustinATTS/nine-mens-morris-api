#ifndef MUEHLE_COMPRESSOR_GENERAL_LIB_H_
#define MUEHLE_COMPRESSOR_GENERAL_LIB_H_

#include <iostream>
#include <sstream>
#include <string>

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
  std::wostream* os_print; /* Steam for output. Default is cout */
  int verbosity;           /* Output detail level. Default is 2 */
  std::wstring name;       /* Name of the compression library */
  LibId id;                /* ID of the compression library */

  /* Constructor/Destructor */
  GeneralLib();
  ~GeneralLib();

 public:
  bool Print(std::wstringstream& ss, int level);
  bool SetVerbosity(int new_verbosity) {
    verbosity = new_verbosity;
    return true;
  };
  std::wstring const& GetName() { return name; };
  LibId const& GetLibId() { return id; };
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

}  // namespace compressor
}  // namespace muehle

#endif  // MUEHLE_COMPRESSOR_GENERAL_LIB_H_
