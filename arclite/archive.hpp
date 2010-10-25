#pragma once

#include "comutils.hpp"

extern const string c_guid_7z;
extern const string c_guid_zip;
extern const string c_guid_iso;
extern const string c_guid_udf;

extern const wchar_t* c_method_copy;
extern const wchar_t* c_method_lzma;
extern const wchar_t* c_method_lzma2;
extern const wchar_t* c_method_ppmd;

extern const unsigned __int64 c_min_volume_size;

extern const wchar_t* c_sfx_ext;
extern const wchar_t* c_volume_ext;

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

  HRESULT get_prop(UInt32 index, PROPID prop_id, PROPVARIANT* prop) const;
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
  wstring name;
  DWORD attr;
  unsigned __int64 size;
  FILETIME ctime;
  FILETIME mtime;
  FILETIME atime;
  bool is_dir() const {
    return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  FindData convert() const;
  void convert(const FindData& file_info);
};

struct ArcFileInfo: public FileInfo {
  UInt32 parent;
  unsigned __int64 psize;
  bool operator<(const ArcFileInfo& file_info) const;
};
typedef vector<ArcFileInfo> FileList;

const UInt32 c_root_index = -1;

typedef vector<UInt32> FileIndex;
typedef pair<FileIndex::const_iterator, FileIndex::const_iterator> FileIndexRange;

struct ArcEntry {
  ArcType type;
  size_t sig_pos;
  ArcEntry(const ArcType& type, size_t sig_pos): type(type), sig_pos(sig_pos) {
  }
};

class ArcChain: public list<ArcEntry> {
public:
  wstring to_string() const;
};

// forwards
class SetDirAttrProgress;
class PrepareExtractProgress;

class Archive {
  // open
private:
  ComObject<IInArchive> in_arc;
  bool open_sub_stream(IInStream** sub_stream, FileInfo& sub_arc_info);
  bool open(IInStream* in_stream);
  static void detect(const wstring& file_path, bool all, vector<Archive>& archives);
public:
  static unsigned max_check_size;
  wstring arc_path;
  FileInfo arc_info;
  set<wstring> volume_names;
  ArcChain arc_chain;
  wstring arc_dir() const {
    return extract_file_path(arc_path);
  }
  static vector<Archive> detect(const wstring& file_path, bool all);
  void close();
  void reopen();
  bool is_open() const {
    return in_arc;
  }
  bool updatable() const {
    return arc_chain.size() == 1 && ArcAPI::formats().at(arc_chain.back().type).updatable;
  }
  bool is_pure_7z() const {
    return arc_chain.size() == 1 && arc_chain.back().type == c_guid_7z && arc_chain.back().sig_pos == 0;
  }

  // archive contents
public:
  UInt32 num_indices;
  FileList file_list;
  FileIndex file_list_index;
  void make_index();
  UInt32 find_dir(const wstring& dir);
  FileIndexRange get_dir_list(UInt32 dir_index);
  const ArcFileInfo& get_file_info(UInt32 file_index) const {
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
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const ExtractOptions& options, ErrorLog& error_log);
  void test(UInt32 src_dir_index, const vector<UInt32>& src_indices);
  void delete_archive();

  // create & update archive
private:
  wstring get_temp_file_name() const;
  void set_properties(IOutArchive* out_arc, const UpdateOptions& options);
  void load_sfx_module(Buffer<char>& buffer, const UpdateOptions& options);
public:
  unsigned level;
  wstring method;
  bool solid;
  bool encrypted;
  wstring password;
  bool update_props_defined;
  void load_update_props();
public:
  void create(const wstring& src_dir, const vector<wstring>& file_names, const UpdateOptions& options, ErrorLog& error_log);
  void update(const wstring& src_dir, const vector<wstring>& file_names, const wstring& dst_dir, const UpdateOptions& options, ErrorLog& error_log);

  // delete files in archive
private:
  void enum_deleted_indices(UInt32 file_index, vector<UInt32>& indices);
public:
  void delete_files(const vector<UInt32>& src_indices);

  // attributes
private:
  void load_arc_attr();
public:
  AttrList arc_attr;
  AttrList get_attr_list(UInt32 item_index);

public:
  Archive(): update_props_defined(false) {
  }
};

IOutStream* get_simple_update_stream(const wstring& arc_path);
