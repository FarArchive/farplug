#pragma once

#include "comutils.hpp"

struct ArcLib {
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

  HRESULT get_bool_prop(UInt32 index, PROPID prop_id, bool& value) const;
  HRESULT get_string_prop(UInt32 index, PROPID prop_id, wstring& value) const;
  HRESULT get_bytes_prop(UInt32 index, PROPID prop_id, string& value) const;
};

struct ArcFormat {
  unsigned lib_index;
  wstring name;
  string class_id;
  bool update;
  string start_signature;
  wstring extension;
};

typedef vector<ArcLib> ArcLibs;
typedef vector<ArcFormat> ArcFormats;

class ArcAPI {
private:
  ArcLibs arc_libs;
  ArcFormats arc_formats;
  static ArcAPI* arc_api;
  ArcAPI() {}
  ~ArcAPI();
  void load();
public:
  static ArcAPI* get();
  const ArcLibs& libs() const {
    return arc_libs;
  }
  const ArcFormats& formats() const {
    return arc_formats;
  }
  const ArcFormat* find_format(const wstring& name) const;
  static void free();
};

struct FileInfo {
  UInt32 parent;
  wstring name;
  DWORD attr;
  unsigned __int64 size;
  unsigned __int64 psize;
  FILETIME ctime;
  FILETIME mtime;
  FILETIME atime;
  bool is_dir() const {
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  bool operator<(const FileInfo& file_info) const;
};
typedef vector<FileInfo> FileList;
const UInt32 c_root_index = -1;
typedef vector<UInt32> FileIndex;
typedef pair<FileIndex::const_iterator, FileIndex::const_iterator> FileIndexRange;

class Archive {
protected:
  ComObject<IInArchive> in_arc;
  vector<ArcFormat> formats;
  wstring archive_dir;
  FindData archive_file_info;
  UInt32 num_indices;
  FileList file_list;
  FileIndex file_list_index;
  wstring password;
  wstring get_default_name() const;
  void make_index();
public:
  bool open(const wstring& file_path);
  void close();
  void reopen();
  const FileInfo& get_file_info(UInt32 file_index) const {
    return file_list[file_index];
  }
  bool updatable() const {
    return formats.size() == 1 && formats.back().update;
  }
  wstring get_file_name() const {
    return add_trailing_slash(archive_dir) + archive_file_info.cFileName;
  }
  wstring get_temp_file_name() const;
  UInt32 find_dir(const wstring& dir);
  FileIndexRange get_dir_list(UInt32 dir_index);
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log);
  void delete_files(const vector<UInt32>& src_indices);
  void create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options);
  void update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options);
  friend class ArchiveOpener;
  friend class ArchiveExtractor;
  friend class ArchiveFileDeleter;
  friend class ArchiveUpdater;
};
