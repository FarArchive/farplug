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

struct FileList: public map<wstring, FileList> {
  UInt32 index;
  FileList(): index(-1) {}
};

class ArchiveReader {
private:
  ComObject<IInArchive> archive;
  WIN32_FIND_DATAW archive_file_info;
  FileList root;
  wstring get_default_name() const;
  void make_index();
public:
  bool open(const ArcFormats& arc_formats, const wstring& file_path);
  FileList* find_dir(const wstring& dir);
  void get_file_info(const UInt32 file_index, const wstring& file_name, PluginPanelItem& panel_item);
};
