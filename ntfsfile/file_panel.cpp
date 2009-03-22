#include <windows.h>

#include <list>

#include "plugin.hpp"
#include "farkeys.hpp"

#include "col/AnsiString.h"
#include "col/UnicodeString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#include "farapi_config.h"

#define _ERROR_WINDOWS
#include "error.h"

#include "msg.h"

#include "options.h"
#include "utils.h"
#include "dlgapi.h"
#include "ntfs.h"
#include "volume.h"
#include "ntfs_file.h"
#include "file_panel.h"

#define IS_DIR(find_data) (((find_data).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
#define IS_REPARSE(find_data) (((find_data).dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT)
#define IS_DOT_DIR(find_data) (IS_DIR(find_data) && ((wcscmp(find_data.cFileName, L".") == 0) || (wcscmp(find_data.cFileName, L"..") == 0)))
#define FILE_SIZE(find_data) ((((unsigned __int64) (find_data).nFileSizeHigh) << 32) + (find_data).nFileSizeLow)

extern struct PluginStartupInfo g_far;

FilePanelMode g_file_panel_mode;
Array<FilePanel*> FilePanel::g_file_panels;

PanelState save_state(HANDLE h_panel) {
  PanelState state;
  PanelInfo pi;
  if (far_control_ptr(h_panel, FCTL_GETPANELINFO, &pi)) {
    state.directory = far_get_cur_dir(h_panel, pi);
    if (pi.CurrentItem < pi.ItemsNumber) {
      PluginPanelItem* ppi = far_get_panel_item(h_panel, pi.CurrentItem, pi);
      if (ppi) {
        state.current_file = CompositeFileName(ppi->FindData);
      }
    }
    if (pi.TopPanelItem < pi.ItemsNumber) {
      PluginPanelItem* ppi = far_get_panel_item(h_panel, pi.TopPanelItem, pi);
      if (ppi) {
        state.top_panel_file = CompositeFileName(ppi->FindData);
      }
    }
    state.selected_files.extend(pi.SelectedItemsNumber);
    for (int i = 0; i < pi.SelectedItemsNumber; i++) {
      PluginPanelItem* ppi = far_get_selected_panel_item(h_panel, i, pi);
      if (ppi) {
        if (ppi->Flags & PPIF_SELECTED) state.selected_files += CompositeFileName(ppi->FindData);
      }
    }
  }
  return state;
}

void restore_state(HANDLE h_panel, const PanelState& state) {
  PanelInfo pi;
  if (far_control_ptr(h_panel, FCTL_GETPANELINFO, &pi)) {
    PanelRedrawInfo pri;
    for (int i = 0; i < pi.ItemsNumber; i++) {
      PluginPanelItem* ppi = far_get_panel_item(h_panel, i, pi);
      if (ppi) {
        for (unsigned j = 0; j < state.selected_files.size(); j++) {
          if (state.selected_files[j] == ppi->FindData) {
#ifdef FARAPI17
            ppi->Flags |= PPIF_SELECTED;
#endif
#ifdef FARAPI18
            g_far.Control(h_panel, FCTL_SETSELECTION, i, TRUE);
#endif
          }
        }
        if (state.current_file == ppi->FindData) pri.CurrentItem = i;
        if (state.top_panel_file == ppi->FindData) pri.TopPanelItem = i;
      }
    }
#ifdef FARAPI17
    g_far.Control(h_panel, FCTL_SETSELECTION, &pi);
#endif
    far_control_ptr(h_panel, FCTL_REDRAWPANEL, &pri);
  }
}

FilePanel* FilePanel::open() {
  FilePanel* panel = new FilePanel();
  try {
    panel->flat_mode = false;
    panel->mft_mode = false;
    panel->usn_journal_id = 0;
    panel->saved_state = save_state(INVALID_HANDLE_VALUE);
    if (g_file_panel_mode.default_mft_mode) {
      panel->current_dir = FARSTR_TO_UNICODE(panel->saved_state.directory);
      panel->toggle_mft_mode();
    }
    panel->change_directory(FARSTR_TO_UNICODE(panel->saved_state.directory), false);
    // signal to restore selection & current item after panel is created
    DWORD key = KEY_F24 | KEY_CTRL | KEY_ALT | KEY_SHIFT;
    KeySequence ks = { KSFLAGS_DISABLEOUTPUT, 1, &key };
    g_far.AdvControl(g_far.ModuleNumber, ACTL_POSTKEYSEQUENCE, &ks);
    g_file_panels += panel;
  }
  catch (...) {
    delete panel;
    throw;
  }
  return panel;
}

void FilePanel::apply_saved_state() {
#ifdef FARAPI17
  g_far.Control(this, FCTL_SETSORTMODE, &g_file_panel_mode.sort_mode);
  g_far.Control(this, FCTL_SETSORTORDER, &g_file_panel_mode.reverse_sort);
  g_far.Control(this, FCTL_SETNUMERICSORT, &g_file_panel_mode.numeric_sort);
#endif
#ifdef FARAPI18
  far_control_int(this, FCTL_SETSORTMODE, g_file_panel_mode.sort_mode);
  far_control_int(this, FCTL_SETSORTORDER, g_file_panel_mode.reverse_sort);
  far_control_int(this, FCTL_SETNUMERICSORT, g_file_panel_mode.numeric_sort);
#endif
  restore_state(this, saved_state);
}

void FilePanel::close() {
  PanelState state = save_state(this);
  far_control_ptr(INVALID_HANDLE_VALUE, FCTL_SETPANELDIR, state.directory.data());
  restore_state(INVALID_HANDLE_VALUE, state);
}

void FilePanel::on_close() {
  if (g_file_panel_mode.use_usn_journal && g_file_panel_mode.use_cache) {
    try {
      if (mft_index.size() != 0) store_mft_index();
    }
    catch (...) {
    }
  }
  delete_usn_journal();
  PanelInfo pi;
  if (far_control_ptr(this, FCTL_GETPANELSHORTINFO, &pi)) {
    g_file_panel_mode.sort_mode = pi.SortMode;
    g_file_panel_mode.reverse_sort = pi.Flags & PFLAGS_REVERSESORTORDER ? 1 : 0;
    g_file_panel_mode.numeric_sort = pi.Flags & PFLAGS_NUMERICSORT ? 1 : 0;
  }
  store_plugin_options();
  g_file_panels.remove(g_file_panels.search(this));
  delete this;
}

FilePanel* FilePanel::get_active_panel() {
  for (unsigned i = 0; i < g_file_panels.size(); i++) {
    PanelInfo pi;
    if (far_control_ptr(g_file_panels[i], FCTL_GETPANELSHORTINFO, &pi)) {
      if (pi.Focus) {
        return g_file_panels[i];
      }
    }
  }
  return NULL;
}

void FileListProgress::do_update_ui() {
  const unsigned c_client_xs = 40;
  ObjectArray<UnicodeString> lines;
  lines += center(UnicodeString::format(far_get_msg(MSG_FILE_PANEL_READ_DIR_PROGRESS_MESSAGE).data(), count), c_client_xs);
  draw_text_box(far_get_msg(MSG_FILE_PANEL_READ_DIR_PROGRESS_TITLE), lines, c_client_xs);
}

UnicodeString& fit_col_str(UnicodeString& str, unsigned size) {
  if (str.size() < size) {
    str.extend(size);
    unsigned cnt = size - str.size();
    wchar_t* buf = str.buf();
    wmemmove(buf + cnt, buf, str.size());
    wmemset(buf, L' ', cnt);
    str.set_size(size);
  }
  else if (str.size() > size) {
    str.set_size(size);
    if (size != 0) str.last_item() = L'}';
  }
  return str;
}

PluginItemList FilePanel::create_panel_items(const std::list<PanelItemData>& pid_list, bool search_mode) {
  PluginItemList pi_list;
  unsigned sz = static_cast<unsigned>(pid_list.size());
  pi_list.extend(sz);
#ifdef FARAPI17
  pi_list.names.extend(sz);
  UnicodeString uname;
  AnsiString oname;
#endif
#ifdef FARAPI18
  pi_list.names.extend(sz * 2);
#endif
  pi_list.col_str.extend(sz * col_indices.size());
  pi_list.col_data.extend(sz);
  Array<const FarCh*> col_data;
  col_data.extend(col_indices.size());
  for (std::list<PanelItemData>::const_iterator pid = pid_list.begin(); pid != pid_list.end(); pid++) {
    PluginPanelItem pi;
    memset(&pi, 0, sizeof(pi));
#ifdef FARAPI17
    unicode_to_oem(oname, pid->file_name);
    oem_to_unicode(uname, oname);
    if ((uname != pid->file_name) || (pid->file_name.size() >= MAX_PATH)) {
      Array<unsigned char> uni_name;
      u32 uni_name_size = sizeof(u32) + pid->file_name.size() * sizeof(wchar_t);
      uni_name.extend(uni_name_size);
      uni_name.add(reinterpret_cast<const u8*>(&uni_name_size), sizeof(uni_name_size));
      uni_name.add(reinterpret_cast<const u8*>(pid->file_name.data()), pid->file_name.size() * sizeof(wchar_t));
      pi_list.names += uni_name;
      pi.UserData = reinterpret_cast<DWORD_PTR>(pi_list.names.last().data());
      pi.Flags |= PPIF_USERDATA;
    }
    strcpy(pi.FindData.cFileName, pid->file_name.size() >= MAX_PATH ? unicode_to_oem(pid->file_name).left(MAX_PATH - 1).data() : unicode_to_oem(pid->file_name).data());
    strcpy(pi.FindData.cAlternateFileName, unicode_to_oem(pid->alt_file_name).data());
#endif
#ifdef FARAPI18
    pi_list.names += pid->file_name;
    pi.FindData.lpwszFileName = const_cast<wchar_t*>(pi_list.names.last().data());
    if (pid->alt_file_name.size() != 0) {
      pi_list.names += pid->alt_file_name;
      pi.FindData.lpwszAlternateFileName = const_cast<wchar_t*>(pi_list.names.last().data());
    }
#endif
    pi.FindData.dwFileAttributes = pid->file_attr;
    pi.FindData.ftCreationTime = pid->creation_time;
    pi.FindData.ftLastAccessTime = pid->last_access_time;
    pi.FindData.ftLastWriteTime = pid->last_write_time;
#ifdef FARAPI17
    pi.FindData.nFileSizeHigh = (DWORD) (pid->data_size >> 32);
    pi.FindData.nFileSizeLow = (DWORD) (pid->data_size & 0xFFFFFFFF);
    pi.PackSizeHigh = (DWORD) (pid->disk_size >> 32);
    pi.PackSize = (DWORD) (pid->disk_size & 0xFFFFFFFF);
#endif
#ifdef FARAPI18
    pi.FindData.nFileSize = pid->data_size;
    pi.FindData.nPackSize = pid->disk_size;
#endif
    pi.NumberOfLinks = pid->hard_link_cnt;
    // custom columns
    col_data.clear();
    for (unsigned i = 0; i < col_indices.size(); i++) {
      if (search_mode) col_data += FAR_T("");
      else if (pid->error) col_data += far_msg_ptr(MSG_FILE_PANEL_ERROR_MARKER);
      else {
        switch (col_indices[i]) {
        case 0: // data size
          if ((pid->stream_cnt == 0) && !pid->ntfs_attr) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(format_data_size(pid->data_size, short_size_suffixes), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 1: // disk size
          if (pid->resident) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(format_data_size(pid->disk_size, short_size_suffixes), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 2: // fragment count
          if (pid->resident) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(int_to_str(pid->fragment_cnt), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 3: // stream count
          if (pid->ntfs_attr) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(int_to_str(pid->stream_cnt), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 4: // hard links
          if (pid->ntfs_attr) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(int_to_str(pid->hard_link_cnt), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 5: // mft record count
          if (pid->ntfs_attr) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(int_to_str(pid->mft_rec_cnt), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        case 6: // valid size
          if ((pid->stream_cnt == 0) && !pid->ntfs_attr) col_data += FAR_T("");
          else {
            pi_list.col_str += UNICODE_TO_FARSTR(fit_col_str(format_data_size(pid->valid_size, short_size_suffixes), col_sizes[i]));
            col_data += pi_list.col_str.last().data();
          }
          break;
        default:
          assert(false);
        }
      }
    }
    pi_list.col_data += col_data;
    pi.CustomColumnData = (FarCh**) pi_list.col_data.last().data();
    pi.CustomColumnNumber = pi_list.col_data.last().size();
    pi_list += pi;
  }
  return pi_list;
}

void FilePanel::scan_dir(const UnicodeString& root_path, const UnicodeString& rel_path, std::list<PanelItemData>& pid_list, FileListProgress& progress) {
  UnicodeString path = add_trailing_slash(root_path) + rel_path;
  bool more = true;
  WIN32_FIND_DATAW find_data;
  HANDLE h_find = FindFirstFileW(long_path(add_trailing_slash(path) + L"*").data(), &find_data);
  try {
    if (h_find == INVALID_HANDLE_VALUE) {
      // special case: symlink that denies access to directory, try real path
      if (GetLastError() == ERROR_ACCESS_DENIED) {
        DWORD attr = GetFileAttributesW(long_path(path).data());
        CHECK_SYS(attr != INVALID_FILE_ATTRIBUTES);
        if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
          h_find = FindFirstFileW(long_path(add_trailing_slash(get_real_path(path)) + L"*").data(), &find_data);
        }
        else CHECK_SYS(false);
      }
    }
    if (h_find == INVALID_HANDLE_VALUE) {
      CHECK_SYS(GetLastError() == ERROR_NO_MORE_FILES);
      more = false;
    }
  }
  catch (...) {
    if (flat_mode) more = false;
    else throw;
  }
  ALLOC_RSRC(;);
  while (more) {
    if (!IS_DOT_DIR(find_data)) {
      UnicodeString file_path = add_trailing_slash(path) + find_data.cFileName;
      UnicodeString rel_file_path = add_trailing_slash(rel_path) + find_data.cFileName;

      UnicodeString file_name = flat_mode ? rel_file_path : find_data.cFileName;
      u64 data_size = 0;
      u64 nr_disk_size = 0;
      u64 valid_size = 0;
      unsigned stream_cnt = 0;
      unsigned fragment_cnt = 0;
      unsigned hard_link_cnt = 0;
      unsigned mft_rec_cnt = 0;
      bool error = false;
      FileInfo file_info;
      try {
        BY_HANDLE_FILE_INFORMATION h_file_info;
        HANDLE h_file = CreateFileW(long_path(file_path).data(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_POSIX_SEMANTICS, NULL);
        CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
        ALLOC_RSRC(;);
        CHECK_SYS(GetFileInformationByHandle(h_file, &h_file_info));
        FREE_RSRC(CloseHandle(h_file));

        u64 file_ref_num = ((u64) h_file_info.nFileIndexHigh << 32) + h_file_info.nFileIndexLow;
        file_info.volume = &volume;
        file_info.process_file(file_ref_num);

        hard_link_cnt = h_file_info.nNumberOfLinks;
        mft_rec_cnt = file_info.mft_rec_cnt;
        for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
          const AttrInfo& attr_info = file_info.attr_list[i];
          if (!attr_info.resident) {
            nr_disk_size += attr_info.disk_size;
          }
          if (attr_info.type == AT_DATA) {
            data_size += attr_info.data_size;
            valid_size += attr_info.valid_size;
            stream_cnt++;
          }
          if (attr_info.fragments > 1) fragment_cnt += (unsigned) (attr_info.fragments - 1);
        }
      }
      catch (...) {
        error = true;
      }

      // is file fully resident?
      bool fully_resident = true;
      for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
        if (!file_info.attr_list[i].resident) {
          fully_resident = false;
          break;
        }
      }

      PanelItemData pid;
      pid.file_name = file_name;
      pid.alt_file_name = find_data.cAlternateFileName;
      pid.file_attr = find_data.dwFileAttributes;
      pid.creation_time = find_data.ftCreationTime;
      pid.last_access_time = find_data.ftLastAccessTime;
      pid.last_write_time = find_data.ftLastWriteTime;
      pid.data_size = data_size;
      pid.disk_size = nr_disk_size;
      pid.valid_size = valid_size;
      pid.fragment_cnt = fragment_cnt;
      pid.stream_cnt = stream_cnt;
      pid.hard_link_cnt = hard_link_cnt;
      pid.mft_rec_cnt = mft_rec_cnt;
      pid.error = error;
      pid.ntfs_attr = false;
      pid.resident = fully_resident;
      pid_list.push_back(pid);

      if (g_file_panel_mode.show_streams && !error) {
        unsigned cnt = 0;
        bool named_data = false;
        for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
          const AttrInfo& attr = file_info.attr_list[i];
          if (!attr.resident || (attr.type == AT_DATA)) cnt++;
          if ((attr.type == AT_DATA) && (attr.name.size() != 0)) named_data = true;
        }
        // multiple non-resident/data attributes or at least one named data attribute
        if ((cnt > 1) || named_data) {
          for (unsigned i = 0; i < file_info.attr_list.size(); i++) {
            const AttrInfo& attr = file_info.attr_list[i];
            if (attr.resident && (attr.type != AT_DATA)) continue;
            if (!g_file_panel_mode.show_main_stream && (attr.type == AT_DATA) && (attr.name.size() == 0)) continue;

            UnicodeString file_name = UnicodeString(flat_mode ? rel_file_path : find_data.cFileName) + L":" + attr.name + L":$" + attr.type_name();

            unsigned fragment_cnt = (unsigned) attr.fragments;
            if (fragment_cnt != 0) fragment_cnt--;

            DWORD file_attr = find_data.dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY & ~FILE_ATTRIBUTE_REPARSE_POINT;
            if (attr.compressed) file_attr |= FILE_ATTRIBUTE_COMPRESSED;
            else file_attr &= ~FILE_ATTRIBUTE_COMPRESSED;
            if (attr.encrypted) file_attr |= FILE_ATTRIBUTE_ENCRYPTED;
            else file_attr &= ~FILE_ATTRIBUTE_ENCRYPTED;
            if (attr.sparse) file_attr |= FILE_ATTRIBUTE_SPARSE_FILE;
            else file_attr &= ~FILE_ATTRIBUTE_SPARSE_FILE;

            PanelItemData pid;
            pid.file_name = file_name;
            pid.alt_file_name = L"";
            pid.file_attr = file_attr;
            pid.creation_time = find_data.ftCreationTime;
            pid.last_access_time = find_data.ftLastAccessTime;
            pid.last_write_time = find_data.ftLastWriteTime;
            pid.data_size = attr.data_size;
            pid.disk_size = attr.disk_size;
            pid.valid_size = attr.valid_size;
            pid.fragment_cnt = fragment_cnt;
            pid.stream_cnt = 0;
            pid.hard_link_cnt = 0;
            pid.mft_rec_cnt = 0;
            pid.error = false;
            pid.ntfs_attr = true;
            pid.resident = attr.resident;
            pid_list.push_back(pid);
          }
        }
      }

      progress.count++;
      progress.update_ui();

      if (flat_mode && IS_DIR(find_data) && !IS_REPARSE(find_data)) scan_dir(root_path, rel_file_path, pid_list, progress);
    }
    if (FindNextFileW(h_find, &find_data) == 0) {
      CHECK_SYS(GetLastError() == ERROR_NO_MORE_FILES);
      more = false;
    }
  }
  FREE_RSRC(if (h_find != INVALID_HANDLE_VALUE) VERIFY(FindClose(h_find)));
}

void FilePanel::sort_file_list(std::list<PanelItemData>& pid_list) {
  switch (g_file_panel_mode.custom_sort_mode) {
  case 1:
    struct DataSizeCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.data_size < item2.data_size;
      }
    };
    pid_list.sort(DataSizeCmp());
    break;
  case 2:
    struct DiskSizeCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.disk_size < item2.disk_size;
      }
    };
    pid_list.sort(DiskSizeCmp());
    break;
  case 3:
    struct FragmentsCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.fragment_cnt < item2.fragment_cnt;
      }
    };
    pid_list.sort(FragmentsCmp());
    break;
  case 4:
    struct StreamsCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.stream_cnt < item2.stream_cnt;
      }
    };
    pid_list.sort(StreamsCmp());
    break;
  case 5:
    struct HardLinksCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.hard_link_cnt < item2.hard_link_cnt;
      }
    };
    pid_list.sort(HardLinksCmp());
    break;
  case 6:
    struct MftRecordsCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.mft_rec_cnt < item2.mft_rec_cnt;
      }
    };
    pid_list.sort(MftRecordsCmp());
    break;
  case 7:
    struct FragmLevelCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        if ((item1.fragment_cnt == 0) && (item2.fragment_cnt == 0)) return false;
        else if (item1.fragment_cnt == 0) return true;
        else if (item2.fragment_cnt == 0) return false;
        else {
          assert(item2.disk_size != 0);
          double r1 = (double) (item1.fragment_cnt + 1) / (item2.fragment_cnt + 1);
          double r2 = (double) (item2.fragment_cnt + 1) / (item1.fragment_cnt + 1) * item1.disk_size / item2.disk_size;
          return r1 < r2;
        }
      }
    };
    pid_list.sort(FragmLevelCmp());
    break;
  case 8:
    struct ValidSizeCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.valid_size < item2.valid_size;
      }
    };
    pid_list.sort(ValidSizeCmp());
    break;
  case 9:
    struct NameLenCmp {
      bool operator()(const PanelItemData& item1, const PanelItemData& item2) const {
        return item1.file_name.size() < item2.file_name.size();
      }
    };
    pid_list.sort(NameLenCmp());
    break;
  }
}

void FilePanel::new_file_list(PluginPanelItem*& panel_items, int& item_num, bool search_mode) {
  FileListProgress progress;
  std::list<PanelItemData> pid_list;
  if (mft_mode) {
    if (!search_mode) update_mft_index_from_usn();
    mft_scan_dir(mft_find_path(current_dir), L"", pid_list, progress);
  }
  else scan_dir(current_dir, L"", pid_list, progress);
  if (!search_mode) sort_file_list(pid_list);
  file_lists += create_panel_items(pid_list, search_mode);
  panel_items = (PluginPanelItem*) file_lists.last().data();
  item_num = file_lists.last().size();
}

void FilePanel::clear_file_list(void* file_list_ptr) {
  for (unsigned i = 0; i < file_lists.size(); i++) {
    if (file_lists[i].data() == file_list_ptr) {
      file_lists.remove(i);
      return;
    }
  }
  assert(false);
}

void FilePanel::change_directory(const UnicodeString& target_dir, bool search_mode) {
  UnicodeString new_cur_dir;
  if (target_dir == L"\\") { // root directory
    new_cur_dir = extract_path_root(current_dir);
  }
  else if (target_dir == L"..") { // parent directory
    new_cur_dir = extract_file_dir(current_dir);
  }
  else if (extract_path_root(target_dir).size() != 0) { // absolute path
    new_cur_dir = del_trailing_slash(target_dir);
  }
  else { // subdirectory name
    new_cur_dir = add_trailing_slash(current_dir) + target_dir;
  }
  if (new_cur_dir.equal(new_cur_dir.size() - 1, L':')) new_cur_dir = add_trailing_slash(new_cur_dir);
  if (mft_mode) {
    if (!search_mode) update_mft_index_from_usn();
    mft_find_path(new_cur_dir);
    if (!search_mode) SetCurrentDirectoryW(new_cur_dir.data());
  }
  else {
    WIN32_FIND_DATAW find_data;
    HANDLE h_find = FindFirstFileW(long_path(add_trailing_slash(new_cur_dir) + L"*").data(), &find_data);
    CHECK_SYS(h_find != INVALID_HANDLE_VALUE);
    FindClose(h_find);
    volume.open(extract_path_root(get_real_path(new_cur_dir)));
    SetCurrentDirectoryW(new_cur_dir.data());
  }
  current_dir = new_cur_dir;
#ifdef FARAPI17
  current_dir_oem = unicode_to_oem(current_dir);
#endif // FARAPI17
  if (g_file_panel_mode.flat_mode_auto_off) flat_mode = false;
}

void FilePanel::fill_plugin_info(OpenPluginInfo* info) {
  info->StructSize = sizeof(struct OpenPluginInfo);
  info->Flags = OPIF_USEFILTER | OPIF_USESORTGROUPS | OPIF_ADDDOTS | OPIF_REALNAMES;
  info->Flags |= !flat_mode || g_file_panel_mode.use_highlighting ? OPIF_USEHIGHLIGHTING : OPIF_USEATTRHIGHLIGHTING;
  panel_title = far_msg_ptr(MSG_FILE_PANEL_TITLE_PREFIX);
  if (flat_mode || mft_mode) {
    panel_title += FAR_T('(');
    if (flat_mode) panel_title += FAR_T('*');
    if (mft_mode) panel_title += FAR_T('$');
    panel_title += FAR_T(')');
  }
  panel_title += FAR_T(':');
#ifdef FARAPI17
  info->CurDir = current_dir_oem.data();
  panel_title += current_dir_oem;
#endif // FARAPI17
#ifdef FARAPI18
  info->CurDir = current_dir.data();
  panel_title += current_dir;
#endif // FARAPI18
  info->PanelTitle = panel_title.data();

  col_indices.clear();
  col_sizes.clear();
  parse_column_spec(g_file_panel_mode.col_types, g_file_panel_mode.col_widths, col_types, col_widths, true);
  parse_column_spec(g_file_panel_mode.status_col_types, g_file_panel_mode.status_col_widths, status_col_types, status_col_widths, false);
  memset(&panel_mode, 0, sizeof(panel_mode));
  panel_mode.ColumnTypes = const_cast<FarCh*>(col_types.data());
  panel_mode.ColumnWidths = const_cast<FarCh*>(col_widths.data());
  panel_mode.ColumnTitles = const_cast<FarCh**>(col_titles.data());
  panel_mode.StatusColumnTypes = const_cast<FarCh*>(status_col_types.data());
  panel_mode.StatusColumnWidths = const_cast<FarCh*>(status_col_widths.data());
  panel_mode.FullScreen = g_file_panel_mode.wide;
  info->PanelModesArray = &panel_mode;
  info->PanelModesNumber = 1;
  info->StartPanelMode = '0';
}

class FilePanelModeDialog: public FarDialog {
private:
  enum {
    c_client_xs = 60
  };

  FilePanelMode& mode;

  int col_types_ctrl_id;
  int status_col_types_ctrl_id;
  int col_widths_ctrl_id;
  int status_col_widths_ctrl_id;
  int wide_ctrl_id;
  int show_streams_ctrl_id;
  int show_main_stream_ctrl_id;
  int sort_mode_ctrl_id;
  int use_mft_index_ctrl_id;
  int use_highlighting_ctrl_id;
  int use_usn_journal_ctrl_id;
  int delete_usn_journal_ctrl_id;
  int use_cache_ctrl_id;
  int default_mft_mode_ctrl_id;
  int backward_mft_scan_ctrl_id;
  int cache_dir_lbl_id;
  int cache_dir_ctrl_id;
  int flat_mode_auto_off_ctrl_id;
  int ok_ctrl_id;
  int cancel_ctrl_id;

  static LONG_PTR WINAPI dialog_proc(HANDLE h_dlg, int msg, int param1, LONG_PTR param2) {
    BEGIN_ERROR_HANDLER;
    FilePanelModeDialog* dlg = (FilePanelModeDialog*) FarDialog::get_dlg(h_dlg);
    if ((msg == DN_CLOSE) && (param1 >= 0) && (param1 != dlg->cancel_ctrl_id)) {
      dlg->mode.col_types = dlg->get_text(dlg->col_types_ctrl_id);
      dlg->mode.status_col_types = dlg->get_text(dlg->status_col_types_ctrl_id);
      dlg->mode.col_widths = dlg->get_text(dlg->col_widths_ctrl_id);
      dlg->mode.status_col_widths = dlg->get_text(dlg->status_col_widths_ctrl_id);
      dlg->mode.wide = dlg->get_check(dlg->wide_ctrl_id);
      dlg->mode.custom_sort_mode = dlg->get_list_pos(dlg->sort_mode_ctrl_id);
      dlg->mode.show_streams = dlg->get_check(dlg->show_streams_ctrl_id);
      dlg->mode.show_main_stream = dlg->get_check(dlg->show_main_stream_ctrl_id);
      dlg->mode.use_highlighting = dlg->get_check(dlg->use_highlighting_ctrl_id);
      dlg->mode.use_usn_journal = dlg->get_check(dlg->use_usn_journal_ctrl_id);
      dlg->mode.delete_usn_journal = dlg->get_check(dlg->delete_usn_journal_ctrl_id);
      dlg->mode.use_cache = dlg->get_check(dlg->use_cache_ctrl_id);
      dlg->mode.default_mft_mode = dlg->get_check(dlg->default_mft_mode_ctrl_id);
      dlg->mode.backward_mft_scan = dlg->get_check(dlg->backward_mft_scan_ctrl_id);
      dlg->mode.cache_dir = dlg->get_text(dlg->cache_dir_ctrl_id);
      dlg->mode.flat_mode_auto_off = dlg->get_check(dlg->flat_mode_auto_off_ctrl_id);
    }
    else if ((msg == DN_BTNCLICK) && (param1 == dlg->show_streams_ctrl_id)) {
      dlg->enable(dlg->show_main_stream_ctrl_id, param2 != 0);
    }
    else if ((msg == DN_BTNCLICK) && (param1 == dlg->use_usn_journal_ctrl_id)) {
      dlg->enable(dlg->delete_usn_journal_ctrl_id, param2 != 0);
      dlg->enable(dlg->use_cache_ctrl_id, param2 != 0);
      dlg->enable(dlg->cache_dir_lbl_id, param2 != 0);
      dlg->enable(dlg->cache_dir_ctrl_id, param2 != 0);
    }
    END_ERROR_HANDLER(;,;);
    return g_far.DefDlgProc(h_dlg, msg, param1, param2);
  }

public:
  FilePanelModeDialog(FilePanelMode& mode): FarDialog(far_get_msg(MSG_FILE_PANEL_MODE_TITLE), c_client_xs), mode(mode) {
  }

  bool show() {
    unsigned col_types_lbl_id = label(far_get_msg(MSG_FILE_PANEL_MODE_COL_TYPES));
    pad(c_client_xs / 2);
    unsigned status_col_types_lbl_id = label(far_get_msg(MSG_FILE_PANEL_MODE_STATUS_COL_TYPES));
    new_line();
    col_types_ctrl_id = var_edit_box(mode.col_types, 30, c_client_xs / 2 - 1);
    link_label(col_types_ctrl_id, col_types_lbl_id);
    pad(c_client_xs / 2);
    status_col_types_ctrl_id = var_edit_box(mode.status_col_types, 30, c_client_xs / 2 - 1);
    link_label(status_col_types_ctrl_id, status_col_types_lbl_id);
    new_line();
    unsigned col_widths_lbl_id = label(far_get_msg(MSG_FILE_PANEL_MODE_COL_WIDTHS));
    pad(c_client_xs / 2);
    unsigned status_col_widths_lbl_id = label(far_get_msg(MSG_FILE_PANEL_MODE_STATUS_COL_WIDTHS));
    new_line();
    col_widths_ctrl_id = var_edit_box(mode.col_widths, 30, c_client_xs / 2 - 1);
    link_label(col_widths_ctrl_id, col_widths_lbl_id);
    pad(c_client_xs / 2);
    status_col_widths_ctrl_id = var_edit_box(mode.status_col_widths, 30, c_client_xs / 2 - 1);
    link_label(status_col_widths_ctrl_id, status_col_widths_lbl_id);
    new_line();
    separator();
    new_line();

    wide_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_MODE_WIDE), mode.wide);
    new_line();
    separator();
    new_line();

    show_streams_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_SHOW_STREAMS), mode.show_streams);
    spacer(2);
    show_main_stream_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_SHOW_MAIN_STREAM), mode.show_main_stream, mode.show_streams ? 0 : DIF_DISABLE);
    new_line();
    label(far_get_msg(MSG_FILE_PANEL_MODE_SORT));
    spacer(1);
    ObjectArray<UnicodeString> items;
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_NOTHING);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_DATA_SIZE);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_DISK_SIZE);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_FRAGMENTS);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_STREAMS);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_HARD_LINKS);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_MFT_RECORDS);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_FRAGM_LEVEL);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_VALID_SIZE);
    items += far_get_msg(MSG_FILE_PANEL_MODE_SORT_NAME_LEN);
    unsigned max_size = 0;
    for (unsigned i = 0; i < items.size(); i++) {
      max_size = max(max_size, items[i].size());
    }
    sort_mode_ctrl_id = combo_box(items, mode.custom_sort_mode, 30, max_size + 1, DIF_DROPDOWNLIST);
    new_line();
    use_highlighting_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_USE_HIGHLIGHTING), mode.use_highlighting);
    new_line();
    default_mft_mode_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_DEFAULT_MFT_MODE), mode.default_mft_mode);
    spacer(2);
    backward_mft_scan_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_BACKWARD_MFT_SCAN), mode.backward_mft_scan);
    new_line();
    flat_mode_auto_off_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_FLAT_MODE_AUTO_OFF), mode.flat_mode_auto_off);
    new_line();
    separator();
    new_line();
    use_usn_journal_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_USE_USN_JOURNAL), mode.use_usn_journal);
    spacer(2);
    delete_usn_journal_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_DELETE_USN_JOURNAL), mode.delete_usn_journal, mode.use_usn_journal ? 0 : DIF_DISABLE);
    new_line();
    use_cache_ctrl_id = check_box(far_get_msg(MSG_FILE_PANEL_USE_CACHE), mode.use_cache, mode.use_usn_journal ? 0 : DIF_DISABLE);
    spacer(2);
    cache_dir_lbl_id = label(far_get_msg(MSG_FILE_PANEL_CACHE_DIR), AUTO_SIZE, mode.use_usn_journal ? 0 : DIF_DISABLE);
    spacer(1);
    cache_dir_ctrl_id = var_edit_box(mode.cache_dir, MAX_PATH, AUTO_SIZE, mode.use_usn_journal ? 0 : DIF_DISABLE);
    new_line();
    separator();
    new_line();

    ok_ctrl_id = def_button(far_get_msg(MSG_BUTTON_OK), DIF_CENTERGROUP);
    cancel_ctrl_id = button(far_get_msg(MSG_BUTTON_CANCEL), DIF_CENTERGROUP);
    new_line();

    int item = FarDialog::show(dialog_proc, FAR_T("file_panel_mode"));

    return (item != -1) && (item != cancel_ctrl_id);
  }
};

bool show_file_panel_mode_dialog(FilePanelMode& mode) {
  return FilePanelModeDialog(mode).show();
}

void FilePanel::parse_column_spec(const UnicodeString& src_col_types, const UnicodeString& src_col_widths, FarStr& col_types, FarStr& col_widths, bool title) {
  const wchar_t* c_col_names[c_cust_col_cnt] = { L"DSZ", L"RSZ", L"FRG", L"STM", L"LNK", L"MFT", L"VSZ" };
  const unsigned c_def_col_sizes[c_cust_col_cnt] = { 7, 7, 5, 3, 3, 3, 7 };
  const FarCh* c_col_titles[c_cust_col_cnt] = { far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_DATA_SIZE), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_DISK_SIZE), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_FRAGMENTS), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_STREAMS), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_HARD_LINKS), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_MFT_RECORDS), far_msg_ptr(MSG_FILE_PANEL_MODE_COL_TITLE_VALID_SIZE) };

  col_types.clear();
  col_widths.clear();
  if (title) col_titles.clear();
  ObjectArray<UnicodeString> col_types_lst = split_str(src_col_types, L',');
  ObjectArray<UnicodeString> col_widths_lst = split_str(src_col_widths, L',');
  for (unsigned i = 0; i < col_types_lst.size(); i++) {
    UnicodeString name = col_types_lst[i];
    unsigned size = 0;
    if (i < col_widths_lst.size()) size = str_to_int(col_widths_lst[i]);
    unsigned col_idx = -1;
    for (unsigned j = 0; j < c_cust_col_cnt; j++) {
      if (name == c_col_names[j]) {
        col_idx = j;
        break;
      }
    }
    if (col_idx != -1) {
      // custom column
      if (size == 0) size = c_def_col_sizes[col_idx];
      unsigned idx = col_indices.search(col_idx);
      if (idx == -1) {
        // new column
        col_indices += col_idx;
        col_sizes += size;
        name = L"C" + int_to_str(col_indices.size() - 1);
      }
      else {
        // column is referenced several times
        if (col_sizes[idx] > size) col_sizes.item(idx) = size;
        name = L"C" + int_to_str(idx);
      }
      if (title) col_titles += c_col_titles[col_idx];
    }
    else {
      // standard column
      if (title) col_titles += NULL;
    }
    if (col_types.size() != 0) col_types += FAR_T(',');
    col_types += UNICODE_TO_FARSTR(name);
    if (col_widths.size() != 0) col_widths += FAR_T(',');
    col_widths += UNICODE_TO_FARSTR(int_to_str(size));
  }
}
