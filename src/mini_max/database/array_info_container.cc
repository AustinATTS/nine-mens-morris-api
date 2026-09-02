#include "muehle/mini_max/database/array_info_container.h"

#ifdef _WIN32
#include <windows.h>  // QueryPerformanceCounter, QueryPerformanceFrequency
#else
#include "muehle/win_32_compat.h"
#endif

namespace muehle {

// Desc: Register a new array in the database
//		 Layer number must be smaller than numLayers, passed in init()
// 		 Size must be greater than 0, compressedSize can be 0
//		 Type must be valid
//		 If the array already exists, the function returns false
// 		 If the array is added, the function returns true
//-----------------------------------------------------------------------------
bool mini_max::database::ArrayInfoContainer::AddArray(
    unsigned int layer_number, ArrayInfoStruct::ArrayType type, long long size,
    long long compressed_size) {
  // checks
  if (!size) return false;
  if (num_layers <= layer_number) return false;
  if (type == ArrayInfoStruct::ArrayType::invalid) return false;
  if (type == ArrayInfoStruct::ArrayType::size) return false;

  // locals
  auto vector_array_index = GetVectorArrayIndex(layer_number, type);

  // thread safety
  std::lock_guard<std::mutex> lock(mutex);

  // if index is out of range, the function returns false
  if (vector_arrays.size() <= vector_array_index) return false;

  // If the array already exists, the function returns false
  if (vector_arrays[vector_array_index] != list_arrays.end()) return false;

  // create new info object and add to list
  ArrayInfoStruct ais;
  ais.belongs_to_layer = layer_number;
  ais.compresed_size_in_bytes = compressed_size;
  ais.size_in_bytes = size;
  ais.type = type;
  list_arrays.push_back(ais);

  // notify change
  ArrayInfoChange aic;
  aic.array_info = &list_arrays.back();
  aic.item_index = (unsigned int)list_arrays.size() - 1;
  array_infos_to_be_updated.push_back(aic);

  // save pointer of info in vector for direct access
  vector_arrays[vector_array_index] = (--list_arrays.end());

  // update total memory usage
  memory_used += size;

  // update GUI
  log.Log(Logger::LogLevel::trace, L"Allocated " + std::to_wstring(size) +
                                       L" bytes in memory for array type " +
                                       ais.GetArrTypeName() + L" of layer " +
                                       std::to_wstring(layer_number));

  return true;
}

// Desc: Remove an array from the database
//-----------------------------------------------------------------------------
bool mini_max::database::ArrayInfoContainer::RemoveArray(
    unsigned int layer_number, ArrayInfoStruct::ArrayType type, long long size,
    long long compressed_size) {
  // checks
  if (!size) return false;
  if (num_layers <= layer_number) return false;
  if (type == ArrayInfoStruct::ArrayType::size) return false;

  // locals
  auto vector_array_index = GetVectorArrayIndex(layer_number, type);

  // find info object in list
  std::lock_guard<std::mutex> lock(mutex);

  // if index is out of range, the function returns false
  if (vector_arrays.size() <= vector_array_index) return false;

  // If the array does not exist, the function returns false
  std::list<ArrayInfoStruct>::iterator& itr = vector_arrays[vector_array_index];
  if (list_arrays.size() == 0) return false;
  if (itr == list_arrays.end()) return false;

  // does sizes fit?
  if (itr->belongs_to_layer != layer_number || itr->type != type ||
      itr->size_in_bytes != size || itr->compresed_size_in_bytes != compressed_size)
    return false;

  // notify cahnge
  ArrayInfoChange aic;
  aic.array_info = NULL;
  aic.item_index = (unsigned int)std::distance(list_arrays.begin(), itr);
  array_infos_to_be_updated.push_back(aic);

  // delete item from list
  list_arrays.erase(itr);
  vector_arrays[vector_array_index] = list_arrays.end();

  // update total memory usage
  memory_used -= size;

  // update GUI
  log.Log(Logger::LogLevel::trace,
          L"Deallocated " + std::to_wstring(size) +
              L" bytes in memory for array type " +
              std::to_wstring(static_cast<unsigned int>(type)) + L" of layer " +
              std::to_wstring(layer_number));

  return true;
}

// Desc: Initialize the array info container
//-----------------------------------------------------------------------------
void mini_max::database::ArrayInfoContainer::Init(unsigned int num_layers) {
  std::lock_guard<std::mutex> lock(mutex);
  this->num_layers = num_layers;
  vector_arrays.resize(
      num_layers * static_cast<size_t>(ArrayInfoStruct::ArrayType::size),
      list_arrays.end());
  array_infos_to_be_updated.clear();
  list_arrays.clear();
}

// Desc: Return true if there are any array infos to update in the GUI
//-----------------------------------------------------------------------------
bool mini_max::database::ArrayInfoContainer::AnyArrayInfoToUpdate() {
  std::lock_guard<std::mutex> lock(mutex);
  return (array_infos_to_be_updated.size() > 0);
}

// Desc: Get the next array info to update in the GUI
//-----------------------------------------------------------------------------
mini_max::database::ArrayInfoChange
mini_max::database::ArrayInfoContainer::GetArrayInfoForUpdate() {
  std::lock_guard<std::mutex> lock(mutex);

  // no array info to update
  if (array_infos_to_be_updated.size() == 0) {
    ArrayInfoChange aic;
    aic.array_info = nullptr;
    aic.item_index = 0xffffffff;
    return aic;
  }
  // get the array info
  ArrayInfoChange tmp = array_infos_to_be_updated.front();
  array_infos_to_be_updated.pop_front();
  return tmp;
}

// Desc: Get the index of the array in the vector
//-----------------------------------------------------------------------------
size_t mini_max::database::ArrayInfoContainer::GetVectorArrayIndex(
    unsigned int layer_number, ArrayInfoStruct::ArrayType type) {
  return layer_number *
             static_cast<unsigned int>(ArrayInfoStruct::ArrayType::size) +
         static_cast<unsigned int>(type);
}

}