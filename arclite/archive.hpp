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
typedef vector<ArcFormat> ArcFormatChain;

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
  const ArcFormat& find_format(const wstring& name) const;
  void create_in_archive(const ArcFormat& format, IInArchive** in_arc);
  void create_out_archive(const ArcFormat& format, IOutArchive** out_arc);
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

struct FileIndexInfo {
  wstring rel_path;
  FindData find_data;
};
typedef map<UInt32, FileIndexInfo> FileIndexMap;

class Archive {
private:
  wstring password;
  wstring get_default_name() const;
public:
  bool updatable() const {
    return format_chain.size() == 1 && format_chain.back().update;
  }
  wstring get_temp_file_name() const;
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log);
  friend class ArchiveExtractor;

  // open & create
private:
  wstring archive_dir;
  FindData archive_file_info;
  wstring get_archive_path() const {
    return add_trailing_slash(archive_dir) + archive_file_info.cFileName;
  }
private:
  ComObject<IInArchive> in_arc;
  ArcFormatChain format_chain;
  bool open_sub_stream(IInArchive* in_arc, IInStream** sub_stream);
  bool open_archive(IInStream* in_stream, IInArchive* archive);
  void detect(IInStream* in_stream, vector<ArcFormatChain>& format_chains);
public:
  vector<ArcFormatChain> detect(const wstring& file_path);
  bool open(const wstring& file_path, const ArcFormatChain& format_chain);
  void close();
  void reopen();
  bool is_open() const {
    return in_arc;
  }

  // archive contents
private:
  UInt32 num_indices;
  FileList file_list;
  FileIndex file_list_index;
  void make_index();
public:
  UInt32 find_dir(const wstring& dir);
  FileIndexRange get_dir_list(UInt32 dir_index);
  const FileInfo& get_file_info(UInt32 file_index) const {
    return file_list[file_index];
  }

  // create & update archive
private:
  UInt32 scan_file(const wstring& sub_dir, const FindData& src_find_data, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map);
  void scan_dir(const wstring& src_dir, const wstring& sub_dir, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map);
  void prepare_file_index_map(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map);
  void set_properties(IOutArchive* out_arc, const UpdateOptions& options);
public:
  void create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options);
  void update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options);

  // delete files in archive
private:
  void enum_deleted_indices(UInt32 file_index, vector<UInt32>& indices);
public:
  void delete_files(const vector<UInt32>& src_indices);
};
