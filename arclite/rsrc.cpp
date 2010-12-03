#include "utils.hpp"
#include "sysutils.hpp"

struct IconImage {
  BYTE width;
  BYTE height;
  BYTE color_cnt;
  WORD plane_cnt;
  WORD bit_cnt;
  WORD id;
  ByteVector bitmap;
};

struct IconRsrc {
  wstring name;
  WORD id;
  WORD lang_id;
  list<IconImage> images;
};

IconRsrc load_icon_from_file(const wstring& file_path) {
  IconRsrc icon;
  File file(file_path, FILE_READ_DATA, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  unsigned __int64 file_size = file.size();
  file.set_pos(sizeof(WORD), FILE_CURRENT); // reserved
  file.set_pos(sizeof(WORD), FILE_CURRENT); // type
  WORD count;
  file.read(&count, sizeof(count));
  for (unsigned i = 0; i < count; i++) {
    IconImage image;
    file.read(&image.width, sizeof(image.width));
    file.read(&image.height, sizeof(image.height));
    file.read(&image.color_cnt, sizeof(image.color_cnt));
    file.set_pos(sizeof(BYTE), FILE_CURRENT); // reserved
    file.read(&image.plane_cnt, sizeof(image.plane_cnt));
    file.read(&image.bit_cnt, sizeof(image.bit_cnt));
    DWORD bytes_in_res;
    file.read(&bytes_in_res, sizeof(bytes_in_res));
    DWORD image_offset;
    file.read(&image_offset, sizeof(image_offset));
    CHECK(image_offset + bytes_in_res <= file_size);
    unsigned __int64 cur_pos = file.set_pos(0, FILE_CURRENT);
    file.set_pos(image_offset, FILE_BEGIN);
    Buffer<unsigned char> buf(bytes_in_res);
    file.read(buf.data(), buf.size());
    image.bitmap.assign(buf.data(), buf.data() + buf.size());
    file.set_pos(cur_pos);
    icon.images.push_back(image);
  }
  return icon;
}

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

IconRsrc load_icon_from_rsrc(HMODULE h_module, LPCTSTR name, WORD language) {
  IconRsrc icon;
  if (IS_INTRESOURCE(name))
    icon.id = reinterpret_cast<WORD>(name);
  else
    icon.name = name;
  icon.lang_id = language;
  HRSRC h_rsrc = FindResource(h_module, name, RT_GROUP_ICON);
  CHECK_SYS(h_rsrc);
  HGLOBAL h_global = LoadResource(h_module, h_rsrc);
  CHECK_SYS(h_global);
  unsigned char* res_data = static_cast<unsigned char*>(LockResource(h_global));
  CHECK_SYS(res_data);
  const IconGroupHeader* header = reinterpret_cast<const IconGroupHeader*>(res_data);
  for (unsigned i = 0; i < header->count; i++) {
    IconImage image;
    const IconGroupEntry* entry = reinterpret_cast<const IconGroupEntry*>(res_data + sizeof(IconGroupHeader) + i * sizeof(IconGroupEntry));
    image.width = entry->width;
    image.height = entry->height;
    image.color_cnt = entry->color_cnt;
    image.plane_cnt = entry->plane_cnt;
    image.bit_cnt = entry->bit_cnt;
    image.id = entry->id;
    HRSRC h_rsrc = FindResource(h_module, MAKEINTRESOURCE(entry->id), RT_ICON);
    CHECK_SYS(h_rsrc);
    HGLOBAL h_global = LoadResource(h_module, h_rsrc);
    CHECK_SYS(h_global);
    unsigned char* icon_data = static_cast<unsigned char*>(LockResource(h_global));
    CHECK_SYS(icon_data);
    image.bitmap.assign(icon_data, icon_data + entry->size);
    icon.images.push_back(image);
  }
  return icon;
}

class IconRsrcLoader {
private:
  HMODULE h_module;
  list<IconRsrc> icons;
  Error error;
  static BOOL CALLBACK enum_langs_proc(HMODULE h_module, LPCTSTR type, LPCTSTR name, WORD language, LONG_PTR param) {
    IconRsrcLoader* loader = reinterpret_cast<IconRsrcLoader*>(param);
    try {
      loader->icons.push_back(load_icon_from_rsrc(h_module, name, language));
    }
    catch (const Error& e) {
      loader->error = e;
      return FALSE;
    }
    return TRUE;
  }
  static BOOL CALLBACK enum_names_proc(HMODULE h_module, LPCTSTR type, LPTSTR name, LONG_PTR param) {
    IconRsrcLoader* loader = reinterpret_cast<IconRsrcLoader*>(param);
    EnumResourceLanguages(h_module, type, name, enum_langs_proc, param);
    return loader->error ? FALSE : TRUE;
  }
public:
  IconRsrcLoader(const wstring& file_path) {
    h_module = LoadLibraryEx(file_path.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE);
    CHECK_SYS(h_module);
  }
  ~IconRsrcLoader() {
    FreeLibrary(h_module);
  }
  list<IconRsrc> load() {
    icons.clear();
    error = Error();
    EnumResourceNames(h_module, RT_GROUP_ICON, enum_names_proc, reinterpret_cast<LONG_PTR>(this));
    if (error)
      throw error;
    return icons;
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
  {
    IconRsrcLoader loader(pe_path);
    icons = loader.load();
  }

  ResourceUpdate rupdate(pe_path);
  // delete existing icons
  for_each(icons.cbegin(), icons.cend(), [&] (const IconRsrc& icon) {
    for_each (icon.images.cbegin(), icon.images.cend(), [&] (const IconImage& image) {
      rupdate.update(RT_ICON, MAKEINTRESOURCE(image.id), icon.lang_id, nullptr, 0);
    });
    rupdate.update(RT_GROUP_ICON, icon.name.empty() ? MAKEINTRESOURCE(icon.id) : icon.name.c_str(), icon.lang_id, nullptr, 0);
  });

  if (!icons.empty())
    icons.pop_front();
  IconRsrc icon = load_icon_from_file(ico_path);
  icons.push_front(icon);

  // create icons
  unsigned icon_group_id = 1;
  unsigned icon_id = 1;
  for_each(icons.cbegin(), icons.cend(), [&] (const IconRsrc& icon) {
    Buffer<unsigned char> buf(sizeof(IconGroupHeader) + icon.images.size() * sizeof(IconGroupEntry));
    IconGroupHeader* header = reinterpret_cast<IconGroupHeader*>(buf.data());
    header->reserved = 0;
    header->type = 1;
    header->count = icon.images.size();
    unsigned offset = sizeof(IconGroupHeader);
    for_each (icon.images.cbegin(), icon.images.cend(), [&] (const IconImage& image) {
      IconGroupEntry* entry = reinterpret_cast<IconGroupEntry*>(buf.data() + offset);
      entry->width = image.width;
      entry->height = image.height;
      entry->color_cnt = image.color_cnt;
      entry->reserved = 0;
      entry->plane_cnt = image.plane_cnt;
      entry->bit_cnt = image.bit_cnt;
      entry->size = image.bitmap.size();
      entry->id = icon_id;
      rupdate.update(RT_ICON, MAKEINTRESOURCE(icon_id), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), image.bitmap.data(), image.bitmap.size());
      icon_id++;
      offset += sizeof(IconGroupEntry);
    });
    rupdate.update(RT_GROUP_ICON, MAKEINTRESOURCE(icon_group_id), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), buf.data(), buf.size());
    icon_group_id++;
  });
  rupdate.finalize();
}
