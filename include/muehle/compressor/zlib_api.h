#ifndef MUEHLE_COMPRESSOR_ZLIB_API_H_
#define MUEHLE_COMPRESSOR_ZLIB_API_H_

#include <zlib.h>

#include <cstdint>

#include "muehle/compressor/general_lib.h"

namespace muehle {
namespace compressor {

/*
 * Linux implementation of the Windows Compression API's
 * COMPRESS_ALGORITHM_MSZIP buffer mode.
 *
 * The existing database was generated on Windows using:
 *
 *   CreateCompressor(COMPRESS_ALGORITHM_MSZIP, ...)
 *
 * without COMPRESS_RAW.
 *
 * Therefore the database contains the MSZIP buffer-mode header
 * followed by MSZIP chunks, rather than ordinary zlib streams.
 */
class ZLibApi : public GeneralLib {
 public:
  ZLibApi();
  ~ZLibApi();

  bool Compress(void* compressed_data, void* source_data,
                unsigned int n_bytes_to_compress,
                unsigned int& n_bytes_compressed) override;

  bool Decompress(void* dest_data, void* compressed_data,
                  unsigned int n_bytes_compressed,
                  unsigned int& n_bytes_decompressed) override;

  long long EstimateMaxSizeOfCompressedData(
      long long amount_uncompressed_data) override;

 private:
  unsigned long compressed_buffer_size = 0;
  unsigned long decompressed_buffer_size = 0;
};

using PlatformCompApi = ZLibApi;

}  // namespace compressor
}  // namespace muehle

#endif  // MUEHLE_COMPRESSOR_ZLIB_API_H_