#pragma once

#include "comutils.hpp"

struct ArcLib {
  HMODULE h_module;
  unsigned __int64 version;
  wstring module_path;
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
  bool updatable;
  string start_signature;
  wstring extension_list;
  wstring default_extension() const;
};

typedef vector<ArcLib> ArcLibs;

typedef string ArcType;
typedef list<ArcType> ArcTypes;

class ArcFormats: public map<ArcType, ArcFormat> {
public:
  ArcTypes find_by_name(const wstring& name) const;
  ArcTypes find_by_ext(const wstring& ext) const;
};

class ArcFormatChain: public list<ArcType> {
public:
  wstring to_string() const;
};

struct SfxModule {
  wstring path;
};
typedef vector<SfxModule> SfxModules;

class ArcAPI {
private:
  ArcLibs arc_libs;
  ArcFormats arc_formats;
  SfxModules sfx_modules;
  static ArcAPI* arc_api;
  ArcAPI() {}
  ~ArcAPI();
  void load_libs(const wstring& path);
  void find_sfx_modules(const wstring& path);
  void load();
  static ArcAPI* get();
public:
  static const ArcLibs& libs() {
    return get()->arc_libs;
  }
  static const ArcFormats& formats() {
    return get()->arc_formats;
  }
  static const SfxModules& sfx() {
    return get()->sfx_modules;
  }
  static void create_in_archive(const ArcType& arc_type, IInArchive** in_arc);
  static void create_out_archive(const ArcType& format, IOutArchive** out_arc);
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

struct ArchiveHandle {
  ArcFormatChain format_chain;
  ComObject<IInArchive> in_arc;
};

typedef vector<ArchiveHandle> ArchiveHandles;

// forwards
class SetDirAttrProgress;
class PrepareExtractProgress;
class DeleteFilesProgress;
class PrepareUpdateProgress;

class Archive {
  // open
private:
  wstring archive_dir;
  wstring get_archive_path() const {
    return add_trailing_slash(archive_dir) + archive_file_info.cFileName;
  }
  ComObject<IInArchive> in_arc;
  bool open_sub_stream(IInArchive* in_arc, IInStream** sub_stream, wstring& sub_ext);
  bool open_archive(IInStream* in_stream, IInArchive* archive);
  void detect(IInStream* in_stream, const wstring& ext, bool all, ArchiveHandles& format_chains);
protected:
  FindData archive_file_info;
  unsigned max_check_size;
  ArcFormatChain format_chain;
public:
  ArchiveHandles detect(const wstring& file_path, bool all);
  void open(const ArchiveHandle& archive_handle);
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
protected:
  wstring password;
public:
  UInt32 find_dir(const wstring& dir);
  FileIndexRange get_dir_list(UInt32 dir_index);
  const FileInfo& get_file_info(UInt32 file_index) const {
    return file_list[file_index];
  }

  // extract
private:
  wstring get_default_name() const;
  void prepare_dst_dir(const wstring& path);
  void prepare_extract(UInt32 file_index, const wstring& parent_dir, list<UInt32>& indices, bool& ignore_errors, ErrorLog& error_log, PrepareExtractProgress& progress);
  void set_dir_attr(const FileIndexRange& index_list, const wstring& parent_dir, bool& ignore_errors, ErrorLog& error_log, SetDirAttrProgress& progress);
  void prepare_test(UInt32 file_index, list<UInt32>& indices);
public:
  bool updatable() const {
    return format_chain.size() == 1 && ArcAPI::formats().at(format_chain.back()).updatable;
  }
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log);
  void test(UInt32 src_dir_index, const vector<UInt32>& src_indices);

  // create & update archive
private:
  wstring get_temp_file_name() const;
  UInt32 scan_file(const wstring& sub_dir, const FindData& src_find_data, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map, PrepareUpdateProgress& progress);
  void scan_dir(const wstring& src_dir, const wstring& sub_dir, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map, PrepareUpdateProgress& progress);
  void prepare_file_index_map(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, UInt32 dst_dir_index, UInt32& new_index, FileIndexMap& file_index_map);
  void set_properties(IOutArchive* out_arc, const UpdateOptions& options);
  void delete_src_file(const wstring& file_path, DeleteFilesProgress& progress);
  void delete_src_dir(const wstring& dir_path, DeleteFilesProgress& progress);
  void delete_src_files(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number);
  void load_sfx_module(Buffer<char>& buffer, const UpdateOptions& options);
public:
  void create(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const UpdateOptions& options, ErrorLog& error_log);
  void update(const wstring& src_dir, const PluginPanelItem* panel_items, unsigned items_number, const wstring& dst_dir, const UpdateOptions& options, ErrorLog& error_log);

  // delete files in archive
private:
  void enum_deleted_indices(UInt32 file_index, vector<UInt32>& indices);
public:
  void delete_files(const vector<UInt32>& src_indices);

  // attributes
private:
  void load_arc_attr();
protected:
  AttrList arc_attr;
  bool encrypted;
public:
  AttrList get_attr_list(UInt32 item_index);
};

class ArchiveUpdateStream: public IOutStream, public ComBase {
private:
  HANDLE h_file;
  wstring file_path;
  __int64 start_offset;

public:
  ArchiveUpdateStream(const wstring& file_path, Error& error);
  ~ArchiveUpdateStream();
  void set_offset(__int64 offset);

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(ISequentialOutStream)
  UNKNOWN_IMPL_ITF(IOutStream)
  UNKNOWN_IMPL_END

  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
  STDMETHOD(SetSize)(Int64 newSize);
};

const string c_guid_7z("\x69\x0F\x17\x23\xC1\x40\x8A\x27\x10\x00\x00\x01\x10\x07\x00\x00", 16);
const string c_guid_zip("\x69\x0F\x17\x23\xC1\x40\x8A\x27\x10\x00\x00\x01\x10\x01\x00\x00", 16);
const string c_guid_iso("\x69\x0F\x17\x23\xC1\x40\x8A\x27\x10\x00\x00\x01\x10\xE7\x00\x00", 16);
const string c_guid_udf("\x69\x0F\x17\x23\xC1\x40\x8A\x27\x10\x00\x00\x01\x10\xE0\x00\x00", 16);
