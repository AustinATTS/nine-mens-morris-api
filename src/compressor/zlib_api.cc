#include "muehle/compressor/zlib_api.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace muehle {
namespace compressor {

namespace {

/*
 * Windows Compression API MSZIP buffer-mode format.
 *
 * Header:
 *
 *   0x00  6 bytes  magic: 0A 51 E5 C0 18 00
 *   0x06  1 byte   CRC/check byte
 *   0x07  1 byte   algorithm: 02 = MSZIP
 *   0x08  8 bytes  total uncompressed size
 *   0x10  8 bytes  first chunk uncompressed size
 *
 * Chunks:
 *
 *   0x00  4 bytes  little-endian chunk size
 *   0x04  2 bytes  "CK"
 *   0x06  ...      raw DEFLATE stream
 *
 * MSZIP uses a 32 KiB dictionary between chunks.
 */

constexpr unsigned int MSZIP_HEADER_SIZE = 24;
constexpr unsigned int MSZIP_CHUNK_HEADER_SIZE = 6;
constexpr unsigned int MSZIP_MAX_CHUNK_SIZE = 32768;

constexpr unsigned char MSZIP_MAGIC[6] = {0x0A, 0x51, 0xE5, 0xC0, 0x18, 0x00};

constexpr unsigned char MSZIP_ALGORITHM = 0x02;

constexpr unsigned char MSZIP_SIGNATURE[2] = {'C', 'K'};

/* Read a little-endian 32-bit integer. */
std::uint32_t ReadUInt32LE(const unsigned char* data) {
  return static_cast<std::uint32_t>(data[0]) |
         (static_cast<std::uint32_t>(data[1]) << 8) |
         (static_cast<std::uint32_t>(data[2]) << 16) |
         (static_cast<std::uint32_t>(data[3]) << 24);
}

/* Read a little-endian 64-bit integer. */
std::uint64_t ReadUInt64LE(const unsigned char* data) {
  std::uint64_t value = 0;

  for (unsigned int i = 0; i < 8; ++i) {
    value |= static_cast<std::uint64_t>(data[i]) << (i * 8);
  }

  return value;
}

/* Write a little-endian 32-bit integer. */
void WriteUInt32LE(unsigned char* data, std::uint32_t value) {
  data[0] = static_cast<unsigned char>(value & 0xff);
  data[1] = static_cast<unsigned char>((value >> 8) & 0xff);
  data[2] = static_cast<unsigned char>((value >> 16) & 0xff);
  data[3] = static_cast<unsigned char>((value >> 24) & 0xff);
}

/* Write a little-endian 64-bit integer. */
void WriteUInt64LE(unsigned char* data, std::uint64_t value) {
  for (unsigned int i = 0; i < 8; ++i) {
    data[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xff);
  }
}

/* Compress one MSZIP chunk.
 *
 * MSZIP chunks contain:
 *
 *   "CK"
 *   raw DEFLATE stream
 *
 * The DEFLATE stream is produced with a 32 KiB window.
 *
 * We use Z_DEFAULT_COMPRESSION and the highest memory level because
 * compatibility with Microsoft's MSZIP implementation is more
 * important here than squeezing out the final few bytes. */
bool CompressChunk(const unsigned char* source, unsigned int source_size,
                   std::vector<unsigned char>& output,
                   const unsigned char* dictionary,
                   unsigned int dictionary_size) {
  if (source_size > MSZIP_MAX_CHUNK_SIZE) {
    return false;
  }

  /* compressBound() gives us a safe upper bound for a zlib stream. */
  uLongf compressed_size = compressBound(source_size);

  std::vector<unsigned char> compressed(compressed_size);

  z_stream stream{};
  stream.next_in = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(source));
  stream.avail_in = source_size;

  stream.next_out = reinterpret_cast<Bytef*>(compressed.data());
  stream.avail_out = static_cast<uInt>(compressed.size());

  /* Raw DEFLATE.
   *
   * MSZIP does not use the normal zlib header/trailer. */
  if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 9,
                   Z_DEFAULT_STRATEGY) != Z_OK) {
    return false;
  }

  /* MSZIP uses the previous chunk as a dictionary. */
  if (dictionary != nullptr && dictionary_size != 0) {
    if (deflateSetDictionary(&stream,
                             reinterpret_cast<const Bytef*>(dictionary),
                             dictionary_size) != Z_OK) {
      deflateEnd(&stream);
      return false;
    }
  }

  int result = deflate(&stream, /* flush: */ Z_FINISH);

  if (result != Z_STREAM_END) {
    deflateEnd(&stream);
    return false;
  }

  const unsigned int actual_compressed_size =
      static_cast<unsigned int>(stream.total_out);

  deflateEnd(&stream);

  /* Chunk size includes:
   *
   *   "CK" + compressed DEFLATE stream */
  const std::uint32_t chunk_size =
      static_cast<std::uint32_t>(2 + actual_compressed_size);

  const std::size_t old_size = output.size();

  output.resize(old_size + sizeof(std::uint32_t) + chunk_size);

  unsigned char* destination = output.data() + old_size;

  WriteUInt32LE(destination, chunk_size);

  destination += sizeof(std::uint32_t);

  destination[0] = MSZIP_SIGNATURE[0];
  destination[1] = MSZIP_SIGNATURE[1];

  std::memcpy(/* dest: */ destination + 2, /* src: */ compressed.data(),
              actual_compressed_size);

  return true;
}

/* Decompress one MSZIP chunk. */
bool DecompressChunk(const unsigned char* compressed,
                     unsigned int compressed_size, unsigned char* destination,
                     unsigned int destination_size,
                     const unsigned char* dictionary,
                     unsigned int dictionary_size,
                     unsigned int& bytes_decompressed) {
  bytes_decompressed = 0;

  /* Every chunk must at least contain: "CK" */
  if (compressed_size < 2) {
    return false;
  }

  if (compressed[0] != MSZIP_SIGNATURE[0] ||
      compressed[1] != MSZIP_SIGNATURE[1]) {
    return false;
  }

  z_stream stream{};

  stream.next_in =
      const_cast<Bytef*>(reinterpret_cast<const Bytef*>(compressed + 2));

  stream.avail_in = compressed_size - 2;

  stream.next_out = reinterpret_cast<Bytef*>(destination);

  stream.avail_out = destination_size;

  /* Raw DEFLATE. */
  if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
    return false;
  }

  /* Supply the previous MSZIP chunk as the preset dictionary. */
  if (dictionary != nullptr && dictionary_size != 0) {
    if (inflateSetDictionary(&stream,
                             reinterpret_cast<const Bytef*>(dictionary),
                             dictionary_size) != Z_OK) {
      inflateEnd(&stream);
      return false;
    }
  }

  const int result = inflate(&stream, /* flush: */ Z_FINISH);

  if (result != Z_STREAM_END) {
    inflateEnd(&stream);
    return false;
  }

  bytes_decompressed = static_cast<unsigned int>(stream.total_out);

  inflateEnd(&stream);

  return true;
}

} /* namespace */

ZLibApi::ZLibApi() {
  /* IMPORTANT:
   *
   * The database footer identifies the compressor as
   * LibId::win_comp_api.
   *
   * We must therefore keep this ID even though the implementation
   * is running on Linux. */
  name = L"MSZIP";
  id = LibId::win_comp_api;
}

ZLibApi::~ZLibApi() {}

/* Compress data using the Windows-compatible MSZIP buffer format. */
bool ZLibApi::Compress(void* compressed_data, void* source_data,
                       unsigned int n_bytes_to_compress,
                       unsigned int& n_bytes_compressed) {
  n_bytes_compressed = 0;

  if (compressed_data == nullptr || source_data == nullptr) {
    return false;
  }

  if (n_bytes_to_compress == 0) {
    return false;
  }

  /* The Windows Compression API buffer-mode header is 24 bytes. */
  if (compressed_buffer_size < MSZIP_HEADER_SIZE) {
    return false;
  }

  const auto* source = reinterpret_cast<const unsigned char*>(source_data);

  std::vector<unsigned char> output;

  output.reserve(compressed_buffer_size);

  /* Reserve header. */
  output.resize(MSZIP_HEADER_SIZE, /* x: */ 0);

  /* Header magic. */
  std::memcpy(/* dest: */ output.data(), /* src: */ MSZIP_MAGIC,
              /* n: */ sizeof(MSZIP_MAGIC));

  /* CRC/check byte.
   *
   * The Windows MSZIP buffer header contains a one-byte field here.
   * Leave it zero; it is not used by the decompression path. */
  output[6] = 0;

  /* Algorithm:
   *
   *   2 = MSZIP */
  output[7] = MSZIP_ALGORITHM;

  /* Total uncompressed size. */
  WriteUInt64LE(/* data: */ output.data() + 8,
                static_cast<std::uint64_t>(n_bytes_to_compress));

  /* First chunk size.
   *
   * This is the uncompressed size of the first chunk. */
  const unsigned int first_chunk_size =
      std::min(/* a: */ n_bytes_to_compress, /* b: */ MSZIP_MAX_CHUNK_SIZE);

  WriteUInt64LE(/* data: */ output.data() + 16,
                static_cast<std::uint64_t>(first_chunk_size));

  /* MSZIP carries the last 32 KiB of the previous chunk as the
   * dictionary for the next chunk. */
  std::vector<unsigned char> dictionary;

  unsigned int source_offset = 0;

  while (source_offset < n_bytes_to_compress) {
    const unsigned int chunk_size =
        std::min(/* a: */ MSZIP_MAX_CHUNK_SIZE,
                 /* b: */ n_bytes_to_compress - source_offset);

    if (!CompressChunk(
            source + source_offset, /* source_size: */ chunk_size, output,
            /* dictionary: */ dictionary.empty() ? nullptr : dictionary.data(),
            /* dictionary_size: */
            static_cast<unsigned int>(dictionary.size()))) {
      return false;
    }

    /* The dictionary for the next block is the last 32 KiB of the
     * current uncompressed block. */
    dictionary.assign(/* first: */ source + source_offset,
                      /* last: */ source + source_offset + chunk_size);

    source_offset += chunk_size;
  }

  /* Make sure the caller's buffer can hold the entire result. */
  if (output.size() > compressed_buffer_size) {
    return false;
  }

  std::memcpy(compressed_data, /* src: */ output.data(),
              /* n: */ output.size());

  n_bytes_compressed = static_cast<unsigned int>(output.size());

  return true;
}

/* Decompress Windows Compression API MSZIP buffer-mode data. */
bool ZLibApi::Decompress(void* dest_data, void* compressed_data,
                         unsigned int n_bytes_compressed,
                         unsigned int& n_bytes_decompressed) {
  n_bytes_decompressed = 0;

  if (dest_data == nullptr || compressed_data == nullptr) {
    return false;
  }

  if (n_bytes_compressed < MSZIP_HEADER_SIZE) {
    return false;
  }

  const auto* source = reinterpret_cast<const unsigned char*>(compressed_data);

  /* Verify MSZIP header magic. */
  if (std::memcmp(/* s1: */ source, /* s2: */ MSZIP_MAGIC,
                  /* n: */ sizeof(MSZIP_MAGIC)) != 0) {
    return false;
  }

  /* Verify algorithm. */
  if (source[7] != MSZIP_ALGORITHM) {
    return false;
  }

  const std::uint64_t expected_uncompressed_size =
      ReadUInt64LE(/* data: */ source + 8);

  /* Our destination buffer is sized to the requested section size. */
  if (expected_uncompressed_size >
      static_cast<std::uint64_t>(decompressed_buffer_size)) {
    return false;
  }

  if (expected_uncompressed_size > std::numeric_limits<unsigned int>::max()) {
    return false;
  }

  auto* destination = reinterpret_cast<unsigned char*>(dest_data);

  std::vector<unsigned char> dictionary;

  unsigned int source_offset = MSZIP_HEADER_SIZE;
  unsigned int destination_offset = 0;

  while (source_offset < n_bytes_compressed) {
    /* Each chunk starts with a four-byte little-endian chunk size. */
    if (n_bytes_compressed - source_offset < 4) {
      return false;
    }

    const std::uint32_t chunk_size =
        ReadUInt32LE(/* data: */ source + source_offset);

    source_offset += 4;

    /*  The chunk size includes:
     *   "CK"
     *   compressed data */
    if (chunk_size < 2) {
      return false;
    }

    if (chunk_size > n_bytes_compressed - source_offset) {
      return false;
    }

    if (chunk_size > MSZIP_MAX_CHUNK_SIZE + 1000) {
      /* A compressed 32 KiB block should never require an
       * unreasonable amount of compressed storage.
       *
       * The +1000 is deliberately generous. */
      return false;
    }

    /* Do not allow decompression to exceed the destination buffer. */
    const unsigned int remaining_output = static_cast<unsigned int>(
        expected_uncompressed_size - destination_offset);

    if (remaining_output == 0) {
      return false;
    }

    const unsigned int maximum_chunk_output =
        std::min(/* a: */ remaining_output, /* b: */ MSZIP_MAX_CHUNK_SIZE);

    unsigned int chunk_output_size = 0;

    if (!DecompressChunk(
            /* compressed: */ source + source_offset,
            /* compressed_size: */ chunk_size, destination + destination_offset,
            /* destination_size: */ maximum_chunk_output,
            /* dictionary: */ dictionary.empty() ? nullptr : dictionary.data(),
            /* dictionary_size: */ static_cast<unsigned int>(dictionary.size()),
            /* bytes_decompressed: */ chunk_output_size)) {
      return false;
    }

    if (chunk_output_size == 0) {
      return false;
    }

    /* Update output position. */
    destination_offset += chunk_output_size;

    /* Preserve the current chunk as the dictionary for the next
     * chunk. */
    dictionary.assign(
        /* first: */ destination + destination_offset - chunk_output_size,
        /* last: */ destination + destination_offset);

    /* The MSZIP dictionary is limited to 32 KiB. */
    if (dictionary.size() > MSZIP_MAX_CHUNK_SIZE) {
      dictionary.erase(/* first: */ dictionary.begin(),
                       /* last: */ dictionary.end() - MSZIP_MAX_CHUNK_SIZE);
    }

    source_offset += chunk_size;
  }

  /* The compressed stream must produce exactly the amount of data
   * declared in the MSZIP header. */
  if (destination_offset !=
      static_cast<unsigned int>(expected_uncompressed_size)) {
    return false;
  }

  n_bytes_decompressed = destination_offset;

  return true;
}

/* Return a safe upper bound for MSZIP output. */
long long ZLibApi::EstimateMaxSizeOfCompressedData(
    long long amount_uncompressed_data) {
  if (amount_uncompressed_data <= 0) {
    compressed_buffer_size = 0;
    decompressed_buffer_size = 0;
    return 0;
  }

  /* compressBound() is for a normal zlib stream. MSZIP adds:
   *   24-byte header
   *   4-byte chunk length for every chunk
   *   2-byte CK signature for every chunk
   *
   * Give ourselves a generous additional margin. */

  const long long number_of_chunks =
      (amount_uncompressed_data + MSZIP_MAX_CHUNK_SIZE - 1) /
      MSZIP_MAX_CHUNK_SIZE;

  const long long zlib_bound =
      compressBound(static_cast<uLong>(amount_uncompressed_data));

  const long long extra_overhead = MSZIP_HEADER_SIZE +
                                   number_of_chunks * MSZIP_CHUNK_HEADER_SIZE +
                                   number_of_chunks * 1024;

  compressed_buffer_size =
      static_cast<unsigned long>(zlib_bound + extra_overhead);

  decompressed_buffer_size =
      static_cast<unsigned long>(amount_uncompressed_data);

  return compressed_buffer_size;
}

} /* namespace compressor */
} /* namespace muehle */