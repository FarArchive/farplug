#include <windows.h>

#include <string>
#include <list>
using namespace std;

#include "utils.hpp"
#include "sysutils.hpp"
#include "archive.hpp"

HRESULT ArcLib::get_bool_prop(UInt32 index, PROPID prop_id, bool& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BOOL)
    value = prop.boolVal == VARIANT_TRUE;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT ArcLib::get_string_prop(UInt32 index, PROPID prop_id, wstring& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BSTR)
    value = prop.bstrVal;
  else
    return E_FAIL;
  return S_OK;
}

HRESULT ArcLib::get_bytes_prop(UInt32 index, PROPID prop_id, string& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, &prop);
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BSTR) {
    UINT len = SysStringByteLen(prop.bstrVal);
    value.assign(reinterpret_cast<string::const_pointer>(prop.bstrVal), len);
  }
  else
    return E_FAIL;
  return S_OK;
}

void ArcLibs::load(const wstring& path) {
  WIN32_FIND_DATAW find_data;
  FindFile find_file(add_trailing_slash(path) + L"*.dll");
  while (find_file.next(find_data)) {
    ArcLib arc_lib;
    arc_lib.h_module = LoadLibraryW((add_trailing_slash(path) + find_data.cFileName).c_str());
    if (arc_lib.h_module) {
      arc_lib.CreateObject = reinterpret_cast<ArcLib::FCreateObject>(GetProcAddress(arc_lib.h_module, "CreateObject"));
      arc_lib.GetNumberOfMethods = reinterpret_cast<ArcLib::FGetNumberOfMethods>(GetProcAddress(arc_lib.h_module, "GetNumberOfMethods"));
      arc_lib.GetMethodProperty = reinterpret_cast<ArcLib::FGetMethodProperty>(GetProcAddress(arc_lib.h_module, "GetMethodProperty"));
      arc_lib.GetNumberOfFormats = reinterpret_cast<ArcLib::FGetNumberOfFormats>(GetProcAddress(arc_lib.h_module, "GetNumberOfFormats"));
      arc_lib.GetHandlerProperty = reinterpret_cast<ArcLib::FGetHandlerProperty>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty"));
      arc_lib.GetHandlerProperty2 = reinterpret_cast<ArcLib::FGetHandlerProperty2>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty2"));
      if (arc_lib.CreateObject && arc_lib.GetNumberOfFormats && arc_lib.GetHandlerProperty2) {
        push_back(arc_lib);
      }
      else {
        FreeLibrary(arc_lib.h_module);
      }
    }
  }
}

ArcLibs::~ArcLibs() {
  for (const_iterator arc_lib = begin(); arc_lib != end(); arc_lib++) {
    FreeLibrary(arc_lib->h_module);
  }
  clear();
}

void ArcFormats::load() {
  for (ArcLibs::const_iterator arc_lib = arc_libs.begin(); arc_lib != arc_libs.end(); arc_lib++) {
    UInt32 num_formats;
    if (arc_lib->GetNumberOfFormats(&num_formats) == S_OK) {
      for (UInt32 idx = 0; idx < num_formats; idx++) {
        ArcFormat arc_format;
        arc_format.arc_lib = &*arc_lib;
        CHECK_COM(arc_lib->get_string_prop(idx, NArchive::kName, arc_format.name));
        CHECK_COM(arc_lib->get_bytes_prop(idx, NArchive::kClassID, arc_format.class_id));
        CHECK_COM(arc_lib->get_bool_prop(idx, NArchive::kUpdate, arc_format.update));
        arc_lib->get_bytes_prop(idx, NArchive::kStartSignature, arc_format.start_signature);
        arc_lib->get_string_prop(idx, NArchive::kExtension, arc_format.extension);
        push_back(arc_format);
      }
    }
  }
}

FileStream::FileStream(const wstring& file_path) {
  h_file = CreateFileW(long_path(file_path).data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_POSIX_SEMANTICS, NULL);
  CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
}

FileStream::~FileStream() {
  CloseHandle(h_file);
}

STDMETHODIMP FileStream::Read(void *data, UInt32 size, UInt32 *processedSize) {
  COM_ERROR_HANDLER_BEGIN
  DWORD bytes_read;
  CHECK_SYS(ReadFile(h_file, data, size, &bytes_read, NULL));
  if (processedSize)
    *processedSize = bytes_read;
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP FileStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) {
  COM_ERROR_HANDLER_BEGIN
  DWORD move_method;
  switch (seekOrigin) {
  case STREAM_SEEK_SET:
    move_method = FILE_BEGIN;
    break;
  case STREAM_SEEK_CUR:
    move_method = FILE_CURRENT;
    break;
  case STREAM_SEEK_END:
    move_method = FILE_END;
    break;
  default:
    return E_INVALIDARG;
  }
  LARGE_INTEGER distance;
  distance.QuadPart = offset;
  LARGE_INTEGER new_position;
  CHECK_SYS(SetFilePointerEx(h_file, distance, &new_position, move_method));
  if (newPosition)
    *newPosition = new_position.QuadPart;
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::SetTotal(const UInt64 *files, const UInt64 *bytes) {
  COM_ERROR_HANDLER_BEGIN
  return S_OK;
  COM_ERROR_HANDLER_END
}

STDMETHODIMP ArchiveOpenCallback::SetCompleted(const UInt64 *files, const UInt64 *bytes) {
  COM_ERROR_HANDLER_BEGIN
  return S_OK;
  COM_ERROR_HANDLER_END
}

bool ArchiveReader::open(const wstring& file_path) {
  ComObject<FileStream> file_stream(new FileStream(file_path));
  ComObject<ArchiveOpenCallback> archive_open_callback(new ArchiveOpenCallback());
  for (ArcFormats::const_iterator arc_format = arc_formats.begin(); arc_format != arc_formats.end(); arc_format++) {
    ComObject<IInArchive> in_arc;
    CHECK_COM(arc_format->arc_lib->CreateObject(reinterpret_cast<const GUID*>(arc_format->class_id.data()), &IID_IInArchive, reinterpret_cast<void**>(&in_arc)));

    const UInt64 max_check_start_position = 1 << 20;
    file_stream->Seek(0, STREAM_SEEK_SET, NULL);
    if (check_com(in_arc->Open(file_stream, &max_check_start_position, archive_open_callback))) {
      archive = in_arc;
      return true;
    }
  }
  return false;
}
