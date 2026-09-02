#include "muehle/compressor/zlib_api.h"

namespace muehle {

/* Class constructor */
compressor::ZLibApi::ZLibApi() {
  name = L"ZLibApi";
  id = LibId::win_comp_api; /* Reuse the same ID as WinCompApi, since both are the "default compressor" for the current platform */
}

/* Class Destructor */
compressor::ZLibApi::~ZLibApi() {
}

bool compressor::ZLibApi::Compress(void* compressed_data, void* source_data,
    unsigned int n_bytes_to_compress, unsigned int& n_bytes_compressed) {
  uLongf dest_len = compressed_buffer_size;

  int result = compress2((Bytef *)compressed_data, &dest_len, (const Bytef *)source_data, (uLong)n_bytes_to_compress, Z_DEFAULT_COMPRESSION);

  if (result != Z_OK) {
    return false;
  }

  n_bytes_compressed = (unsigned int)result;

  return true;
}

bool compressor::ZLibApi::Decompress(void* dest_data, void* compressed_data,
    unsigned int n_bytes_compressed, unsigned int& n_bytes_decompressed) {
  uLongf dest_len = decompressed_buffer_size;

  int result = uncompress((Bytef *)dest_data, &dest_len, (const Bytef *)compressed_data, (uLong)n_bytes_compressed);

  if (result != Z_OK) {
    return false;
  }

  n_bytes_decompressed = (unsigned int)dest_len;

  return true;
}

long long compressor::ZLibApi::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  compressed_buffer_size = compressBound((uLong)amount_uncompressed_data);
  decompressed_buffer_size = (unsigned long)amount_uncompressed_data;
  return compressed_buffer_size;
}
}  // namespace muehle
