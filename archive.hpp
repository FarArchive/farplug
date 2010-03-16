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

struct ArcLibs: public list<ArcLib> {
  ~ArcLibs();
  void load(const wstring& path);
};

struct ArcFormat {
  const ArcLib* arc_lib;
  wstring name;
  string class_id;
  bool update;
  string start_signature;
  wstring extension;
};

struct ArcFormats: public list<ArcFormat> {
  void load(const ArcLibs& arc_libs);
};

struct FileInfo {
  UInt32 index;
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
};
typedef vector<FileInfo> FileList;
const UInt32 c_root_index = -1;
typedef vector<UInt32> FileIndex;
typedef pair<FileList::const_iterator, FileList::const_iterator> FileListRef;

class Archive {
private:
  const ArcFormats& arc_formats;
  ComObject<IInArchive> in_arc;
  wstring archive_dir;
  FindData archive_file_info;
  FileList file_list;
  FileIndex dir_find_index;
  FileIndex file_id_index;
  wstring password;
  wstring get_default_name() const;
  void make_index();
  void prepare_extract(UInt32 dir_index, const wstring& parent_dir, list<UInt32>& indices);
  void set_dir_attr(FileInfo dir_info, const wstring& dir_path);
public:
  Archive(const ArcFormats& arc_formats, const wstring& file_path);
  bool open();
  UInt32 dir_find(const wstring& dir);
  FileListRef dir_list(UInt32 dir_index);
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const wstring& dest_dir);
  friend class ArchiveOpener;
  friend class ArchiveExtractor;
};
