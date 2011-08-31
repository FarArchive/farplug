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
  ObjectArray<UnicodeString> names;
};

class InfoPanel: private Array<InfoPanelLine> {
private:
  ObjectArray<UnicodeString> info_lines;
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
  ObjectArray<PluginItemList> file_lists;
  DeviceInfo device_info;
  UnicodeString last_object;
  InfoPanel sys_info;
  UnicodeString panel_title;
  unsigned __int64 free_space;
  UnicodeString directory_name_buf;
  UnicodeString dest_path_buf;
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
