#pragma once

struct VolumeInfo {
  UnicodeString name;
  unsigned cluster_size;
  VolumeInfo(const UnicodeString& file_name);
};

struct NtfsVolume {
  UnicodeString name;
  DWORD serial;
  unsigned file_rec_size;
  unsigned cluster_size;
  unsigned __int64 mft_size;
  HANDLE handle;
  bool synced;
  NtfsVolume(): handle(INVALID_HANDLE_VALUE), serial(0) {
  }
  ~NtfsVolume() {
    close();
  }
  void close() {
    if (handle != INVALID_HANDLE_VALUE) {
      CHECK_SYS(CloseHandle(handle));
      handle = INVALID_HANDLE_VALUE;
    }
    name.clear();
    serial = 0;
  }
  void open(const UnicodeString& volume_name);
  void flush();
};

UnicodeString get_real_path(const UnicodeString& fp);
UnicodeString get_volume_guid(const UnicodeString& volume_name);
