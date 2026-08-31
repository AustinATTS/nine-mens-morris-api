#ifndef MUEHLE_UTILS_CYCLIC_ARRAY_H_
#define MUEHLE_UTILS_CYCLIC_ARRAY_H_

#ifdef _WIN32
#include <windows.h>
#else  // _WIN32
#include "muehle/win_32_compat.h"
#endif  // _WIN32
#include <iostream>
#include <string>

#include "muehle/utils/logger.h"

namespace muehle {

/* CyclicArray is a clas for cyclic data access.
 * The class uses a file as temporary data buffer for the cyclic array.
 * The class is designed for high performance file access of single bytes, on a
 * very large file. The reading and writing operations are buffered in memory,
 * and are only written to the file when the buffer is full. The class is not
 * thread safe
 */
class CyclicArray {
 private:
  /* Constants */
  static const uint64_t MAX_BLOCK_SIZE = 1e9;      /* 1 GB */
  static const uint64_t MAX_NUM_BLOCKS = 1e8;      /* 1 Million */
  static const uint64_t MAX_FILE_SIZE = 1e15;      /* 1 PetaByte */
  static const unsigned int MAX_PATH_LENGTH = 260; /* Max length of a path */
  static const unsigned int MAX_NUM_RETRIES =
      10; /* Max number of retries for reading/writing to file */
  static const unsigned int SLEEP_TIME_IN_MS =
      1000; /* Sleep time in ms for waiting for file access */

  /* Variables */
  Logger& log;   /* Logger, used for output */
  HANDLE h_file; /* Handle of the file */
  unsigned char*
      reading_block; /* Array of size [block_size] containing the data of the
                        block, where reading is taking place */
  unsigned char* writing_block;
  unsigned char*
      cur_reading_pointer; /* Pointer to the byte which is currently read */
  unsigned char* cur_writing_pointer;
  uint64_t
      cur_reading_pos; /* Position in the file, where reading is taking place */
  uint64_t cur_writing_pos;
  uint64_t
      cur_reading_block; /* Index of the block where reading is taking place */
  uint64_t
      cur_writing_block; /* Index of the block where writing is taking place */
  const uint64_t block_size;     /* Size in bytes of a block */
  const uint64_t num_blocks;     /* Amount of blocks */
  bool read_write_in_same_round; /* True if cur_reading_block >
                                    cur_writing_block, false otherwise */

  /* Functions */
  bool WriteDataToFile(HANDLE h_file, uint64_t offset,
                       unsigned int size_in_bytes, void* p_data);
  bool ReadDataFromFile(HANDLE h_file, uint64_t offset,
                        unsigned int size_in_bytes, void* p_data);

 public:
  /* Constructor / Destructor */
  CyclicArray(unsigned int block_size_in_bytes, unsigned int number_of_blocks,
              std::wstring const& file_name, Logger& log);
  ~CyclicArray();

  /* Functions */
  void Reset();
  bool AddBytes(unsigned int num_bytes, const unsigned char* p_data);
  bool TakeBytes(unsigned int num_bytes, unsigned char* p_data);
  bool LoadFile(std::wstring const& file_name, uint64_t& num_bytes_loaded);
  bool SaveFile(std::wstring const& file_name);
  uint64_t BytesAvailable() const;
  uint64_t WriteableBytes() const;
  uint64_t GetNumBlocks() const { return num_blocks; }
};

}  // namespace muehle

#endif  // MUEHLE_UTILS_CYCLIC_ARRAY_H_
