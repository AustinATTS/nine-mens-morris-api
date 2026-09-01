#include "muehle/compressor/zlib_api.h"

namespace muehle {

// Implementation goes here.

compressor::ZLibApi::ZLibApi() {
}

compressor::ZLibApi::~ZLibApi() {
}

bool compressor::ZLibApi::Compress(void* compressed_data, void* source_data,
    unsigned int n_bytes_to_compress, unsigned int& n_bytes_compressed) {
  return GeneralLib::Compress(compressed_data, source_data, n_bytes_to_compress,
                              n_bytes_compressed);
}

bool compressor::ZLibApi::Decompress(void* dest_data, void* compressed_data,
    unsigned int n_bytes_compressed, unsigned int& n_bytes_decompressed) {
  return GeneralLib::Decompress(dest_data, compressed_data, n_bytes_compressed,
                                n_bytes_decompressed);
}

long long compressor::ZLibApi::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  return GeneralLib::EstimateMaxSizeOfCompressedData(amount_uncompressed_data);
}
}  // namespace muehle
