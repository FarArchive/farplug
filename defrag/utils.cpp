#include <windows.h>

#include "col/UnicodeString.h"
using namespace col;

#include "utils.h"

// extract path root component (drive letter / volume name / UNC share)
UnicodeString extract_path_root(const UnicodeString& path) {
  unsigned prefix_len = 0;
  bool is_unc_path = false;
  if (path.equal(0, L"\\\\?\\UNC\\")) {
    prefix_len = 8;
    is_unc_path = true;
  }
  else if (path.equal(0, L"\\\\?\\") || path.equal(0, L"\\??\\") || path.equal(0, L"\\\\.\\")) {
    prefix_len = 4;
  }
  else if (path.equal(0, L"\\\\")) {
    prefix_len = 2;
    is_unc_path = true;
  }
  if ((prefix_len == 0) && !path.equal(1, L':')) return UnicodeString();
  unsigned p = path.search(prefix_len, L'\\');
  if (p == -1) p = path.size();
  if (is_unc_path) {
    p = path.search(p + 1, L'\\');
    if (p == -1) p = path.size();
  }
  return path.left(p);
}

UnicodeString extract_file_name(const UnicodeString& path) {
  unsigned pos = path.rsearch('\\');
  if (pos == -1) pos = 0;
  else pos++;
  if (pos < extract_path_root(path).size()) return UnicodeString();
  return path.slice(pos);
}

UnicodeString extract_file_dir(const UnicodeString& path) {
  unsigned pos = path.rsearch('\\');
  if (pos == -1) pos = 0;
  if (pos < extract_path_root(path).size()) return UnicodeString();
  return path.left(pos);
}

UnicodeString long_path(const UnicodeString& path) {
  if (path.size() < MAX_PATH) return path;
  if (path.equal(0, L"\\\\?\\") || path.equal(0, L"\\\\.\\")) return path;
  if (path.equal(0, L"\\??\\")) return UnicodeString(path).replace(0, 4, L"\\\\?\\");
  if (path.equal(0, L"\\\\")) return UnicodeString(path).replace(0, 1, L"\\\\?\\UNC");
  return L"\\\\?\\" + path;
}

UnicodeString add_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() == 0) || (file_path.last() == L'\\')) return file_path;
  else return file_path + L'\\';
}

UnicodeString del_trailing_slash(const UnicodeString& file_path) {
  if ((file_path.size() < 2) || (file_path.last() != L'\\')) return file_path;
  else return file_path.left(file_path.size() - 1);
}
