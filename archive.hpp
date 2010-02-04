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

class ArchiveExtractCallback;

class ArchiveReader {
private:
  const ArcFormats& arc_formats;
  ComObject<IInArchive> archive;
  wstring archive_dir;
  FindData archive_file_info;
  FileList file_list;
  FileIndex dir_find_index;
  FileIndex file_id_index;
  wstring password;
  wstring get_default_name() const;
  void make_index();
  void detect(IInStream* in_stream, IArchiveOpenCallback* callback, vector<ComObject<IInArchive>>& archives, vector<wstring>& format_names);
  void prepare_extract(UInt32 dir_index, const wstring& parent_dir, list<UInt32>& indices, ArchiveExtractCallback* progress);
  void set_dir_attr(FileInfo dir_info, const wstring& dir_path, ArchiveExtractCallback* progress);
public:
  ArchiveReader(const ArcFormats& arc_formats, const wstring& file_path);
  bool open();
  UInt32 dir_find(const wstring& dir);
  FileListRef dir_list(UInt32 dir_index);
  void extract(UInt32 src_dir_index, const vector<UInt32>& src_indices, const wstring& dest_dir);
  friend class ArchiveOpenCallback;
  friend class ArchiveExtractCallback;
};

class ArchiveOpenCallback: public IArchiveOpenCallback, public IArchiveOpenVolumeCallback, public ICryptoGetTextPassword, public UnknownImpl, public ProgressMonitor {
private:
  ArchiveReader& reader;
  UInt64 total_files;
  UInt64 total_bytes;
  UInt64 completed_files;
  UInt64 completed_bytes;
  FindData volume_file_info;
  virtual void do_update_ui();
public:
  ArchiveOpenCallback(ArchiveReader& reader): reader(reader), volume_file_info(reader.archive_file_info), total_files(0), total_bytes(0), completed_files(0), completed_bytes(0) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IArchiveOpenCallback)
  UNKNOWN_IMPL_ITF(IArchiveOpenVolumeCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
  UNKNOWN_IMPL_END

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

  STDMETHOD(GetProperty)(PROPID propID, PROPVARIANT *value);
  STDMETHOD(GetStream)(const wchar_t *name, IInStream **inStream);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
};

enum OverwriteOption { ooAsk, ooOverwrite, ooSkip };

class ArchiveExtractCallback: public IArchiveExtractCallback, public ICryptoGetTextPassword, public UnknownImpl, public ProgressMonitor {
private:
  ArchiveReader& reader;
  UInt32 src_dir_index;
  wstring dest_dir;
  UInt64 total;
  UInt64 completed;
  wstring file_path;
  OverwriteOption oo;
  virtual void do_update_ui();
public:
  ArchiveExtractCallback(ArchiveReader& reader, UInt32 src_dir_index, const wstring& dest_dir): reader(reader), src_dir_index(src_dir_index), dest_dir(dest_dir), total(0), completed(0), oo(ooAsk) {
  }

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IProgress)
  UNKNOWN_IMPL_ITF(IArchiveExtractCallback)
  UNKNOWN_IMPL_ITF(ICryptoGetTextPassword)
  UNKNOWN_IMPL_END

  STDMETHOD(SetTotal)(UInt64 total);
  STDMETHOD(SetCompleted)(const UInt64 *completeValue);

  STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream,  Int32 askExtractMode);
  STDMETHOD(PrepareOperation)(Int32 askExtractMode);
  STDMETHOD(SetOperationResult)(Int32 resultEOperationResult);

  STDMETHOD(CryptoGetTextPassword)(BSTR *password);
};
