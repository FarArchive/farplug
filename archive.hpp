#pragma once

#define INITGUID
#include "CPP/7zip/Archive/IArchive.h"

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

class ArcFormats: public list<ArcFormat> {
private:
  const ArcLibs& arc_libs;
  void load();
public:
  ArcFormats(const ArcLibs& arc_libs): arc_libs(arc_libs) {
    load();
  }
};

class FileStream: public IInStream, public UnknownImpl {
private:
  HANDLE h_file;
public:
  FileStream(const wstring& file_path);
  ~FileStream();

  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IInStream)
  UNKNOWN_IMPL_ITF(ISequentialInStream)
  UNKNOWN_IMPL_END

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition);
};

class ArchiveOpenCallback: public IArchiveOpenCallback, public UnknownImpl {
public:
  UNKNOWN_IMPL_BEGIN
  UNKNOWN_IMPL_ITF(IArchiveOpenCallback)
  UNKNOWN_IMPL_END

  STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
  STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);
};

class ArchiveReader {
private:
  const ArcFormats& arc_formats;
  ComObject<IInArchive> archive;
public:
  ArchiveReader(const ArcFormats& arc_formats): arc_formats(arc_formats) {
  }
  void open(const wstring& file_path);
};
