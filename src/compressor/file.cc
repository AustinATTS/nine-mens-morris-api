#include "muehle/compressor/file.h"

namespace muehle {

// Implementation goes here.

compressor::File::File(GeneralLib& comp) {
}

compressor::File::~File() {
}

bool compressor::File::Open(std::wstring const& file_path, bool only_read) {
}

bool compressor::File::Close() {
}

bool compressor::File::IsOpen() {
}

bool compressor::File::Read(std::wstring const& key, long long position,
    long long num_bytes, void* p_bytes) {
}

bool compressor::File::Write(std::wstring const& key, long long position,
    long long num_bytes, void* p_bytes) {
}

bool compressor::File::Flush() {
}

bool compressor::File::SetBlockSize(unsigned int new_size_in_bytes) {
}

long long compressor::File::GetSizeOfUncompressedSection(
    std::wstring const& key) {
}

long long compressor::File::
GetSizeOfCompressedSection(std::wstring const& key) {
}

std::vector<std::wstring> compressor::File::GetKeys() {
}

bool compressor::File::DoesKeyExist(std::wstring const& key) {
}

bool compressor::File::SectionInfo::Write(std::fstream& fs,
    FooterStruct& footer) {
}

bool compressor::File::SectionInfo::
Read(std::fstream& fs, FooterStruct& footer) {
}

bool compressor::File::SectionInfo::WriteData(std::fstream& fs,
    FooterStruct& footer, GeneralLib& comp, TmpFile& tmp_file,
    bool force_single_threading) {
}

bool compressor::File::SectionInfo::ReadData(std::fstream& fs,
    FooterStruct& footer, GeneralLib& comp, void* p_bytes, long long num_bytes,
    long long position) {
}

bool compressor::File::FooterStruct::Write(std::fstream& fs) {
}

bool compressor::File::FooterStruct::Read(std::fstream& fs,
    GeneralLib::LibId lib_id) {
}

void compressor::File::FooterStruct::Clear() {
}

compressor::File::SectionInfo& compressor::File::FooterStruct::GetSection(
    std::wstring const& key) {
}

compressor::File::TmpFile::TmpFile(std::wstring& key_name) {
}

compressor::File::TmpFile::~TmpFile() {
}

long long compressor::File::TmpFile::GetSize() {
}

bool compressor::File::TmpFile::Write(long long position, long long num_bytes,
    const void* p_bytes) {
}

bool compressor::File::TmpFile::Read(long long position, long long num_bytes,
    void* p_bytes) {
}

bool compressor::File::TmpFile::DoesExist(std::wstring const& key_name) {
}

bool compressor::File::TmpFile::OpenIfNotOpen() {
}

compressor::File::TmpFile& compressor::File::GetTmpFile(std::wstring& key) {
}

bool compressor::File::ReadFromCompressed(std::wstring const& key,
    long long position, long long num_bytes, void* p_bytes) {
}
}  // namespace muehle
