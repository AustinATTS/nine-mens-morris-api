#ifndef MUEHLE_COMPRESSOR_FILE_H_
#define MUEHLE_COMPRESSOR_FILE_H_

/* Standard library & win32 API */
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif /* NOMINMAX */
#include <windows.h>
#endif /* _WIN32 */
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "muehle/compressor/general_lib.h"

namespace muehle {
namespace compressor {

/* Class providing read/write functions for a compressed file.
 * It can handle very large files and is able to randomly read and write single
 * bytes of the file. Any C++ compression library can be used. The access to the
 * file is done in named sections, addressed by a string key. The actual file is
 * written in the destructor during the Flush() function. Before, the sections
 * are written to temporary files. */
class File {
 public:
  File(GeneralLib& comp);
  ~File();

  bool Open(std::wstring const& file_path, bool only_read);
  bool Close();
  bool IsOpen();
  bool Read(std::wstring const& key, long long position, long long num_bytes,
            void* p_bytes);
  bool Write(std::wstring const& key, long long position, long long num_bytes,
             const void* p_bytes);
  bool Flush();
  bool SetBlockSize(unsigned int new_size_in_bytes);
  long long GetSizeOfUncompressedSection(std::wstring const& key);
  long long GetSizeOfCompressedSection(std::wstring const& key);
  std::vector<std::wstring> GetKeys();
  bool DoesKeyExist(std::wstring const& key);

  /* Each section is stored in blocks of a fixed size. This enables random read
   * access. Instead of a header, a footer is used here. */

  class FooterStruct;
  class TmpFile;

  /* Section of the file containing the data */
  struct SectionInfo {
    struct BlockInfo {
      /* Data copied directly to file */

      /* Offset in bytes within the section */
      unsigned int offset_in_section = 0;

      /* Size of the compressed block in bytes */
      unsigned int compressed_size = 0;
    };

    /* Data copied directly to file */

    /* Offset in bytes within the file */
    long long offset_in_file = 0;

    /* Size of the uncompressed section in bytes */
    long long uncompressed_size = 0;

    /* Size of the compressed section in bytres */
    long long compressed_size = 0;

    /* Number of blocks in the section */
    unsigned int num_blocks = 0;

    /* index of the section in the file */
    unsigned int section_id = 0;

    /* Length of the key in bytes */
    unsigned int key_length_in_bytes = 0;

    /* Abstract data (cannot be written to file) */

    /* Key of the section, by which the section is addressed in the file */
    std::wstring key_name;

    /* Data */
    std::vector<BlockInfo> blocks;

    /* Functions */
    bool Write(std::fstream& fs, FooterStruct& footer);
    bool Read(std::fstream& fs, FooterStruct& footer);
    bool WriteData(std::fstream& fs, FooterStruct& footer, GeneralLib& comp,
                   TmpFile& tmp_file, bool force_single_threading = false);
    bool ReadData(std::fstream& fs, FooterStruct& footer, GeneralLib& comp,
                  void* p_bytes, long long num_bytes, long long position);
  };

  /* Footer of the file, containing infos about the sections */
  struct FooterStruct {
    /* Data copied directly to file */

    /* Unique ID of the file format */
    short type_id = 0x7d67;

    /* Version of the file format */
    short version_id = 1;

    /* Number of sections in the file */
    unsigned int num_sections = 0;

    /* Offset in file where the sections starts */
    long long sections_offset_in_file = 0;

    /* offset in file where the footer starts */
    long long footer_offset_in_file = 0;

    /* Offset in file where the file info starts */
    long long file_info_offset_in_file = 0;

    /* Each section is separated in blocks to enable random read access */
    unsigned int block_size_in_bytes = 1000;

    /* Id of the used compression algorithm/library */
    GeneralLib::LibId used_lib = GeneralLib::LibId::undefined;

    /* Abstract data (cannot be written directly to file) */

    /* Mapping from keys to section indices */
    std::map<std::wstring, size_t> dictionary;

    /* Data */
    std::vector<SectionInfo> sections;

    /* Functions */
    bool DoesKeyExist(std::wstring const& key) {
      return dictionary.find(key) != dictionary.end();
    };
    bool Write(std::fstream& fs);
    bool Read(std::fstream& fs, GeneralLib::LibId lib_id);
    void Clear();
    SectionInfo& GetSection(std::wstring const& key);
  };

  /* Uncompressed temporary file for writing the sections, used before it is
   * written to the actual compressed file this allows fast rendering and
   * writing of the sections, even if the file is very large */
  class TmpFile {
   public:
    TmpFile(std::wstring const& key_name);
    ~TmpFile();

    std::wstring const& GetKeyName() {
      return key_name;
    };
    std::wstring const& GetFilePath() {
      return file_path;
    };
    long long GetSize();
    bool Write(long long position, long long num_bytes, const void* p_bytes);
    bool Read(long long position, long long num_bytes, void* p_bytes);

    static bool DoesExist(std::wstring const& key_name);

   private:
    /* Key of the section */
    std::wstring key_name;

    /* Path to the temporary file */
    std::wstring file_path;

    /* File stream for reading/writing the temporary file */
    std::fstream fs_tmp;

    bool OpenIfNotOpen();
  };

 private:
  /* Each section in the file is identified by a string key. This is the maximum
   * length of the key. */
  static const size_t max_key_length = 240;

  /* Footer of the file, containing infos about the sections */
  FooterStruct footer;

  /* File stream for reading/writing the actual file on the disk */
  std::fstream fs;

  /* Pointer to the compression library */
  GeneralLib* comp = nullptr;

  /* If true, no writing is allowed */
  bool read_only_mode = false;

  /* Temporary files for writing the sections */
  std::vector<TmpFile*> tmp_files;

  TmpFile& GetTmpFile(std::wstring const& key);
  bool ReadFromCompressed(std::wstring const& key, long long position,
                          long long num_bytes, void* p_bytes);
};

} /* namespace compressor */
} /* namespace muehle */

#ifdef _WIN32
#include "muehle/compressor/win_comp_api.h"
#else /* _WIN32 */
#include "muehle/compressor/zlib_api.h"
#endif /* _WIN32 */

#endif /* MUEHLE_COMPRESSOR_FILE_H_ */
