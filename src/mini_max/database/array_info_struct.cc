#include "muehle/mini_max/database/array_info_struct.h"

#ifdef _WIN32
#include <windows.h>  // QueryPerformanceCounter, QueryPerformanceFrequency
#else
#include "muehle/win_32_compat.h"
#endif

namespace muehle {

// Desc: Get the name of the array type
//-----------------------------------------------------------------------------
const std::wstring mini_max::database::ArrayInfoStruct::GetArrTypeName() {
  switch (type) {
    case mini_max::database::ArrayInfoStruct::ArrayType::count_array:
      return L"countArray";
    case mini_max::database::ArrayInfoStruct::ArrayType::knot_already_calculated:
      return L"knotAlreadyCalculated";
    case mini_max::database::ArrayInfoStruct::ArrayType::layer_stats:
      return L"layerStats";
    case mini_max::database::ArrayInfoStruct::ArrayType::ply_infos:
      return L"plyInfos";
    case mini_max::database::ArrayInfoStruct::ArrayType::size:
      return L"size";
    default:
      return L"invalid";
  }
}

}