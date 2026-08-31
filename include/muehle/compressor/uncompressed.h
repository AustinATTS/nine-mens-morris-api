#ifndef MUEHLE_COMPRESSOR_UNCOMPRESSED_H_
#define MUEHLE_COMPRESSOR_UNCOMPRESSED_H_

#include "muehle/compressor/general_lib.h"

namespace muehle {
namespace compressor {

/* Class for uncompressed data */
class Uncompressed : public GeneralLib {
 public:
  Uncompressed() { name = L"uncompressed", id = LibId::uncompressed; };
  bool Compress(void* compressed_data, void* source_data,
                unsigned int n_bytes_to_compress,
                unsigned int& n_bytes_compressed);
  bool Decompress(void* dest_data, void* compressed_data,
                  unsigned int n_bytes_compressed,
                  unsigned int& n_bytes_decompressed);
  long long EstimateMaxSizeOfCompressedData(long long amount_uncompressed_data);
};

}  // namespace compressor
}  // namespace muehle

#endif  // MUEHLE_COMPRESSOR_UNCOMPRESSED_H_
