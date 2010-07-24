#pragma once

struct CompressFilesParams {
  unsigned __int64 min_file_size;
  unsigned max_compression_ratio; // 75% = comp_size / file_size
};

void plugin_compress_files(const ObjectArray<UnicodeString>& file_list, const CompressFilesParams& params, Log& log);
