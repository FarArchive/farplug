#include "utils.hpp"
#include "error.hpp"
#include "common.hpp"

Error g_com_error;

unsigned calc_percent(unsigned __int64 completed, unsigned __int64 total) {
  unsigned percent;
  if (total == 0)
    percent = 0;
  else
    percent = round(static_cast<double>(completed) / total * 100);
  if (percent > 100)
    percent = 100;
  return percent;
}

unsigned __int64 get_module_version(const wstring& file_path) {
  unsigned __int64 version = 0;
  DWORD dw_handle;
  DWORD ver_size = GetFileVersionInfoSizeW(file_path.c_str(), &dw_handle);
  if (ver_size) {
    Buffer<unsigned char> ver_block(ver_size);
    if (GetFileVersionInfoW(file_path.c_str(), dw_handle, ver_size, ver_block.data())) {
      VS_FIXEDFILEINFO* fixed_file_info;
      UINT len;
      if (VerQueryValueW(ver_block.data(), L"\\", reinterpret_cast<LPVOID*>(&fixed_file_info), &len)) {
        return (static_cast<unsigned __int64>(fixed_file_info->dwFileVersionMS) << 32) + fixed_file_info->dwFileVersionLS;
      }
    }
  }
  return version;
}

unsigned __int64 parse_size_string(const wstring& str) {
  unsigned __int64 size = 0;
  unsigned i = 0;
  while (i < str.size() && str[i] >= L'0' && str[i] <= L'9') {
    size = size * 10 + (str[i] - L'0');
    i++;
  }
  while (i < str.size() && str[i] == L' ') i++;
  if (i < str.size()) {
    wchar_t mod_ch = str[i];
    unsigned __int64 mod_mul;
    if (mod_ch == L'K' || mod_ch == L'k') mod_mul = 1024;
    else if (mod_ch == L'M' || mod_ch == L'm') mod_mul = 1024 * 1024;
    else if (mod_ch == L'G' || mod_ch == L'g') mod_mul = 1024 * 1024 * 1024;
    else mod_mul = 1;
    size *= mod_mul;
  }
  return size;
}
