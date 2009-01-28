#ifndef _CONTENT_H
#define _CONTENT_H

// content analysis results
struct ContentInfo {
  u64 file_size;
  u64 time; // ms
  u64 comp_size;
  Array<u8> crc32;
  Array<u8> md5;
  Array<u8> sha1;
  Array<u8> ed2k;
};

struct CompressionStats {
  u64 data_size; // total file data size
  u64 comp_size; // total compressed data size
  u64 time; // total processing time in ms
  unsigned file_cnt; // number of files processed
  unsigned dir_cnt; // number of dirs processed
  unsigned reparse_cnt; // number of reparse points skipped
  unsigned err_cnt; // number of files/dirs skipped because of errors
};

bool show_options_dialog(ContentOptions& options, bool single_file);
void process_file_content(const UnicodeString& file_name, const ContentOptions& options, ContentInfo& result);
void show_result_dialog(const ContentOptions& options, const ContentInfo& info);
void compress_files(const ObjectArray<UnicodeString>& file_list, CompressionStats& result);
void show_result_dialog(const CompressionStats& stats);

#endif /* _CONTENT_H */
