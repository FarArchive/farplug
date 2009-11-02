#include <string>
#include <list>
using namespace std;

#include "CPP/7zip/Archive/IArchive.h"

#include "utils.hpp"
#include "archive.hpp"

struct ArcLibInfo {
  HMODULE h_module;
  typedef UInt32 (WINAPI *FCreateObject)(const GUID *clsID, const GUID *interfaceID, void **outObject);
  typedef UInt32 (WINAPI *FGetNumberOfMethods)(UInt32 *numMethods);
  typedef UInt32 (WINAPI *FGetMethodProperty)(UInt32 index, PROPID propID, PROPVARIANT *value);
  typedef UInt32 (WINAPI *FGetNumberOfFormats)(UInt32 *numFormats);
  typedef UInt32 (WINAPI *FGetHandlerProperty)(PROPID propID, PROPVARIANT *value);
  typedef UInt32 (WINAPI *FGetHandlerProperty2)(UInt32 index, PROPID propID, PROPVARIANT *value);
  typedef UInt32 (WINAPI *FSetLargePageMode)();
  FCreateObject CreateObject;
  FGetNumberOfMethods GetNumberOfMethods;
  FGetMethodProperty GetMethodProperty;
  FGetNumberOfFormats GetNumberOfFormats;
  FGetHandlerProperty GetHandlerProperty;
  FGetHandlerProperty2 GetHandlerProperty2;
};

list<ArcLibInfo> load_arc_libs(const wstring& path) {
  list<ArcLibInfo> lib_list;
  WIN32_FIND_DATAW find_data;
  HANDLE h_find = FindFirstFileW((add_trailing_slash(path) + L"*.dll").c_str(), &find_data);
  CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
  CLEAN(HANDLE, h_find, FindClose(h_find));
  while (true) {
    ArcLibInfo lib_info;
    lib_info.h_module = LoadLibraryW((add_trailing_slash(path) + find_data.cFileName).c_str());
    if (lib_info.h_module) {
      lib_info.CreateObject = reinterpret_cast<ArcLibInfo::FCreateObject>(GetProcAddress(lib_info.h_module, "CreateObject"));
      lib_info.GetNumberOfMethods = reinterpret_cast<ArcLibInfo::FGetNumberOfMethods>(GetProcAddress(lib_info.h_module, "GetNumberOfMethods"));
      lib_info.GetMethodProperty = reinterpret_cast<ArcLibInfo::FGetMethodProperty>(GetProcAddress(lib_info.h_module, "GetMethodProperty"));
      lib_info.GetNumberOfFormats = reinterpret_cast<ArcLibInfo::FGetNumberOfFormats>(GetProcAddress(lib_info.h_module, "GetNumberOfFormats"));
      lib_info.GetHandlerProperty = reinterpret_cast<ArcLibInfo::FGetHandlerProperty>(GetProcAddress(lib_info.h_module, "GetHandlerProperty"));
      lib_info.GetHandlerProperty2 = reinterpret_cast<ArcLibInfo::FGetHandlerProperty2>(GetProcAddress(lib_info.h_module, "GetHandlerProperty2"));
      if (lib_info.CreateObject && lib_info.GetNumberOfFormats && lib_info.GetHandlerProperty2) {
        lib_list.push_back(lib_info);
      }
      else {
        FreeLibrary(lib_info.h_module);
      }
    }
    if (FindNextFileW(h_find, &find_data) == 0) {
      CHECK_SYS(GetLastError() == ERROR_NO_MORE_FILES);
      break;
    }
  }
  return lib_list;
}
