#ifndef MUEHLE_COMPRESSOR_WIN_COMP_API_H_
#define MUEHLE_COMPRESSOR_WIN_COMP_API_H_

#ifdef _WIN32

#include "muehle/compressor/general_lib.h"
#include <compressapi.h>

namespace muehle {
namespace compressor {

class WinCompApi : public GeneralLib {
 public:
  WinCompApi();
  ~WinCompApi();
  bool Compress(void* compressed_data, void* source_data,
                unsigned int n_bytes_to_compress,
                unsigned int& n_bytes_compressed) override;
  bool Decompress(void* dest_data, void* compressed_data,
                  unsigned int n_bytes_compressed,
                  unsigned int& n_bytes_decompressed) override;
  long long EstimateMaxSizeOfCompressedData(
      long long amount_uncompressed_data) override;

 private:
  COMPRESSOR_HANDLE compressor = NULL;
  DECOMPRESSOR_HANDLE decompressor = NULL;
  PBYTE compressed_buffer = NULL;
  SIZE_T compressed_buffer_size = 0;
  SIZE_T decompressed_buffer_size = 0;
};

using PlatformCompApi = WinCompApi;

}  // namespace compressor
}  // namespace muehle

#endif  // _WIN32
#endif  // MUEHLE_COMPRESSOR_WIN_COMP_API_H_
