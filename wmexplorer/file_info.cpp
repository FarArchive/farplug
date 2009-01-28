#include <windows.h>

#include "rapi2.h"

#include "plugin.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#include "util.h"
#include "file_info.h"

FileInfo::FileInfo(const WIN32_FIND_DATAW& find_data) {
  file_name = find_data.cFileName;
  attr = find_data.dwFileAttributes;
  creation_time = find_data.ftCreationTime;
  access_time = find_data.ftLastAccessTime;
  write_time = find_data.ftLastWriteTime;
  size = FILE_SIZE(find_data);
  child_cnt = 0;
}

FileInfo::FileInfo(const CE_FIND_DATA& find_data) {
  file_name = find_data.cFileName;
  attr = find_data.dwFileAttributes;
  creation_time = find_data.ftCreationTime;
  access_time = find_data.ftLastAccessTime;
  write_time = find_data.ftLastWriteTime;
  size = FILE_SIZE(find_data);
  child_cnt = 0;
}

FileInfo::FileInfo(const FAR_FIND_DATA& find_data) {
  file_name = FAR_DECODE_PATH(FAR_FILE_NAME(find_data));
  attr = find_data.dwFileAttributes;
  creation_time = find_data.ftCreationTime;
  access_time = find_data.ftLastAccessTime;
  write_time = find_data.ftLastWriteTime;
  size = FAR_FILE_SIZE(find_data);
  child_cnt = 0;
}

bool FileInfo::is_dir() const {
  return (attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
}
