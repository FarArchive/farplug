#include "utils.hpp"
#include "sysutils.hpp"

class RsrcId {
private:
  wstring name;
  WORD id;
  void set(LPCTSTR name_id) {
    if (IS_INTRESOURCE(name_id)) {
      id = reinterpret_cast<WORD>(name_id);
      name.clear();
    }
    else {
      id = 0;
      name = name_id;
    }
  }
public:
  RsrcId() {
    set(0);
  }
  RsrcId(LPCTSTR name_id) {
    set(name_id);
  }
  void operator=(LPCTSTR name_id) {
    set(name_id);
  }
  operator LPCTSTR() const {
    return name.empty() ? MAKEINTRESOURCE(id) : name.c_str();
  }
  bool is_int() const {
    return name.empty();
  }
};

struct IconImage {
  BYTE width;
  BYTE height;
  BYTE color_cnt;
  WORD plane_cnt;
  WORD bit_cnt;
  ByteVector bitmap;
};

typedef list<IconImage> IconFile;

#pragma pack(push)
#pragma pack(1)
struct IconFileHeader {
  WORD reserved; // Reserved (must be 0)
  WORD type;     // Resource Type (1 for icons)
  WORD count;    // How many images?
};

struct IconFileEntry {
  BYTE width;         // Width, in pixels, of the image
  BYTE height;        // Height, in pixels, of the image
  BYTE color_count;   // Number of colors in image (0 if >=8bpp)
  BYTE reserved;      // Reserved ( must be 0)
  WORD planes;        // Color Planes
  WORD bit_count;     // Bits per pixel
  DWORD bytes_in_res; // How many bytes in this resource?
  DWORD image_offset; // Where in the file is this image?
};
#pragma pack(pop)

IconFile load_icon_file(const wstring& file_path) {
  IconFile icon;
  File file(file_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  IconFileHeader header;
  unsigned __int64 file_size = file.size();
  file.read(&header, sizeof(header));
  for (unsigned i = 0; i < header.count; i++) {
    IconImage image;
    IconFileEntry entry;
    file.read(&entry, sizeof(entry));
    image.width = entry.width;
    image.height = entry.height;
    image.color_cnt = entry.color_count;
    image.plane_cnt = entry.planes;
    image.bit_cnt = entry.bit_count;
    CHECK(entry.image_offset + entry.bytes_in_res <= file_size);
    unsigned __int64 cur_pos = file.set_pos(0, FILE_CURRENT);
    file.set_pos(entry.image_offset);
    Buffer<unsigned char> buf(entry.bytes_in_res);
    file.read(buf.data(), buf.size());
    image.bitmap.assign(buf.data(), buf.data() + buf.size());
    file.set_pos(cur_pos);
    icon.push_back(image);
  }
  return icon;
}


struct IconImageRsrc {
  WORD id;
  WORD lang_id;
  IconImage image;
};

struct IconRsrc {
  RsrcId id;
  WORD lang_id;
  list<IconImageRsrc> images;
};

#pragma pack(push)
#pragma pack(2)
struct IconGroupHeader {
  WORD reserved; // Reserved (must be 0)
  WORD type;     // Resource type (1 for icons)
  WORD count;    // How many images?
};

struct IconGroupEntry {
  BYTE width;     // Width, in pixels, of the image
  BYTE height;    // Height, in pixels, of the image
  BYTE color_cnt; // Number of colors in image (0 if >=8bpp)
  BYTE reserved;  // Reserved
  WORD plane_cnt; // Color Planes
  WORD bit_cnt;   // Bits per pixel
  DWORD size;     // how many bytes in this resource?
  WORD id;        // the ID
};
#pragma pack(pop)

IconRsrc load_icon_rsrc(HMODULE h_module, LPCTSTR name, WORD lang_id) {
  IconRsrc icon_rsrc;
  icon_rsrc.id = name;
  icon_rsrc.lang_id = lang_id;
  HRSRC h_rsrc = FindResourceEx(h_module, RT_GROUP_ICON, name, lang_id);
  CHECK_SYS(h_rsrc);
  HGLOBAL h_global = LoadResource(h_module, h_rsrc);
  CHECK_SYS(h_global);
  unsigned char* res_data = static_cast<unsigned char*>(LockResource(h_global));
  CHECK_SYS(res_data);
  const IconGroupHeader* header = reinterpret_cast<const IconGroupHeader*>(res_data);
  for (unsigned i = 0; i < header->count; i++) {
    IconImageRsrc image_rsrc;
    const IconGroupEntry* entry = reinterpret_cast<const IconGroupEntry*>(res_data + sizeof(IconGroupHeader) + i * sizeof(IconGroupEntry));
    image_rsrc.id = entry->id;
    image_rsrc.lang_id = lang_id;
    image_rsrc.image.width = entry->width;
    image_rsrc.image.height = entry->height;
    image_rsrc.image.color_cnt = entry->color_cnt;
    image_rsrc.image.plane_cnt = entry->plane_cnt;
    image_rsrc.image.bit_cnt = entry->bit_cnt;
    HRSRC h_rsrc = FindResourceEx(h_module, RT_ICON, MAKEINTRESOURCE(entry->id), lang_id);
    CHECK_SYS(h_rsrc);
    HGLOBAL h_global = LoadResource(h_module, h_rsrc);
    CHECK_SYS(h_global);
    unsigned char* icon_data = static_cast<unsigned char*>(LockResource(h_global));
    CHECK_SYS(icon_data);
    image_rsrc.image.bitmap.assign(icon_data, icon_data + entry->size);
    icon_rsrc.images.push_back(image_rsrc);
  }
  return icon_rsrc;
}

BOOL CALLBACK enum_names_proc(HMODULE h_module, LPCTSTR type, LPTSTR name, LONG_PTR param) {
  try {
    list<RsrcId>* result = reinterpret_cast<list<RsrcId>*>(param);
    result->push_back(name);
    return TRUE;
  }
  catch (...) {
    return FALSE;
  }
}

list<RsrcId> enum_rsrc_names(HMODULE h_module, LPCTSTR type) {
  list<RsrcId> result;
  EnumResourceNames(h_module, type, enum_names_proc, reinterpret_cast<LONG_PTR>(&result));
  return result;
}

BOOL CALLBACK enum_langs_proc(HMODULE h_module, LPCTSTR type, LPCTSTR name, WORD language, LONG_PTR param) {
  try {
    list<WORD>* result = reinterpret_cast<list<WORD>*>(param);
    result->push_back(language);
    return TRUE;
  }
  catch (...) {
    return FALSE;
  }
}

list<WORD> enum_rsrc_langs(HMODULE h_module, LPCTSTR type, LPCTSTR name) {
  list<WORD> result;
  EnumResourceLanguages(h_module, type, name, enum_langs_proc, reinterpret_cast<LONG_PTR>(&result));
  return result;
}

class RsrcModule {
private:
  HMODULE h_module;
public:
  RsrcModule(const wstring& file_path) {
    h_module = LoadLibraryEx(file_path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    CHECK_SYS(h_module);
  }
  ~RsrcModule() {
    close();
  }
  HMODULE handle() const {
    return h_module;
  }
  void close() {
    if (h_module) {
      FreeLibrary(h_module);
      h_module = nullptr;
    }
  }
};

class ResourceUpdate {
private:
  HANDLE handle;
public:
  ResourceUpdate(const wstring& file_path) {
    handle = BeginUpdateResource(file_path.c_str(), FALSE);
    CHECK_SYS(handle);
  }
  ~ResourceUpdate() {
    if (handle)
      EndUpdateResource(handle, TRUE);
  }
  void update(const wchar_t* type, const wchar_t* name, WORD lang_id, const unsigned char* data, DWORD size) {
    assert(handle);
    CHECK_SYS(UpdateResource(handle, type, name, lang_id, reinterpret_cast<LPVOID>(const_cast<unsigned char*>(data)), size));
  }
  void finalize() {
    CHECK_SYS(EndUpdateResource(handle, FALSE));
    handle = nullptr;
  }
};

void replace_icon(const wstring& pe_path, const wstring& ico_path) {
  list<IconRsrc> icons;
  RsrcModule module(pe_path);
  list<RsrcId> group_ids = enum_rsrc_names(module.handle(), RT_GROUP_ICON);
  for_each(group_ids.cbegin(), group_ids.cend(), [&] (const RsrcId& id) {
    list<WORD> lang_ids = enum_rsrc_langs(module.handle(), RT_GROUP_ICON, id);
    for_each(lang_ids.cbegin(), lang_ids.cend(), [&] (WORD lang_id) {
      icons.push_back(load_icon_rsrc(module.handle(), id, lang_id));
    });
  });
  module.close();

  ResourceUpdate rupdate(pe_path);
  // delete existing icons
  for_each(icons.cbegin(), icons.cend(), [&] (const IconRsrc& icon) {
    for_each (icon.images.cbegin(), icon.images.cend(), [&] (const IconImageRsrc& image) {
      rupdate.update(RT_ICON, MAKEINTRESOURCE(image.id), image.lang_id, nullptr, 0);
    });
    rupdate.update(RT_GROUP_ICON, icon.id, icon.lang_id, nullptr, 0);
  });

  WORD lang_id;
  if (!icons.empty())
    lang_id = icons.front().lang_id;
  else
    lang_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
  IconFile icon_file = load_icon_file(ico_path);
  IconRsrc icon_rsrc;
  icon_rsrc.lang_id = lang_id;
  for_each(icon_file.cbegin(), icon_file.cend(), [&] (const IconImage& image) {
    IconImageRsrc image_rsrc;
    image_rsrc.lang_id = lang_id;
    image_rsrc.image = image;
    icon_rsrc.images.push_back(image_rsrc);
  });

  // drop first icon (all languages)
  if (!icons.empty()) {
    RsrcId id = icons.front().id;
    while (!icons.empty() && icons.front().id == id)
      icons.pop_front();
  }

  icons.push_front(icon_rsrc);

  // renumber resource ids
  WORD icon_id = 1;
  WORD image_id = 1;
  for_each(icons.begin(), icons.end(), [&] (IconRsrc& icon) {
    if (icon.id.is_int()) {
      icon.id = MAKEINTRESOURCE(icon_id);
      icon_id++;
    }
    for_each(icon.images.begin(), icon.images.end(), [&] (IconImageRsrc& image) {
      image.id = image_id;
      image_id++;
    });
  });

  // write new icons
  for_each(icons.cbegin(), icons.cend(), [&] (const IconRsrc& icon) {
    Buffer<unsigned char> buf(sizeof(IconGroupHeader) + icon.images.size() * sizeof(IconGroupEntry));
    IconGroupHeader* header = reinterpret_cast<IconGroupHeader*>(buf.data());
    header->reserved = 0;
    header->type = 1;
    header->count = icon.images.size();
    unsigned offset = sizeof(IconGroupHeader);
    for_each (icon.images.cbegin(), icon.images.cend(), [&] (const IconImageRsrc& image) {
      IconGroupEntry* entry = reinterpret_cast<IconGroupEntry*>(buf.data() + offset);
      entry->width = image.image.width;
      entry->height = image.image.height;
      entry->color_cnt = image.image.color_cnt;
      entry->reserved = 0;
      entry->plane_cnt = image.image.plane_cnt;
      entry->bit_cnt = image.image.bit_cnt;
      entry->size = image.image.bitmap.size();
      entry->id = image.id;
      rupdate.update(RT_ICON, MAKEINTRESOURCE(image.id), image.lang_id, image.image.bitmap.data(), image.image.bitmap.size());
      offset += sizeof(IconGroupEntry);
    });
    rupdate.update(RT_GROUP_ICON, icon.id, icon.lang_id, buf.data(), buf.size());
  });
  rupdate.finalize();
}
