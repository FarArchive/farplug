#ifndef _PLUGIN_H
#define _PLUGIN_H

struct DeviceInfo {
  RAPIDEVICEID id;
  UnicodeString name;
  UnicodeString platform;
  UnicodeString con_type;
  UnicodeString strid();
};

struct PluginItemList: public Array<PluginPanelItem> {
#ifdef FARAPI18
  ObjectArray<UnicodeString> names;
#endif // FARAPI18
};

class InfoPanel: private Array<InfoPanelLine> {
private:
#ifdef FARAPI18
  ObjectArray<UnicodeString> info_lines;
#endif
public:
  void clear();
  void add_separator(const UnicodeString& text);
  void add_info(const UnicodeString& name, const UnicodeString& value);
  const InfoPanelLine* data() const { return Array<InfoPanelLine>::data(); }
  unsigned size() const { return Array<InfoPanelLine>::size(); }
};

struct PluginInstance {
  IRAPISession* session;
  IRAPIDevice* device;
  UnicodeString current_dir;
#ifdef FARAPI17
  AnsiString current_dir_oem;
#endif // FARAPI17
  ObjectArray<PluginItemList> file_lists;
  DeviceInfo device_info;
  UnicodeString last_object;
  InfoPanel sys_info;
  FarStr panel_title;
  unsigned __int64 free_space;
#ifdef FARAPI18
  UnicodeString directory_name_buf;
  UnicodeString dest_path_buf;
#endif // FARAPI18
  PluginInstance(): session(NULL), device(NULL) {
  }
  ~PluginInstance() {
    if (session != NULL) session->Release();
    if (device != NULL) device->Release();
  }
};

extern struct PluginStartupInfo g_far;
extern ModuleVersion g_version;
extern unsigned __int64 g_time_freq;

#endif // _PLUGIN_H
