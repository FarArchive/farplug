#pragma once

#include "NonCopyable.hpp"

class FindFile: private NonCopyable {
private:
  HANDLE h_find;
  wstring find_mask;
public:
  FindFile(const wstring& find_mask);
  ~FindFile();
  bool next(WIN32_FIND_DATAW& find_data);
};

wstring get_system_message(HRESULT hr);
