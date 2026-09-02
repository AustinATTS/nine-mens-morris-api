#include "muehle/compressor/file.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>

namespace muehle {

/* Constructor */
compressor::File::File(GeneralLib& comp) : comp{&comp} {
  footer.used_lib = comp.GetLibId();
}

/* Destructor */
compressor::File::~File() {
  for (auto& cur_tmp_file : tmp_files) {
    delete cur_tmp_file;
  }
  tmp_files.clear();
  if (fs.is_open()) {
    fs.close();
  }
}

/* Open the compressed file and reads in the footer information. */
bool compressor::File::Open(std::wstring const& file_path, bool only_read) {
  if (!comp) {
    return false;
  }
  if (fs.is_open()) {
    return false;
  }

  /* convert std::wstring to string */
  std::filesystem::path file_path_str = file_path;

  /* file does not exist */
  if (!std::filesystem::exists(file_path_str)) {
    if (only_read) {
      return false;
    }

    /* create directory if it does not exist */
    if (file_path_str.has_parent_path() &&
        !std::filesystem::exists(file_path_str.parent_path())) {
      try {
        if (!std::filesystem::create_directories(file_path_str.parent_path())) {
          return false;
        }
      } catch (const std::exception&) {
        return false;
      }
    }

    /* open file */
    fs.open(file_path_str,
            std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

    if (!fs.good() || !fs.is_open()) {
      return false;
    }

  } else {
    /* open file */
    fs.open(file_path_str, only_read
                               ? std::ios::in | std::ios::binary | std::ios::ate
                               : std::ios::in | std::ios::out |
                                     std::ios::binary | std::ios::ate);

    /* read footer */
    footer.Read(fs, comp->GetLibId());
  }

  read_only_mode = only_read;
  return true;
}

/* Rewrites the compressed file, by copying the data from the temporary files,
 * deletes the temporary files and clears all data in memory. */
bool compressor::File::Close() {
  if (!fs.good()) {
    return false;
  }
  if (!fs.is_open()) {
    return false;
  }
  Flush();
  footer.Clear();
  fs.close();
  return true;
}

/* Returns true if the file is open. */
bool compressor::File::IsOpen() { return fs.is_open(); }

/* Returns a list of all keys in the file. */
std::vector<std::wstring> compressor::File::GetKeys() {
  std::vector<std::wstring> keys;
  if (!fs.good()) {
    return keys;
  }
  if (!fs.is_open()) {
    return keys;
  }
  for (auto& cur_section :
       footer.sections) { /* get all keys from the footer, which are already */
                          /* written to the compressed file */
    keys.push_back(cur_section.key_name);
  }
  for (auto& cur_tmp_file :
       tmp_files) { /* get all keys from the temporary files */
    keys.push_back(cur_tmp_file->GetKeyName());
  }
  sort(keys.begin(), keys.end()); /* sort keys */
  keys.erase(unique(keys.begin(), keys.end()),
             keys.end()); /* remove duplicates */
  return keys;
}

/* Returns true if a key exists in the file. */
bool compressor::File::DoesKeyExist(std::wstring const& key) {
  if (!fs.good()) {
    return false;
  }
  if (!fs.is_open()) {
    return false;
  }
  return footer.DoesKeyExist(key) || TmpFile::DoesExist(key);
}

/* Reads data from the temporary and compressed files for one section. */
bool compressor::File::Read(std::wstring const& key, long long position,
                            long long num_bytes, void* p_bytes) {
  /* checks */
  if (!num_bytes) {
    return false; /* some bytes neds to be read */
  }
  if (!comp) {
    return false; /* a compressor must be available */
  }
  if (!p_bytes) {
    return false; /* data pointer must not be nullptr */
  }
  if (!fs.good()) {
    return false; /* a file must be open */
  }
  if (!fs.is_open()) {
    return false; /* a file must be open */
  }

  /* if there is already a temporary file for the section, then read from it */
  if (TmpFile::DoesExist(key)) {
    return GetTmpFile(key).Read(position, num_bytes, p_bytes);
  } else {
    if (!footer.DoesKeyExist(key)) {
      return false; /* key must exist */
    }
    return ReadFromCompressed(key, position, num_bytes, p_bytes);
  }

  return true;
}

/* Returns the temporary file for a section. */
compressor::File::TmpFile& compressor::File::GetTmpFile(
    std::wstring const& key) {
  /* check if the temporary file already exists */
  for (auto& cur_tmp_file : tmp_files) {
    if (cur_tmp_file->GetKeyName() == key) {
      return *cur_tmp_file;
    }
  }
  /* create a new temporary file */
  tmp_files.push_back(new TmpFile(key));
  return *tmp_files.back();
}

/* Reads data from the compressed files for one section. */
bool compressor::File::ReadFromCompressed(std::wstring const& key,
                                          long long position,
                                          long long num_bytes, void* p_bytes) {
  if (!num_bytes) {
    return false; /* some bytes neds to be read */
  }
  if (!comp) {
    return false; /* a compressor must be available */
  }
  if (!p_bytes) {
    return false; /* data pointer must not be nullptr */
  }
  if (!fs.good()) {
    return false; /* a file must be open */
  }
  if (!fs.is_open()) {
    return false; /* a file must be open */
  }

  /* locals */
  if (!footer.DoesKeyExist(key)) {
    return false;
  }
  return footer.GetSection(key).ReadData(fs, footer, *comp, p_bytes, num_bytes,
                                         position);
}

/* Write data to the compressed file, which is compressed and stored in blocks.
 * However, the data is not written to the file, but stored in temporary files.
 * One file for each section, allowing to write data in parallel and without
 * fragmentation. */
bool compressor::File::Write(std::wstring const& key, long long position,
                             long long num_bytes, const void* p_bytes) {
  /* check preconditions */
  if (read_only_mode) {
    return false; /* no appending when in read only mode */
  }
  if (key.length() > max_key_length) {
    return false; /* limit key length */
  }
  if (!p_bytes) {
    return false; /* data pointer must not be nullptr */
  }
  if (!num_bytes) {
    return false; /* some bytes neds to be written */
  }
  if (!comp) {
    return false; /* a compressor must be available */
  }
  if (!fs.good()) {
    return false; /* a file must be open */
  }
  if (!fs.is_open()) {
    return false; /* a file must be open */
  }

  /* create or open a temporary file for the section */
  TmpFile& cur_tmp_file = GetTmpFile(key);

  /* When the section already exist in the compressed file, and the temporary
   * file is new, then copy data to the temporary file, so the the data can be
   * modified. */
  if (cur_tmp_file.GetSize() == 0 && footer.DoesKeyExist(key)) {
    /* read data from compressed file in chucks of footer.block_size_in_bytes,
     * and write to the temporary file */
    SectionInfo& cur_section = footer.GetSection(key);
    long long num_bytes_resting = cur_section.uncompressed_size;
    long long offset_within_section = 0;
    std::vector<char> tmp_data(footer.block_size_in_bytes);
    while (num_bytes_resting) {
      long long num_bytes_to_read =
          std::min((long long)footer.block_size_in_bytes, num_bytes_resting);
      if (!ReadFromCompressed(key, offset_within_section, num_bytes_to_read,
                              &tmp_data[0])) {
        return false;
      }
      cur_tmp_file.Write(0, num_bytes_to_read, &tmp_data[0]);
      num_bytes_resting -= num_bytes_to_read;
    }
  }

  /* write user data to temporary file */
  return cur_tmp_file.Write(position, num_bytes, p_bytes);
}

/* Copies the data from the temporary files to the compressed file. */
bool compressor::File::Flush() {
  /* check preconditions */
  if (read_only_mode) {
    return false; /* no appending when in read only mode */
  }
  if (!comp) {
    return false; /* a compressor must be available */
  }
  if (!fs.good()) {
    return false; /* a file must be open */
  }
  if (!fs.is_open()) {
    return false; /* a file must be open */
  }
  if (tmp_files.size() == 0) {
    return false; /* no data to write */
  }

  /* locals */
  std::vector<std::filesystem::path> tmp_file_paths;

  /* get all temporary file paths */
  for (auto& cur_tmp_file : tmp_files) {
    tmp_file_paths.push_back(cur_tmp_file->GetFilePath());
  }

  /* loop over all files in the temporary directory */
  for (auto& cur_tmp_file : tmp_files) {
    /* get key from file name and num_bytes from file size */
    std::wstring key = cur_tmp_file->GetKeyName();
    long long num_bytes = cur_tmp_file->GetSize();
    SectionInfo cur_section;

    /* print info */
    std::wstringstream ss;
    ss << L"Flushing section \"" << key << L"\" with " << num_bytes
       << L" bytes." << std::endl;
    comp->Print(ss, 2);

    /* does the section already exist? */
    if (footer.DoesKeyExist(key)) {
      cur_section = footer.GetSection(key);
    }

    /* set section properties */
    cur_section.offset_in_file = footer.file_info_offset_in_file;
    cur_section.uncompressed_size = num_bytes;
    cur_section.compressed_size = 0;
    cur_section.num_blocks =
        (unsigned int)num_bytes / footer.block_size_in_bytes + 1;
    cur_section.key_length_in_bytes = key.length() * sizeof(wchar_t);
    cur_section.key_name = key;
    cur_section.section_id = footer.sections.size();
    cur_section.blocks.resize(cur_section.num_blocks);

    /* read data from temporary file in chucks of footer.block_size_in_bytes,
     * compress and write to the compressed file */
    if (!cur_section.WriteData(fs, footer, *comp, *cur_tmp_file)) {
      return false;
    }

    /* update file info */
    footer.dictionary[key] = cur_section.section_id;
    footer.sections.push_back(cur_section);

    /* update footer info */
    footer.num_sections++;
    footer.file_info_offset_in_file +=
        cur_section.compressed_size +
        cur_section.num_blocks * sizeof(SectionInfo::BlockInfo);
    footer.footer_offset_in_file +=
        cur_section.compressed_size +
        cur_section.num_blocks * sizeof(SectionInfo::BlockInfo);
  }

  /* write footer */
  footer.Write(fs);

  /* delete all temporary files */
  for (auto& cur_tmp_file : tmp_files) {
    delete cur_tmp_file;
  }
  tmp_files.clear();

  return true;
}

/* By default the block size is 1000 bytes. This function allows to change the
 * block size, but only before flushing. */
bool compressor::File::SetBlockSize(unsigned int new_size_in_bytes) {
  if (footer.num_sections || footer.sections.size()) {
    return false;
  }
  footer.block_size_in_bytes = new_size_in_bytes;
  return true;
}

/* Returns the size of the uncompressed section, but only after flushing. */
long long compressor::File::GetSizeOfUncompressedSection(
    std::wstring const& key) {
  if (TmpFile::DoesExist(key)) {
    return GetTmpFile(key).GetSize();
  }
  if (!footer.DoesKeyExist(key)) {
    return 0;
  }
  return footer.GetSection(key).uncompressed_size;
}

/* Returns the size of the compressed section, but only after flushing. */
long long compressor::File::GetSizeOfCompressedSection(
    std::wstring const& key) {
  if (!footer.DoesKeyExist(key)) {
    return 0;
  }
  return footer.GetSection(key).compressed_size;
}

/* Clears the footer. */
void compressor::File::FooterStruct::Clear() {
  file_info_offset_in_file = 0;
  footer_offset_in_file = 0;
  num_sections = 0;
  sections_offset_in_file = 0;
  dictionary.clear();
  sections.clear();
}

/* Returns the section info for a given key. If the key does not exist, then a
 * new section is created. */
compressor::File::SectionInfo& compressor::File::FooterStruct::GetSection(
    std::wstring const& key) {
  auto section_id_it = dictionary.find(key);
  if (section_id_it == dictionary.end()) {
    sections.push_back(SectionInfo());
    sections.back().key_name = key;
    sections.back().section_id = sections.size();
    dictionary[key] = sections.size();
    return sections.back();
  } else {
    return sections[section_id_it->second];
  }
}

/* Writes the footer to the file. */
bool compressor::File::FooterStruct::Write(std::fstream& fs) {
  /* check preconditions */
  if (!fs.good()) {
    return false; /* a database file must be open */
  }
  if (!fs.is_open()) {
    return false; /* a database file must be open */
  }
  if (num_sections == 0) {
    return false; /* must have at least one section */
  }
  if (file_info_offset_in_file == 0) {
    return false;
  } /* file info must be present */
  if (sections.size() == 0) {
    return false; /* must have at least one section */
  }
  if (version_id != 1) {
    return false; /* check file version */
  }

  /* set file pointer, to write infos about the sections */
  fs.seekg(file_info_offset_in_file, std::ios_base::beg);

  size_t section_id = 0;
  for (auto& cur_section_info : sections) {
    cur_section_info.Write(fs, *this);
    section_id++;
  }

  /* set file pointer, to write the footer */
  footer_offset_in_file = fs.tellg();

  /* write footer */
  fs.write((char*)this, sizeof(type_id) + sizeof(version_id) +
                            sizeof(num_sections) + sizeof(block_size_in_bytes) +
                            sizeof(file_info_offset_in_file) +
                            sizeof(footer_offset_in_file) +
                            sizeof(sections_offset_in_file) + sizeof(used_lib));

  return true;
}

/* Reads the footer from the file. */
bool compressor::File::FooterStruct::Read(std::fstream& fs,
                                          GeneralLib::LibId lib_id) {
  /* check preconditions */
  if (!fs.good()) {
    return false;
  }
  if (!fs.is_open()) {
    return false;
  }

  /* read footer, which is at the end of the file */
  size_t size_of_footer = sizeof(type_id) + sizeof(version_id) +
                          sizeof(num_sections) + sizeof(block_size_in_bytes) +
                          sizeof(file_info_offset_in_file) +
                          sizeof(footer_offset_in_file) +
                          sizeof(sections_offset_in_file) + sizeof(used_lib);
  fs.seekg(-1 * (int)size_of_footer, std::ios_base::end);
  fs.read((char*)this, size_of_footer);

  if (num_sections == 0) {
    return false; /* must have at least one section */
  }
  if (file_info_offset_in_file == 0) {
    return false;
  } /* file info must be present */
  if (type_id != 0x7d67) {
    return false;
  } /* does type_id match? */
  if (used_lib != lib_id) {
    return false;
  } /* does lib_id match? */
  if (version_id != 1) {
    return false;
  } /* check file version */

  /* set file pointer, to read infos about the sections */
  fs.seekg(file_info_offset_in_file, std::ios_base::beg);

  /* read sections */
  sections.resize(num_sections);
  for (auto& cur_section_info : sections) {
    if (!cur_section_info.Read(fs, *this)) {
      return false;
    }
    dictionary[cur_section_info.key_name] = cur_section_info.section_id;
  }
  return true;
}

/* Writes the section info to the file, at the current file pointer position. */
bool compressor::File::SectionInfo::Write(std::fstream& fs,
                                          FooterStruct& footer) {
  /* check preconditions */
  if (!fs.good()) {
    return false;
  }
  if (!fs.is_open()) {
    return false;
  }
  if (num_blocks == 0) {
    return false;
  }

  /* write section info */
  unsigned int dummy_unit32 = 0;
  fs.write((char*)&offset_in_file, sizeof(offset_in_file));
  fs.write((char*)&uncompressed_size, sizeof(uncompressed_size));
  fs.write((char*)&compressed_size, sizeof(compressed_size));
  fs.write((char*)&num_blocks, sizeof(num_blocks));
  fs.write((char*)&dummy_unit32,
           sizeof(dummy_unit32)); /* 4 bytes were formerly used for padding */
  fs.write((char*)&section_id, sizeof(section_id));
  fs.write((char*)&dummy_unit32,
           sizeof(dummy_unit32)); /* 4 bytes were formerly used for padding */
  fs.write((char*)&key_length_in_bytes, sizeof(key_length_in_bytes));
  fs.write((char*)key_name.c_str(), key_length_in_bytes);

  /* blocks are already stored in each section */

  return true;
}

/* Reads the section info from the file, from the current file pointer position.
 */
bool compressor::File::SectionInfo::Read(std::fstream& fs,
                                         FooterStruct& footer) {
  /* check preconditions */
  if (!fs.good()) {
    return false;
  }
  if (!fs.is_open()) {
    return false;
  }

  /* locals */
  std::vector<wchar_t> key_name_tmp(max_key_length, L'\0');
  unsigned int dummy_unit32;

  /* read section info */
  fs.read((char*)&offset_in_file, sizeof(offset_in_file));
  fs.read((char*)&uncompressed_size, sizeof(uncompressed_size));
  fs.read((char*)&compressed_size, sizeof(compressed_size));
  fs.read((char*)&num_blocks, sizeof(num_blocks));
  fs.read((char*)&dummy_unit32,
          sizeof(dummy_unit32)); /* 4 bytes were formerly used for padding */
  fs.read((char*)&section_id, sizeof(section_id));
  fs.read((char*)&dummy_unit32,
          sizeof(dummy_unit32)); /* 4 bytes were formerly used for padding */
  fs.read((char*)&key_length_in_bytes, sizeof(key_length_in_bytes));

  /* keys */
  if (key_length_in_bytes > max_key_length) {
    fs.close();
    return false;
  }
  fs.read((char*)&key_name_tmp[0], key_length_in_bytes);
  key_name_tmp[key_length_in_bytes] = L'\0';
  key_name.assign(max_key_length, L'\0');
  key_name.assign(&key_name_tmp[0]);
  return true;
}

/* Writes the data from tmp_file to the compressed file. IMPORTANT: The whole
 * section must be written at once. */
bool compressor::File::SectionInfo::WriteData(std::fstream& fs,
                                              FooterStruct& footer,
                                              GeneralLib& comp,
                                              TmpFile& cur_tmp_file,
                                              bool force_single_threading) {
  /* set file pointer */
  fs.seekp(offset_in_file, std::ios_base::beg);

  /* BUG: Multithreading does not work */
  force_single_threading = true;

  /* single thread compression */
  if (force_single_threading || cur_tmp_file.GetSize() < 1000000) {
    std::vector<char> uncompressed_data;
    std::vector<char> compressed_data;
    long long num_bytes_resting = cur_tmp_file.GetSize();
    unsigned int n_bytes_compressed = 0;
    unsigned int cur_offset_in_section = 0;
    unsigned int cur_offset_in_tmp_file = 0;
    size_t block_id = 0;

    compressed_data.resize(
        comp.EstimateMaxSizeOfCompressedData(footer.block_size_in_bytes));
    uncompressed_data.resize(footer.block_size_in_bytes);

    while (num_bytes_resting) {
      /* locals */
      unsigned int num_bytes_to_compress = (unsigned int)std::min(
          (long long)footer.block_size_in_bytes, num_bytes_resting);
      cur_tmp_file.Read(cur_offset_in_tmp_file, num_bytes_to_compress,
                        &uncompressed_data[0]);

      /* compress data */
      if (!comp.Compress(&compressed_data[0], &uncompressed_data[0],
                         num_bytes_to_compress, n_bytes_compressed)) {
        return false;
      }

      /* write to file */
      fs.write(&compressed_data[0], n_bytes_compressed);

      compressed_size += n_bytes_compressed;
      num_bytes_resting -= num_bytes_to_compress;
      blocks[block_id].compressed_size = n_bytes_compressed;
      blocks[block_id].offset_in_section = cur_offset_in_section;
      cur_offset_in_section += n_bytes_compressed;
      cur_offset_in_tmp_file += num_bytes_to_compress;
      block_id++;
    }

    /* use multi-threading and a buffered file for compression */
  } else {
    /* Multi-threaded compression with buffered file */
    const size_t num_threads = std::thread::hardware_concurrency();
    size_t total_blocks =
        (size_t)((cur_tmp_file.GetSize() + footer.block_size_in_bytes - 1) /
                 footer.block_size_in_bytes);

    std::vector<char> uncompressed_data;
    std::vector<char> compressed_data;

    compressed_data.resize(
        comp.EstimateMaxSizeOfCompressedData(footer.block_size_in_bytes));
    uncompressed_data.resize(footer.block_size_in_bytes);

    std::vector<std::future<std::pair<std::vector<char>, unsigned int>>>
        futures;
    std::vector<std::vector<char>> compressed_blocks(total_blocks);
    std::vector<unsigned int> compressed_sizes(total_blocks, 0);

    for (size_t block_id = 0; block_id < total_blocks; ++block_id) {
      futures.push_back(std::async(std::launch::async, [&, block_id]() {
        std::vector<char> block_uncompressed(footer.block_size_in_bytes);
        std::vector<char> block_compressed(
            comp.EstimateMaxSizeOfCompressedData(footer.block_size_in_bytes));
        unsigned int n_bytes_compressed = 0;
        long long offset = block_id * footer.block_size_in_bytes;
        unsigned int num_bytes_to_compress =
            (unsigned int)std::min((long long)footer.block_size_in_bytes,
                                   cur_tmp_file.GetSize() - offset);
        cur_tmp_file.Read(offset, num_bytes_to_compress,
                          &block_uncompressed[0]);
        if (!comp.Compress(&block_compressed[0], &block_uncompressed[0],
                           num_bytes_to_compress, n_bytes_compressed)) {
          throw std::runtime_error("Compression failed in thread");
        }
        block_compressed.resize(n_bytes_compressed);
        return std::make_pair(std::move(block_compressed), n_bytes_compressed);
      }));
      /* Limit number of concurrent threads */
      if (futures.size() >= num_threads) {
        for (auto& fut : futures) {
          auto result = fut.get();
          size_t idx = &fut - &futures[0] + block_id - futures.size() + 1;
          compressed_blocks[idx] = std::move(result.first);
          compressed_sizes[idx] = result.second;
        }
        futures.clear();
      }
    }

    /* Collect remaining futures */
    size_t start_idx = total_blocks - futures.size();
    for (size_t i = 0; i < futures.size(); ++i) {
      auto result = futures[i].get();
      compressed_blocks[start_idx + i] = std::move(result.first);
      compressed_sizes[start_idx + i] = result.second;
    }

    /* Write all compressed blocks to file and update block info */
    unsigned int cur_offset_in_section = 0;
    compressed_size = 0;
    for (size_t block_id = 0; block_id < total_blocks; ++block_id) {
      fs.write(compressed_blocks[block_id].data(), compressed_sizes[block_id]);
      blocks[block_id].compressed_size = compressed_sizes[block_id];
      blocks[block_id].offset_in_section = cur_offset_in_section;
      cur_offset_in_section += compressed_sizes[block_id];
      compressed_size += compressed_sizes[block_id];
    }
  }

  /* write all block infos to file */
  for (auto& cur_block : blocks) {
    fs.write((char*)&cur_block, sizeof(cur_block));
  }

  return true;
}

/* Reads the data from the file. This may happen as random access. */
bool compressor::File::SectionInfo::ReadData(std::fstream& fs,
                                             FooterStruct& footer,
                                             GeneralLib& comp, void* p_bytes,
                                             long long num_bytes,
                                             long long position) {
  /* check preconditions */
  if (fs.good() == false) {
    return false;
  }
  if (fs.is_open() == false) {
    return false;
  }
  if (p_bytes == nullptr) {
    return false;
  }
  if (num_bytes <= 0) {
    return false;
  }
  if (position < 0) {
    return false;
  }
  if (position >= uncompressed_size) {
    return false;
  }
  if (position > num_blocks * footer.block_size_in_bytes) {
    return false;
  }

  /* locals */
  long long block_id = position / footer.block_size_in_bytes; /* block index */
  long long offset_inside_block =
      position % footer.block_size_in_bytes; /* offset inside block */
  long long num_bytes_resting =
      num_bytes; /* number of bytes still to read and to copy to p_bytes */
  char* p_block = (char*)p_bytes; /* moving pointer to the current position in
                                     p_bytes during the copy process */
  std::vector<char> compressed_data;   /* buffer for compressed data */
  std::vector<char> uncompressed_data; /* buffer for uncompressed data */
  long long num_resting_comp_bytes =
      compressed_size; /* number of bytes still available in the compressed
                          section */
  unsigned int n_bytes_decompressed =
      0; /* number of bytes decompressed in the current block */

  /* is block id valid? */
  if (block_id >= num_blocks) {
    return false;
  }

  /* seek to beginning of section */
  compressed_data.resize(
      comp.EstimateMaxSizeOfCompressedData(footer.block_size_in_bytes));
  uncompressed_data.resize(footer.block_size_in_bytes);

  /* if blockInfo not loaded yet than do it now */
  if (blocks.size() == 0) {
    fs.seekg(offset_in_file + compressed_size, std::ios_base::beg);
    blocks.resize(num_blocks);
    for (auto& cur_block : blocks) {
      fs.read((char*)&cur_block, sizeof(cur_block));
    }
  }

  /* seekg to relevant block */
  fs.seekg(offset_in_file, std::ios_base::beg);
  fs.seekg(blocks[block_id].offset_in_section, std::ios_base::cur);
  num_resting_comp_bytes -=
      blocks[block_id]
          .offset_in_section; /* update num_resting_comp_bytes, because we have
                                 seeked to the block */

  /* loop through all remaining blocks only single threading here */
  while (num_bytes_resting) {
    /* is block id valid? */
    if (block_id >= num_blocks) {
      return false;
    }

    /* read current block from compresed file */
    fs.read(&compressed_data[0], blocks[block_id].compressed_size);

    /* decompress data */
    if (!comp.Decompress(&uncompressed_data[0], &compressed_data[0],
                         blocks[block_id].compressed_size,
                         n_bytes_decompressed)) {
      return false;
    }

    /* copy data to passed pointer from caller */
    long long num_bytes_to_copy =
        std::min({(long long)n_bytes_decompressed, num_bytes_resting,
                  (long long)footer.block_size_in_bytes - offset_inside_block});
    memcpy(p_block, &uncompressed_data[offset_inside_block], num_bytes_to_copy);

    /* goto next block */
    num_bytes_resting -= num_bytes_to_copy;
    num_resting_comp_bytes -= std::min(
        (long long)blocks[block_id].compressed_size, num_resting_comp_bytes);
    p_block += num_bytes_to_copy;
    offset_inside_block = 0;
    block_id++;
  }

  return true;
}

/* Constructor */
compressor::File::TmpFile::TmpFile(std::wstring const& key_name)
    : key_name{key_name} {
  file_path = (std::filesystem::temp_directory_path() / L"compressor" /
               (key_name + L".dat"))
                  .wstring();
  std::filesystem::path file_path_str = file_path;
  std::filesystem::create_directories(file_path_str.parent_path());
  fs_tmp.open(file_path_str, std::ios::in | std::ios::out | std::ios::binary |
                                 std::ios::trunc);
  if (!fs_tmp.good() || !fs_tmp.is_open()) {
    throw std::runtime_error("Could not create temporary file for section.");
  }
  fs_tmp.close();
}

/* Destructor */
compressor::File::TmpFile::~TmpFile() {
  if (fs_tmp.is_open()) {
    fs_tmp.close();
  }
  std::filesystem::remove(file_path);
}

/* Returns true if a temporary file exists for a section. */
bool compressor::File::TmpFile::DoesExist(std::wstring const& key_name) {
  std::filesystem::path tmp_file_path =
      (std::filesystem::temp_directory_path() / L"compressor" /
       (key_name + L".dat"))
          .c_str();
  return std::filesystem::exists(tmp_file_path);
}

/* Opens the temporary file if not already open. */
bool compressor::File::TmpFile::OpenIfNotOpen() {
  if (!fs_tmp.good() || !fs_tmp.is_open()) {
    std::filesystem::path file_path_str = file_path;
    fs_tmp.open(file_path_str.string(),
                std::ios::in | std::ios::out | std::ios::binary);
    if (!fs_tmp.good() || !fs_tmp.is_open()) {
      return false;
    }
  }
  return true;
}

/* Reads data from the temporary file. */
bool compressor::File::TmpFile::Read(long long position, long long num_bytes,
                                     void* p_bytes) {
  if (!p_bytes) {
    return false;
  }
  if (position < 0) {
    return false;
  }
  if (num_bytes <= 0) {
    return false;
  }
  if (position + num_bytes > GetSize()) {
    return false;
  }
  if (!OpenIfNotOpen()) {
    return false;
  }
  fs_tmp.seekg(position, std::ios_base::beg);
  fs_tmp.read((char*)p_bytes, num_bytes);
  fs_tmp.close();
  return true;
}

/* Writes data to the temporary file. */
bool compressor::File::TmpFile::Write(long long position, long long num_bytes,
                                      const void* p_bytes) {
  if (!p_bytes) {
    return false;
  }
  if (position < 0) {
    return false;
  }
  if (num_bytes <= 0) {
    return false;
  }
  if (!OpenIfNotOpen()) {
    return false;
  }
  fs_tmp.seekp(position, std::ios_base::beg);
  fs_tmp.write((char*)p_bytes, num_bytes);
  fs_tmp.close();
  return true;
}

/* Returns the size of the temporary file. */
long long compressor::File::TmpFile::GetSize() {
  if (!OpenIfNotOpen()) {
    return -1;
  }
  fs_tmp.seekg(0, std::ios_base::end);
  long long size = fs_tmp.tellg();
  fs_tmp.close();
  return size;
}

}  // namespace muehle