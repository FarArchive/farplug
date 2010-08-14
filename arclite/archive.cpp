#include "msg.h"
#include "utils.hpp"
#include "sysutils.hpp"
#include "farutils.hpp"
#include "common_types.hpp"
#include "ui.hpp"
#include "archive.hpp"

HRESULT ArcLib::get_bool_prop(UInt32 index, PROPID prop_id, bool& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, prop.var());
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
  HRESULT res = GetHandlerProperty2(index, prop_id, prop.var());
  if (FAILED(res))
    return res;
  if (prop.vt == VT_BSTR) {
    UINT len = SysStringLen(prop.bstrVal);
    value.assign(prop.bstrVal, len);
  }
  else
    return E_FAIL;
  return S_OK;
}

HRESULT ArcLib::get_bytes_prop(UInt32 index, PROPID prop_id, string& value) const {
  PropVariant prop;
  HRESULT res = GetHandlerProperty2(index, prop_id, prop.var());
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


wstring ArcFormat::default_extension() const {
  wstring ext = L"." + extension_list;
  size_t pos = ext.find(L' ');
  if (pos != wstring::npos)
    ext.erase(pos);
  return ext;
}


ArcTypes ArcFormats::find_by_name(const wstring& name) const {
  ArcTypes types;
  for (const_iterator fmt = begin(); fmt != end(); fmt++) {
    if (fmt->second.name == name)
      types.push_back(fmt->first);
  }
  return types;
}

ArcTypes ArcFormats::find_by_ext(const wstring& ext) const {
  ArcTypes types;
  for (const_iterator fmt = begin(); fmt != end(); fmt++) {
    list<wstring> ext_list = split(fmt->second.extension_list, L' ');
    for (list<wstring>::const_iterator ext_iter = ext_list.begin(); ext_iter != ext_list.end(); ext_iter++) {
      // ext.c_str() + 1 == remove dot
      if (_wcsicmp(ext_iter->c_str(), ext.c_str() + 1) == 0) {
        types.push_back(fmt->first);
        break;
      }
    }
  }
  return types;
}


wstring ArcFormatChain::to_string() const {
  wstring result;
  for_each(begin(), end(), [&] (const ArcType& arc_type) {
    if (!result.empty())
      result += L"->";
    result += ArcAPI::formats().at(arc_type).name;
  });
  return result;
}


ArcAPI* ArcAPI::arc_api = nullptr;

ArcAPI::~ArcAPI() {
  for_each(arc_libs.begin(), arc_libs.end(), [&] (const ArcLib& arc_lib) {
    if (arc_lib.h_module)
      FreeLibrary(arc_lib.h_module);
  });
}

ArcAPI* ArcAPI::get() {
  if (arc_api == nullptr) {
    arc_api = new ArcAPI();
    arc_api->load();
  }
  return arc_api;
}

void ArcAPI::load_libs(const wstring& path) {
  FileEnum file_enum(path);
  while (file_enum.next()) {
    ArcLib arc_lib;
    arc_lib.module_path = add_trailing_slash(path) + file_enum.data().cFileName;
    arc_lib.h_module = LoadLibraryW(arc_lib.module_path.c_str());
    if (arc_lib.h_module == nullptr) continue;
    try {
      arc_lib.CreateObject = reinterpret_cast<ArcLib::FCreateObject>(GetProcAddress(arc_lib.h_module, "CreateObject"));
      CHECK_SYS(arc_lib.CreateObject);
      arc_lib.GetNumberOfMethods = reinterpret_cast<ArcLib::FGetNumberOfMethods>(GetProcAddress(arc_lib.h_module, "GetNumberOfMethods"));
      arc_lib.GetMethodProperty = reinterpret_cast<ArcLib::FGetMethodProperty>(GetProcAddress(arc_lib.h_module, "GetMethodProperty"));
      arc_lib.GetNumberOfFormats = reinterpret_cast<ArcLib::FGetNumberOfFormats>(GetProcAddress(arc_lib.h_module, "GetNumberOfFormats"));
      CHECK_SYS(arc_lib.GetNumberOfFormats);
      arc_lib.GetHandlerProperty = reinterpret_cast<ArcLib::FGetHandlerProperty>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty"));
      arc_lib.GetHandlerProperty2 = reinterpret_cast<ArcLib::FGetHandlerProperty2>(GetProcAddress(arc_lib.h_module, "GetHandlerProperty2"));
      CHECK_SYS(arc_lib.GetHandlerProperty2);
      arc_lib.version = get_module_version(arc_lib.module_path);
      arc_libs.push_back(arc_lib);
    }
    catch (...) {
      FreeLibrary(arc_lib.h_module);
    }
  }
}

void ArcAPI::load() {
  IGNORE_ERRORS(load_libs(Far::get_plugin_module_path()));
  IGNORE_ERRORS(load_libs(Key(HKEY_LOCAL_MACHINE, L"Software\\7-Zip", KEY_QUERY_VALUE).query_str(L"Path")));
  for (unsigned i = 0; i < arc_libs.size(); i++) {
    const ArcLib& arc_lib = arc_libs[i];
    UInt32 num_formats;
    if (arc_lib.GetNumberOfFormats(&num_formats) == S_OK) {
      for (UInt32 idx = 0; idx < num_formats; idx++) {
        ArcFormat arc_format;
        arc_format.lib_index = i;
        ArcType type;
        if (arc_lib.get_bytes_prop(idx, NArchive::kClassID, type) != S_OK) continue;
        arc_lib.get_string_prop(idx, NArchive::kName, arc_format.name);
        if (arc_lib.get_bool_prop(idx, NArchive::kUpdate, arc_format.updatable) != S_OK)
          arc_format.updatable = false;
        arc_lib.get_bytes_prop(idx, NArchive::kStartSignature, arc_format.start_signature);
        arc_lib.get_string_prop(idx, NArchive::kExtension, arc_format.extension_list);
        ArcFormats::const_iterator existing_format = arc_formats.find(type);
        if (existing_format == arc_formats.end() || arc_libs[existing_format->second.lib_index].version < arc_lib.version)
          arc_formats[type] = arc_format;
      }
    }
  }
  // unload unused libraries
  set<unsigned> used_libs;
  for_each(arc_formats.begin(), arc_formats.end(), [&] (const pair<ArcType, ArcFormat>& arc_format) {
    used_libs.insert(arc_format.second.lib_index);
  });
  for (unsigned i = 0; i < arc_libs.size(); i++) {
    if (used_libs.count(i) == 0) {
      FreeLibrary(arc_libs[i].h_module);
      arc_libs[i].h_module = nullptr;
    }
  }
}

void ArcAPI::create_in_archive(const ArcType& arc_type, IInArchive** in_arc) {
  CHECK_COM(libs()[formats().at(arc_type).lib_index].CreateObject(reinterpret_cast<const GUID*>(arc_type.data()), &IID_IInArchive, reinterpret_cast<void**>(in_arc)));
}

void ArcAPI::create_out_archive(const ArcType& arc_type, IOutArchive** out_arc) {
  CHECK_COM(libs()[formats().at(arc_type).lib_index].CreateObject(reinterpret_cast<const GUID*>(arc_type.data()), &IID_IOutArchive, reinterpret_cast<void**>(out_arc)));
}

void ArcAPI::free() {
  if (arc_api) {
    delete arc_api;
    arc_api = nullptr;
  }
}


wstring Archive::get_default_name() const {
  wstring name = archive_file_info.cFileName;
  size_t pos = name.find_last_of(L'.');
  if (pos == wstring::npos)
    return name;
  else
    return name.substr(0, pos);
}

wstring Archive::get_temp_file_name() const {
  GUID guid;
  CHECK_COM(CoCreateGuid(&guid));
  wchar_t guid_str[50];
  CHECK(StringFromGUID2(guid, guid_str, ARRAYSIZE(guid_str)));
  return add_trailing_slash(archive_dir) + guid_str + L".tmp";
}

bool FileInfo::operator<(const FileInfo& file_info) const {
  if (parent == file_info.parent)
    if (is_dir() == file_info.is_dir())
      return lstrcmpiW(name.c_str(), file_info.name.c_str()) < 0;
    else
      return is_dir();
  else
    return parent < file_info.parent;
}

void Archive::make_index() {
  class Progress: public ProgressMonitor {
  private:
    UInt32 completed;
    UInt32 total;
    virtual void do_update_ui() {
      wostringstream st;
      st << Far::get_msg(MSG_PLUGIN_NAME) << L'\n';
      st << completed << L" / " << total << L'\n';
      st << Far::get_progress_bar_str(60, completed, total) << L'\n';
      Far::message(st.str(), 0, FMSG_LEFTALIGN);
    }
  public:
    Progress(): completed(0), total(0) {
    }
    void update(UInt32 completed, UInt32 total) {
      this->completed = completed;
      this->total = total;
      update_ui();
    }
  };
  Progress progress;

  num_indices = 0;
  CHECK_COM(in_arc->GetNumberOfItems(&num_indices));
  file_list.clear();
  file_list.reserve(num_indices);

  struct DirInfo {
    UInt32 index;
    UInt32 parent;
    wstring name;
    bool operator<(const DirInfo& dir_info) const {
      if (parent == dir_info.parent)
        return name < dir_info.name;
      else
        return parent < dir_info.parent;
    }
  };
  typedef set<DirInfo> DirList;
  map<UInt32, unsigned> dir_index_map;
  DirList dir_list;

  DirInfo dir_info;
  UInt32 dir_index = 0;
  FileInfo file_info;
  wstring path;
  PropVariant prop;
  for (UInt32 i = 0; i < num_indices; i++) {
    progress.update(i, num_indices);

    if (s_ok(in_arc->GetProperty(i, kpidPath, prop.var())) && prop.vt == VT_BSTR)
      path.assign(prop.bstrVal);
    else
      path.assign(get_default_name());

    // attributes
    bool is_dir = s_ok(in_arc->GetProperty(i, kpidIsDir, prop.var())) && prop.vt == VT_BOOL && prop.boolVal;
    if (s_ok(in_arc->GetProperty(i, kpidAttrib, prop.var())) && prop.vt == VT_UI4)
      file_info.attr = prop.ulVal;
    else
      file_info.attr = 0;
    if (is_dir)
      file_info.attr |= FILE_ATTRIBUTE_DIRECTORY;
    else
      is_dir = file_info.is_dir();

    // size
    if (!is_dir && s_ok(in_arc->GetProperty(i, kpidSize, prop.var())) && prop.vt == VT_UI8)
      file_info.size = prop.uhVal.QuadPart;
    else
      file_info.size = 0;
    if (!is_dir && s_ok(in_arc->GetProperty(i, kpidPackSize, prop.var())) && prop.vt == VT_UI8)
      file_info.psize = prop.uhVal.QuadPart;
    else
      file_info.psize = 0;

    // date & time
    if (s_ok(in_arc->GetProperty(i, kpidCTime, prop.var())) && prop.vt == VT_FILETIME)
      file_info.ctime = prop.filetime;
    else
      file_info.ctime = archive_file_info.ftCreationTime;
    if (s_ok(in_arc->GetProperty(i, kpidMTime, prop.var())) && prop.vt == VT_FILETIME)
      file_info.mtime = prop.filetime;
    else
      file_info.mtime = archive_file_info.ftLastWriteTime;
    if (s_ok(in_arc->GetProperty(i, kpidATime, prop.var())) && prop.vt == VT_FILETIME)
      file_info.atime = prop.filetime;
    else
      file_info.atime = archive_file_info.ftLastAccessTime;

    // file name
    size_t name_end_pos = path.size();
    while (name_end_pos && is_slash(path[name_end_pos - 1])) name_end_pos--;
    size_t name_pos = name_end_pos;
    while (name_pos && !is_slash(path[name_pos - 1])) name_pos--;
    file_info.name.assign(path.data() + name_pos, name_end_pos - name_pos);

    // split path into individual directories and put them into DirList
    dir_info.parent = c_root_index;
    size_t begin_pos = 0;
    while (begin_pos < name_pos) {
      dir_info.index = dir_index;
      size_t end_pos = begin_pos;
      while (end_pos < name_pos && !is_slash(path[end_pos])) end_pos++;
      if (end_pos != begin_pos) {
        dir_info.name.assign(path.data() + begin_pos, end_pos - begin_pos);
        pair<DirList::iterator, bool> ins_pos = dir_list.insert(dir_info);
        if (ins_pos.second)
          dir_index++;
        dir_info.parent = ins_pos.first->index;
      }
      begin_pos = end_pos + 1;
    }
    file_info.parent = dir_info.parent;

    if (is_dir) {
      dir_info.index = dir_index;
      dir_info.parent = file_info.parent;
      dir_info.name = file_info.name;
      pair<DirList::iterator, bool> ins_pos = dir_list.insert(dir_info);
      if (ins_pos.second)
        dir_index++;
      dir_index_map[ins_pos.first->index] = i;
    }

    file_list.push_back(file_info);
  }

  // add directories that not present in archive index
  file_list.reserve(file_list.size() + dir_list.size() - dir_index_map.size());
  dir_index = num_indices;
  for_each(dir_list.begin(), dir_list.end(), [&] (const DirInfo& dir_info) {
    if (dir_index_map.count(dir_info.index) == 0) {
      dir_index_map[dir_info.index] = dir_index;
      file_info.parent = dir_info.parent;
      file_info.name = dir_info.name;
      file_info.attr = FILE_ATTRIBUTE_DIRECTORY;
      file_info.size = file_info.psize = 0;
      file_info.ctime = archive_file_info.ftCreationTime;
      file_info.mtime = archive_file_info.ftLastWriteTime;
      file_info.atime = archive_file_info.ftLastAccessTime;
      dir_index++;
      file_list.push_back(file_info);
    }
  });

  // fix parent references
  for_each(file_list.begin(), file_list.end(), [&] (FileInfo& file_info) {
    if (file_info.parent != c_root_index)
      file_info.parent = dir_index_map[file_info.parent];
  });

  // create search index
  file_list_index.clear();
  file_list_index.reserve(file_list.size());
  for (size_t i = 0; i < file_list.size(); i++) {
    file_list_index.push_back(i);
  }
  sort(file_list_index.begin(), file_list_index.end(), [&] (UInt32 left, UInt32 right) -> bool {
    return file_list[left] < file_list[right];
  });
}

UInt32 Archive::find_dir(const wstring& path) {
  if (file_list.empty())
    make_index();

  FileInfo dir_info;
  dir_info.attr = FILE_ATTRIBUTE_DIRECTORY;
  dir_info.parent = c_root_index;
  size_t begin_pos = 0;
  while (begin_pos < path.size()) {
    size_t end_pos = begin_pos;
    while (end_pos < path.size() && !is_slash(path[end_pos])) end_pos++;
    if (end_pos != begin_pos) {
      dir_info.name.assign(path.data() + begin_pos, end_pos - begin_pos);
      FileIndexRange fi_range = equal_range(file_list_index.begin(), file_list_index.end(), -1, [&] (UInt32 left, UInt32 right) -> bool {
        const FileInfo& fi_left = left == -1 ? dir_info : file_list[left];
        const FileInfo& fi_right = right == -1 ? dir_info : file_list[right];
        return fi_left < fi_right;
      });
      if (fi_range.first == fi_range.second)
        FAIL(ERROR_PATH_NOT_FOUND);
      dir_info.parent = *fi_range.first;
    }
    begin_pos = end_pos + 1;
  }
  return dir_info.parent;
}

FileIndexRange Archive::get_dir_list(UInt32 dir_index) {
  if (file_list.empty())
    make_index();

  FileInfo file_info;
  file_info.parent = dir_index;
  FileIndexRange index_range = equal_range(file_list_index.begin(), file_list_index.end(), -1, [&] (UInt32 left, UInt32 right) -> bool {
    const FileInfo& fi_left = left == -1 ? file_info : file_list[left];
    const FileInfo& fi_right = right == -1 ? file_info : file_list[right];
    return fi_left.parent < fi_right.parent;
  });

  return index_range;
}
