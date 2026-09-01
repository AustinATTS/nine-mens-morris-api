#include "muehle/compressor/uncompressed.h"

namespace muehle {

// Implementation goes here.

bool compressor::Uncompressed::Compress(void* compressed_data,
    void* source_data, unsigned int n_bytes_to_compress,
    unsigned int& n_bytes_compressed) {
  return GeneralLib::Compress(compressed_data, source_data, n_bytes_to_compress,
                              n_bytes_compressed);
}

bool compressor::Uncompressed::Decompress(void* dest_data,
    void* compressed_data, unsigned int n_bytes_compressed,
    unsigned int& n_bytes_decompressed) {
  return GeneralLib::Decompress(dest_data, compressed_data, n_bytes_compressed,
                                n_bytes_decompressed);
}

long long compressor::Uncompressed::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  return GeneralLib::EstimateMaxSizeOfCompressedData(amount_uncompressed_data);
}
}  // namespace muehle
