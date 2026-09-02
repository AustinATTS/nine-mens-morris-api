#include "muehle/utils/cyclic_array.h"

#include "muehle/utils/logger.h"

namespace muehle {

/* Creates a cyclic array. The passed file is used as temporary data buffer for
 * the cyclic array. */
CyclicArray::CyclicArray(unsigned int block_size_in_bytes,
                         unsigned int number_of_blocks,
                         std::wstring const& file_name, Logger& log)
    : h_file(NULL),
      block_size(block_size_in_bytes),
      num_blocks(number_of_blocks),
      reading_block(nullptr),
      writing_block(nullptr),
      log(log) {
  /* Checks */
  if (block_size > MAX_BLOCK_SIZE) {
    return;
  }
  if (num_blocks > MAX_NUM_BLOCKS) {
    return;
  }
  if (block_size * num_blocks > MAX_FILE_SIZE) {
    return;
  }
  if (file_name.length() > MAX_PATH_LENGTH) {
    return;
  }
  if (block_size == 0) {
    return;
  }
  if (num_blocks == 0) {
    return;
  }
  if (file_name.length() == 0) {
    return;
  }

  /* Init blocks */
  reading_block = new unsigned char[block_size];
  writing_block = new unsigned char[block_size];
  Reset();
  log.Log(Logger::LogLevel::trace,
          L"CyclicArray created: " + file_name + L" with block_size: " +
              std::to_wstring(block_size) + L" bytes and " +
              std::to_wstring(num_blocks) + L" blocks.");

  /* Open Database file (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH |
   * FILE_FLAG_RANDOM_ACCESS) */
  h_file = CreateFile(file_name.c_str(), GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);

  /* Opened file succesfully */
  if (h_file == INVALID_HANDLE_VALUE) {
    h_file = NULL;
    return;
  }
}

/* CyclicArray class destructor */
CyclicArray::~CyclicArray() {
  /* Delete arrays */
  if (reading_block != nullptr) {
    delete[] reading_block;
  }
  if (writing_block != nullptr) {
    delete[] writing_block;
  }

  /* Close file */
  if (h_file != nullptr) {
    CloseHandle(h_file);
  }
}

/* Resets the cyclic array. The file is not changed */
void CyclicArray::Reset() {
  cur_reading_pos = 0;
  cur_writing_pos = 0;
  cur_reading_pointer = writing_block;
  cur_writing_pointer = writing_block;
  read_write_in_same_round = true;
  cur_reading_block = 0;
  cur_writing_block = 0;
}

/* Writes 'size_in_bytes; -bytes to the position 'offset' to the file */
bool CyclicArray::WriteDataToFile(HANDLE h_file, uint64_t offset,
                                  unsigned int size_in_bytes, void* p_data) {
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL");
  }
  if (size_in_bytes == 0) {
    return true;
  }
  if (p_data == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: Pointer to data is NULL");
  }
  if (size_in_bytes > MAX_FILE_SIZE) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Size of data to write is too large! size_in_bytes: " +
            std::to_wstring(size_in_bytes) + L" bytes!");
  }
  if (offset < 0) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Offset is negative! offset:" +
                       std::to_wstring(offset) + L" bytes!");
  }
  if (offset + size_in_bytes > MAX_FILE_SIZE) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Offset + size_in_bytes is greater than max file "
                   L"size! offset: " +
                       std::to_wstring(offset) + L" bytes! size_in_bytes: " +
                       std::to_wstring(size_in_bytes) + L" bytes!");
  }

  DWORD dw_bytes_written;
  LARGE_INTEGER li_distance_to_move;
  unsigned int resting_bytes = size_in_bytes;
  unsigned int num_retries = 0;

  li_distance_to_move.QuadPart = offset;

  while (!SetFilePointerEx(h_file, li_distance_to_move, NULL, FILE_BEGIN)) {
    log << L"SetFilePointerEx failed! Retry counter:" << num_retries << L"\n";
    num_retries++;
    if (num_retries > MAX_NUM_RETRIES) {
      return false;
    }
    Sleep(SLEEP_TIME_IN_MS);
  }

  while (resting_bytes > 0) {
    if (WriteFile(h_file, p_data, size_in_bytes, &dw_bytes_written, NULL) ==
        TRUE) {
      resting_bytes -= dw_bytes_written;
      p_data = (void*)(((unsigned char*)p_data) + dw_bytes_written);
      if (resting_bytes > 0) {
        log << L"Still " << resting_bytes << L" to write!" << L"\n";
      }
    } else {
      log << L"WriteFile Failed! Retry counter: " << num_retries << L"\n";
      num_retries++;
      if (num_retries > MAX_NUM_RETRIES) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: WriteFile failed! num_retries:" +
                           std::to_wstring(num_retries));
      }
      Sleep(SLEEP_TIME_IN_MS);
    }
  }
  return true;
}

/* Reads 'size_in_bytes' -bytes from the possition 'offset' of the file. */
bool CyclicArray::ReadDataFromFile(HANDLE h_file, uint64_t offset,
                                   unsigned int size_in_bytes, void* p_data) {
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL");
  }
  if (size_in_bytes == 0) {
    return true;
  }
  if (p_data == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: Pointer to data is NULL");
  }
  if (size_in_bytes > MAX_FILE_SIZE) {
    return log.Log(
        Logger::LogLevel::error,
        L"ERROR: Size of data to write is too large! size_in_bytes: " +
            std::to_wstring(size_in_bytes) + L" bytes!");
  }
  if (offset < 0) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Offset is negative! offset:" +
                       std::to_wstring(offset) + L" bytes!");
  }
  if (offset + size_in_bytes > MAX_FILE_SIZE) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Offset + size_in_bytes is greater than max file "
                   L"size! offset: " +
                       std::to_wstring(offset) + L" bytes! size_in_bytes: " +
                       std::to_wstring(size_in_bytes) + L" bytes!");
  }

  DWORD dw_bytes_written;
  LARGE_INTEGER li_distance_to_move;
  unsigned int resting_bytes = size_in_bytes;
  unsigned int num_retries = 0;

  li_distance_to_move.QuadPart = offset;

  while (!SetFilePointerEx(h_file, li_distance_to_move, NULL, FILE_BEGIN)) {
    log << L"SetFilePointerEx failed! Retry counter:" << num_retries << L"\n";
    num_retries++;
    if (num_retries > MAX_NUM_RETRIES) {
      return false;
    }
    Sleep(SLEEP_TIME_IN_MS);
  }

  while (resting_bytes > 0) {
    if (ReadFile(h_file, p_data, size_in_bytes, &dw_bytes_written, NULL) ==
        TRUE) {
      resting_bytes -= dw_bytes_written;
      p_data = (void*)(((unsigned char*)p_data) + dw_bytes_written);
      if (resting_bytes > 0) {
        log << L"Still " << resting_bytes << L" to read!" << L"\n";
      }
    } else {
      log << L"ReadFile Failed! Retry counter: " << num_retries << L"\n";
      num_retries++;
      if (num_retries > MAX_NUM_RETRIES) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: ReadFile failed! num_retries:" +
                           std::to_wstring(num_retries));
      }
      Sleep(SLEEP_TIME_IN_MS);
    }
  }
  return true;
}

/* Returns true if there are any bytes available for reading. */
uint64_t CyclicArray::BytesAvailable() const {
  /* Cyclic array is empty */
  if (cur_writing_pos == cur_reading_pos && read_write_in_same_round) {
    return 0;
    /* Cyclic array is full */
  } else if (cur_writing_pos == cur_reading_pos) {
    return block_size * num_blocks;
    /* Reading and writing pointer are in the same round */
  } else if (read_write_in_same_round) {
    return cur_writing_pos - cur_reading_pos;
    /* Reading and writing pointer are in different rounds*/
  } else {
    return block_size * num_blocks - (cur_reading_pos - cur_writing_pos);
  }
}

/* Returns the amount of bytes that can be written to the cyclic array at the
 * moment. */
uint64_t CyclicArray::WriteableBytes() const {
  /* Cyclic array is empty */
  if (cur_writing_pos == cur_reading_pos && read_write_in_same_round) {
    return block_size * num_blocks;
    /* Cyclic array is full */
  } else if (cur_writing_pos == cur_reading_pos) {
    return 0;
    /* Reading and writing pointer are in the same round */
  } else if (read_write_in_same_round) {
    return block_size * num_blocks - (cur_reading_pos - cur_writing_pos);
    /* Reading and writing pointer are in different rounds */
  } else {
    return cur_reading_pos - cur_writing_pos;
  }
}

/* Add the passed data to the cyclic array. If the writing pointer reaches the
 * end of a block, the data of the whole block is written to the file and the
 * next block is considered for writing. */
bool CyclicArray::AddBytes(unsigned int num_bytes,
                           const unsigned char* p_data) {
  /* Checks */
  if (num_bytes == 0) {
    return true;
  }
  if (p_data == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: Pointer to data is NULL!");
  }
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL!");
  }
  if (num_bytes > WriteableBytes()) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Not enough space in cyclic array! num_bytes:" +
                       std::to_wstring(num_bytes) + L" bytes! writeable_bytes" +
                       std::to_wstring(WriteableBytes()) + L" bytes!");
  }

  /* Locals */
  unsigned int bytes_written = 0;

  /* Write each byte */
  while (bytes_written < num_bytes) {
    /* Store byte in current reading data */
    *cur_writing_pointer = *p_data;
    cur_writing_pointer++;
    cur_writing_pos++;
    bytes_written++;
    p_data++;

    /* When block is full then save current one to file and begin new one */
    if (cur_writing_pointer == writing_block + block_size) {
      /* Copy data into reading block, if reading block is the same as writing
       * block */
      if (cur_reading_block == cur_writing_block) {
        memcpy(reading_block, writing_block, block_size);
        /* Until now the reading pointer was using the writing block. Now it has
         * to use the reading block */
        cur_reading_pointer =
            reading_block + (cur_reading_pointer - writing_block);
      }

      /* Store block in file */
      if (!WriteDataToFile(h_file, block_size * cur_writing_block, block_size,
                           writing_block)) {
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: WriteDataToFile failed! cur_writing_block:" +
                           std::to_wstring(cur_writing_block) +
                           L" bytes_written:" + std::to_wstring(bytes_written) +
                           L" bytes!");
      }

      /* Set pointer to beginning of writing block */
      cur_writing_pointer = writing_block;
      cur_writing_block = (cur_writing_block + 1) % block_size;
      if (cur_writing_block == 0) {
        read_write_in_same_round = false;
        cur_writing_pos = 0;
      }
    }
  }

  /* Everything ok */
  return true;
}

/* Load data from the cyclic array. If the reading pointer reaches the end of a
 * block, the data of the next whole block is read from the file. */
bool CyclicArray::TakeBytes(unsigned int num_bytes, unsigned char* p_data) {
  /* Checks */
  if (num_bytes == 0) {
    return true;
  }
  if (p_data == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: Pointer to data is NULL!");
  }
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL!");
  }
  if (num_bytes > WriteableBytes()) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: Not enough space in cyclic array! num_bytes:" +
                       std::to_wstring(num_bytes) + L" bytes! writeable_bytes" +
                       std::to_wstring(WriteableBytes()) + L" bytes!");
  }

  /* Locals */
  unsigned int bytes_read = 0;

  /* Read each byte */
  while (bytes_read < num_bytes) {
    /* Read current byte */
    *p_data = *cur_reading_pointer;
    cur_reading_pointer++;
    cur_reading_pos++;
    bytes_read++;
    p_data++;

    /* Load next block */
    if (cur_reading_pointer == reading_block + block_size) {
      /* Go to next block */
      if (cur_reading_block == (cur_reading_block + 1) % num_blocks) {
        if (cur_reading_block == 0) {
          read_write_in_same_round = true;
          cur_reading_pos = 0;
        }

        /* Writing block reached? */
        if (cur_reading_block == cur_writing_block) {
          cur_reading_pointer = writing_block;
        }
      } else {
        /* Set pointer to beginning of the reading block */
        cur_reading_pointer = reading_block;

            /* Store block in file */
            if (!ReadDataFromFile(h_file, block_size * cur_reading_block,
                                  block_size, reading_block)) {
          return log.Log(Logger::LogLevel::error,
                         L"ERROR: ReadDataFromFile failed! cur_reading_block:" +
                             std::to_wstring(cur_reading_block) +
                             L" bytes_written:" + std::to_wstring(bytes_read) +
                             L" bytes!");
        }
      }
    }
  }

  /* Everything ok */
  return true;
}

/* Load the passed file into the cyclic arrary.
 * The passed filename must be different than the passed filename to the
 * constructor CyclicArray().
 */
bool CyclicArray::LoadFile(std::wstring const& file_name,
                           uint64_t& num_bytes_loaded) {
  /* Locals */
  HANDLE h_load_file;
  unsigned char* data_in_file;
  LARGE_INTEGER large_int;
  int64_t max_file_size = block_size * num_blocks;
  int64_t cur_offset = 0;
  uint64_t num_blocks_in_file;
  uint64_t cur_block;
  uint64_t num_bytes_in_last_block;
  num_bytes_loaded = 0;

  /* Cyclic array file must be open */
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL!");
  }

  /* Open Database File (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH |
   * FILE_FLAG_RANDOM_ACCESS) */
  h_load_file = CreateFile(file_name.c_str(), GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  /* Opened file succesfully */
  if (h_load_file == INVALID_HANDLE_VALUE) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: File handle is NULL! file_name:" + file_name);
  }

  /* Does data of the file fit into cyclic array? */
  GetFileSizeEx(h_load_file, &large_int);

  /* File too large? */
  if (max_file_size < large_int.QuadPart) {
    CloseHandle(h_load_file);
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: File too large! file_name:" + file_name +
                       L" max_file_size:" + std::to_wstring(max_file_size) +
                       L" bytes! file_size:" +
                       std::to_wstring(large_int.QuadPart) + L" bytes!");
  }

  /* Reset */
  Reset();

  /* Calculate number of blocks */
  num_blocks_in_file = (large_int.QuadPart / block_size) + 1;
  num_bytes_in_last_block = (large_int.QuadPart % block_size);
  data_in_file = new unsigned char[block_size];

  /* Load blocks */
  for (cur_block = 0; cur_block < num_blocks_in_file - 1;
       cur_block++, cur_offset += block_size) {
    /* Load data from file */
    if (!ReadDataFromFile(h_load_file, cur_offset, block_size, data_in_file)) {
      delete[] data_in_file;
      CloseHandle(h_load_file);
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: ReadDataFromFile failed! cur_block:" +
                         std::to_wstring(cur_block) + L" bytes_loaded: " +
                         std::to_wstring(num_bytes_loaded) + L" bytes!");
    }

    /* Put block in cyclic array */
    if (!AddBytes(block_size, data_in_file)) {
      delete[] data_in_file;
      CloseHandle(h_load_file);
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: AddBytes failed! cur_block:" +
                         std::to_wstring(cur_block) + L" bytes_loaded: " +
                         std::to_wstring(num_bytes_loaded) + L" bytes!");
    }
  }

  /* Last block */
  ReadDataFromFile(h_load_file, cur_offset, num_bytes_in_last_block,
                   data_in_file);
  if (!AddBytes(num_bytes_in_last_block, data_in_file)) {
    delete[] data_in_file;
    CloseHandle(h_load_file);
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: AddBytes failed for last block! bytes_loaded: " +
                       std::to_wstring(num_bytes_loaded) + L" bytes!");
  }
  cur_offset += num_bytes_in_last_block;
  num_bytes_loaded = cur_offset;

  /* Everything ok */
  delete[] data_in_file;
  CloseHandle(h_load_file);
  return true;
}

/* Writes the whole current content of the cyclic array to the passed file.
 * The passes filename must be different than the passed filename to the
 * constructor CyclicArray(). The cyclic array is not changed by this operation.
 * The file is overwritten. Then the file is created if it does not exist.
 * The first byte takeable from the cyclic array is the first byte in the file.
 * This allows to load the file back into a fresh cyclic array.
 */
bool CyclicArray::SaveFile(std::wstring const& file_name) {
  /* Locals */
  unsigned char* data_in_file;
  HANDLE h_save_file;
  uint64_t cur_block;
  uint64_t bytes_to_write;
  uint64_t total_bytes_to_write;
  uint64_t total_bytes_written;
  void* pointer;

  /* Cyclic array must be open */
  if (h_file == NULL) {
    return log.Log(Logger::LogLevel::error, L"ERROR: File handle is NULL!");
  }

  /* Open Database File (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH |
   * FILE_FLAG_RANDOM_ACCESS) */
  h_save_file = CreateFile(file_name.c_str(), GENERIC_READ, FILE_SHARE_READ,
                           NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  /* Opened file succesfully */
  if (h_save_file == INVALID_HANDLE_VALUE) {
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: File handle is NULL! file_name:" + file_name);
  }

  /* Alloc mem */
  cur_block = cur_reading_block;
  data_in_file = new unsigned char[block_size];
  total_bytes_to_write = BytesAvailable();
  total_bytes_written = 0;

  /* Save block by block starting with the current reading block */
  const uint64_t MAX_ITERATIONS = num_blocks * 2; /* Reasonable upper bound */
  uint64_t iteration_count = 0;
  while (total_bytes_written < total_bytes_to_write &&
         iteration_count < MAX_ITERATIONS) {
    /* If reading and writing block are the same, and the reading pointer is in
     * the writing block */
    if (cur_block == cur_writing_block && cur_block == cur_reading_block &&
        read_write_in_same_round) {
      pointer = cur_reading_pointer;
      bytes_to_write = cur_writing_pointer - cur_reading_pointer;
      /* If reading and writing block are the same, and the reading pointer is
       * in the reading block */
    } else if (cur_block == cur_writing_block &&
               cur_block == cur_reading_block) {
      pointer = cur_reading_pointer;
      bytes_to_write = block_size - (cur_reading_pointer - reading_block);
      /* Store data from writing block */
    } else if (cur_block == cur_writing_block) {
      pointer = writing_block;
      bytes_to_write = cur_writing_pointer - writing_block;
      /* Store data from reading block */
    } else if (cur_block == cur_reading_block) {
      pointer = cur_reading_pointer;
      bytes_to_write = block_size - (cur_reading_pointer - reading_block);
      /* Store data from file */
    } else {
      if (!ReadDataFromFile(h_file, cur_block * block_size, block_size,
                            data_in_file)) {
        delete[] data_in_file;
        CloseHandle(h_file);
        return log.Log(Logger::LogLevel::error,
                       L"ERROR: ReadDataFromFile failed! cur_block:" +
                           std::to_wstring(cur_block) + L" bytes_written: " +
                           std::to_wstring(total_bytes_written) + L" bytes!");
      }
      pointer = data_in_file;
      bytes_to_write = block_size;
    }

    /* Save data to file*/
    if (!WriteDataToFile(h_save_file, total_bytes_written, bytes_to_write,
                         pointer)) {
      delete[] data_in_file;
      CloseHandle(h_save_file);
      return log.Log(Logger::LogLevel::error,
                     L"ERROR: WriteDataFromFile failed! cur_block:" +
                         std::to_wstring(cur_block) + L" bytes_written: " +
                         std::to_wstring(total_bytes_written) + L" bytes!");
    }
    total_bytes_written += bytes_to_write;
    cur_block = (cur_block + 1) % num_blocks;
    ;
    iteration_count++;
  };

  /* Exceeded maximum iterations? */
  if (iteration_count >= MAX_ITERATIONS &&
      total_bytes_written < total_bytes_to_write) {
    delete[] data_in_file;
    CloseHandle(h_save_file);
    return log.Log(Logger::LogLevel::error,
                   L"ERROR: SaveFile exceeded maxmimum iterations, possible "
                   L"corrupted state!");
  }

  /* Everything ok */
  delete[] data_in_file;
  CloseHandle(h_file);
  return true;
}

}  // namespace muehle
