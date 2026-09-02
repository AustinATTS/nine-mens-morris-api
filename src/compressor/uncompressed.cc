#include "muehle/compressor/uncompressed.h"

#include <cstring>

namespace muehle {

bool compressor::Uncompressed::Compress(void* compressed_data,
                                        void* source_data,
                                        unsigned int n_bytes_to_compress,
                                        unsigned int& n_bytes_compressed) {
  memcpy(compressed_data, source_data, n_bytes_to_compress);
  n_bytes_compressed = n_bytes_to_compress;
  return true;
}

bool compressor::Uncompressed::Decompress(void* dest_data,
                                          void* compressed_data,
                                          unsigned int n_bytes_compressed,
                                          unsigned int& n_bytes_decompressed) {
  memcpy(dest_data, compressed_data, n_bytes_compressed);
  n_bytes_decompressed = n_bytes_compressed;
  return true;
}

long long compressor::Uncompressed::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  return amount_uncompressed_data;
}
}  // namespace muehle
