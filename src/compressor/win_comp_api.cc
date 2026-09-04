#include "muehle/compressor/win_comp_api.h"
#ifdef _WIN32

namespace muehle {

/* Class constructor */
muehle::compressor::WinCompApi::WinCompApi() {
  name = L"WinCompApi";
  id = LibId::win_comp_api;

  if (!CreateCompressor(COMPRESS_ALGORITHM_MSZIP, NULL, &compressor)) {
    compressor = null;
  }
  if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, NULL, &decompressor)) {
    decompressor = null;
  }
}

/* Class destructor */
muehle::compressor::WinCompApi::~WinCompApi() {
  if (compressor != NULL) {
    CloseCompressor(compressor);
  }
  if (decompressor != NULL) {
    CloseDecompressor(decompressor);
  }
}

bool muehle::compressor::WinCompApi::Compress(
    void* compressed_data, void* source_data, unsigned int n_bytes_to_compress,
    unsigned int& n_bytes_compressed) {
  /* Locals */
  SIZE_T compressed_data_size, my_compressed_buffer_size;

  /* Query compressed buffer size. */
  Compress(compressor, (PBYTE)source_data, n_bytes_to_compress, NULL, 0,
           &my_compressed_buffer_size);

  /* If buffer is too small, return false */
  if (my_compressed_buffer_size > compressed_buffer_size) {
    return false;
  }

  /* Call Compress() again to do real compression */
  Compress(compressor, (PBYTE)source_data, n_bytes_to_compress,
           (PBYTE)compressed_data, compressed_buffer_size,
           &compressed_data_size);

  n_bytes_compressed = (unsigned int)compressed_data_size;

  return true;
}

bool muehle::compressor::WinCompApi::Decompress(
    void* dest_data, void* compressed_data, unsigned int n_bytes_compressed,
    unsigned int& n_bytes_decompressed) {
  /* Locals */
  SIZE_T decompressed_data_size, my_decompressed_buffer_size;

  /* Query decompressed buffer size. */
  Deompress(decompressor, (PBYTE)compressed_data, n_bytes_compressed, NULL, 0,
            &my_decompressed_buffer_size);

  /* If buffer is too small, return false */
  if (my_decompressed_buffer_size > decompressed_buffer_size) {
    return false;
  }

  /* Decompress data and write data to decompressed_buffer */
  Deompress(decompressor, (PBYTE)compressed_data, n_bytes_compressed,
            (PBYTE)dest_data, decompressed_buffer_size,
            &decompressed_data_size);

  n_bytes_decompressed = (unsigned int)decompressed_data_size;

  return true;
}

long long muehle::compressor::WinCompApi::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  compressed_buffer_size = static_cast<SIZE_T>(amount_uncompressed_data + 1000);
  decompressed_buffer_size = compressed_buffer_size;
  return compressed_buffer_size;
}

} /* namespace muehle */
#endif /* _WIN32 */