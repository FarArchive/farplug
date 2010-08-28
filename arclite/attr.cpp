#include "msg.h"
#include "error.hpp"
#include "common.hpp"
#include "comutils.hpp"
#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "ui.hpp"
#include "archive.hpp"

wstring uint_to_hex_str(unsigned __int64 val, unsigned num_digits = 0) {
  wchar_t str[16];
  unsigned pos = 16;
  do {
    unsigned d = static_cast<unsigned>(val % 16);
    pos--;
    str[pos] = d < 10 ? d + L'0' : d - 10 + L'A';
    val /= 16;
  }
  while (val);
  if (num_digits) {
    while (pos + num_digits > 16) {
      pos--;
      str[pos] = L'0';
    }
  }
  return wstring(str + pos, 16 - pos);
}

wstring format_str_prop(const PropVariant& prop) {
  wstring str = prop.get_str();
  for (unsigned i = 0; i < str.size(); i++)
    if (str[i] == L'\r' || str[i] == L'\n')
      str[i] = L' ';
  return str;
}

wstring format_int_prop(const PropVariant& prop) {
  wchar_t buf[32];
  errno_t res = _i64tow_s(prop.get_int(), buf, ARRAYSIZE(buf), 10);
  return buf;
}

wstring format_uint_prop(const PropVariant& prop) {
  wchar_t buf[32];
  errno_t res = _ui64tow_s(prop.get_uint(), buf, ARRAYSIZE(buf), 10);
  return buf;
}

wstring format_size_prop(const PropVariant& prop) {
  if (!prop.is_uint())
    return wstring();
  wstring short_size = format_data_size(prop.get_uint(), get_size_suffixes());
  wstring long_size = format_uint_prop(prop);
  if (short_size == long_size)
    return short_size;
  else
    return short_size + L"  " + long_size;
}

wstring format_filetime_prop(const PropVariant& prop) {
  if (!prop.is_filetime())
    return wstring();
  FILETIME local_file_time;
  SYSTEMTIME sys_time;
  if (!FileTimeToLocalFileTime(&prop.get_filetime(), &local_file_time))
    return wstring();
  if (!FileTimeToSystemTime(&local_file_time, &sys_time))
    return wstring();
  wchar_t buf[64];
  if (GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &sys_time, nullptr, buf, ARRAYSIZE(buf)) == 0)
    return wstring();
  wstring date_time(buf);
  if (GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &sys_time, nullptr, buf, ARRAYSIZE(buf)) == 0)
    return wstring();
  date_time = date_time + L" " + buf;
  return date_time;
}

wstring format_crc_prop(const PropVariant& prop) {
  if (!prop.is_uint())
    return wstring();
  return uint_to_hex_str(prop.get_uint(), prop.get_int_size() * 2);
}

wstring format_attrib_prop(const PropVariant& prop) {
  if (!prop.is_uint())
    return wstring();
  const wchar_t c_win_attr[] = L"RHS8DAdNTsrCOnE_";
  wchar_t attr[ARRAYSIZE(c_win_attr)];
  unsigned pos = 0;
  unsigned val = static_cast<unsigned>(prop.get_uint());
  for (unsigned i = 0; i < ARRAYSIZE(c_win_attr); i++) {
    if ((val & (1 << i)) && i != 7) {
      attr[pos] = c_win_attr[i];
      pos++;
    }
  }
  return wstring(attr, pos);
}

#define ATTR_CHAR(a, n, c) (((a) & (1 << (n))) ? c : L'-')
wstring format_posix_attrib_prop(const PropVariant& prop) {
  if (!prop.is_uint())
    return wstring();
  unsigned val = static_cast<unsigned>(prop.get_uint());
  wchar_t attr[10];
  attr[0] = ATTR_CHAR(val, 14, L'd');
  for (int i = 6; i >= 0; i -= 3)  {
    attr[7 - i] = ATTR_CHAR(val, i + 2, L'r');
    attr[8 - i] = ATTR_CHAR(val, i + 1, L'w');
    attr[9 - i] = ATTR_CHAR(val, i + 0, L'x');
  }
  wstring res(attr, ARRAYSIZE(attr));

  unsigned extra = val & 0x3E00;
  if (extra)
    res = uint_to_hex_str(extra, 4) + L' ' + res;
  return res;
}
#undef ATTR_CHAR


typedef wstring (*PropToString)(const PropVariant& var);

struct PropInfo {
  PROPID prop_id;
  unsigned name_id;
  PropToString prop_to_string;
};

static PropInfo c_prop_info[] =
{
  { kpidPath, MSG_KPID_PATH, nullptr },
  { kpidName, MSG_KPID_NAME, nullptr },
  { kpidExtension, MSG_KPID_EXTENSION, nullptr },
  { kpidIsDir, MSG_KPID_ISDIR, nullptr },
  { kpidSize, MSG_KPID_SIZE, format_size_prop },
  { kpidPackSize, MSG_KPID_PACKSIZE, format_size_prop },
  { kpidAttrib, MSG_KPID_ATTRIB, format_attrib_prop },
  { kpidCTime, MSG_KPID_CTIME, format_filetime_prop },
  { kpidATime, MSG_KPID_ATIME, format_filetime_prop },
  { kpidMTime, MSG_KPID_MTIME, format_filetime_prop },
  { kpidSolid, MSG_KPID_SOLID, nullptr },
  { kpidCommented, MSG_KPID_COMMENTED, nullptr },
  { kpidEncrypted, MSG_KPID_ENCRYPTED, nullptr },
  { kpidSplitBefore, MSG_KPID_SPLITBEFORE, nullptr },
  { kpidSplitAfter, MSG_KPID_SPLITAFTER, nullptr },
  { kpidDictionarySize, MSG_KPID_DICTIONARYSIZE, format_size_prop },
  { kpidCRC, MSG_KPID_CRC, format_crc_prop },
  { kpidType, MSG_KPID_TYPE, nullptr },
  { kpidIsAnti, MSG_KPID_ISANTI, nullptr },
  { kpidMethod, MSG_KPID_METHOD, nullptr },
  { kpidHostOS, MSG_KPID_HOSTOS, nullptr },
  { kpidFileSystem, MSG_KPID_FILESYSTEM, nullptr },
  { kpidUser, MSG_KPID_USER, nullptr },
  { kpidGroup, MSG_KPID_GROUP, nullptr },
  { kpidBlock, MSG_KPID_BLOCK, nullptr },
  { kpidComment, MSG_KPID_COMMENT, nullptr },
  { kpidPosition, MSG_KPID_POSITION, nullptr },
  { kpidPrefix, MSG_KPID_PREFIX, nullptr },
  { kpidNumSubDirs, MSG_KPID_NUMSUBDIRS, nullptr },
  { kpidNumSubFiles, MSG_KPID_NUMSUBFILES, nullptr },
  { kpidUnpackVer, MSG_KPID_UNPACKVER, nullptr },
  { kpidVolume, MSG_KPID_VOLUME, nullptr },
  { kpidIsVolume, MSG_KPID_ISVOLUME, nullptr },
  { kpidOffset, MSG_KPID_OFFSET, nullptr },
  { kpidLinks, MSG_KPID_LINKS, nullptr },
  { kpidNumBlocks, MSG_KPID_NUMBLOCKS, nullptr },
  { kpidNumVolumes, MSG_KPID_NUMVOLUMES, nullptr },
  { kpidTimeType, MSG_KPID_TIMETYPE, nullptr },
  { kpidBit64, MSG_KPID_BIT64, nullptr },
  { kpidBigEndian, MSG_KPID_BIGENDIAN, nullptr },
  { kpidCpu, MSG_KPID_CPU, nullptr },
  { kpidPhySize, MSG_KPID_PHYSIZE, format_size_prop },
  { kpidHeadersSize, MSG_KPID_HEADERSSIZE, format_size_prop },
  { kpidChecksum, MSG_KPID_CHECKSUM, nullptr },
  { kpidCharacts, MSG_KPID_CHARACTS, nullptr },
  { kpidVa, MSG_KPID_VA, nullptr },
  { kpidId, MSG_KPID_ID, nullptr },
  { kpidShortName, MSG_KPID_SHORTNAME, nullptr },
  { kpidCreatorApp, MSG_KPID_CREATORAPP, nullptr },
  { kpidSectorSize, MSG_KPID_SECTORSIZE, format_size_prop },
  { kpidPosixAttrib, MSG_KPID_POSIXATTRIB, format_posix_attrib_prop },
  { kpidLink, MSG_KPID_LINK, nullptr },
  { kpidTotalSize, MSG_KPID_TOTALSIZE, format_size_prop },
  { kpidFreeSpace, MSG_KPID_FREESPACE, format_size_prop },
  { kpidClusterSize, MSG_KPID_CLUSTERSIZE, format_size_prop },
  { kpidVolumeName, MSG_KPID_VOLUMENAME, nullptr },
  { kpidLocalName, MSG_KPID_LOCALNAME, nullptr },
  { kpidProvider, MSG_KPID_PROVIDER, nullptr },
};

const PropInfo* find_prop_info(PROPID prop_id) {
  for (unsigned i = 0; i < ARRAYSIZE(c_prop_info); i++) {
    if (c_prop_info[i].prop_id == prop_id)
      return c_prop_info + i;
  }
  return nullptr;
}

AttrList Archive::get_attr_list(UInt32 item_index) {
  AttrList attr_list;
  UInt32 num_props;
  CHECK_COM(in_arc->GetNumberOfProperties(&num_props));
  for (unsigned i = 0; i < num_props; i++) {
    Attr attr;
    BStr name;
    PROPID prop_id;
    VARTYPE vt;
    CHECK_COM(in_arc->GetPropertyInfo(i, name.ref(), &prop_id, &vt));
    const PropInfo* prop_info = find_prop_info(prop_id);
    if (prop_info)
      attr.name = Far::get_msg(prop_info->name_id);
    else if (name)
      attr.name.assign(name, name.size());
    else
      attr.name = int_to_str(prop_id);

    PropVariant prop;
    CHECK_COM(in_arc->GetProperty(item_index, prop_id, prop.ref()));

    if (prop_info && prop_info->prop_to_string) {
      attr.value = prop_info->prop_to_string(prop);
    }
    else {
      if (prop.is_str())
        attr.value = format_str_prop(prop);
      else if (prop.is_bool())
        attr.value = Far::get_msg(prop.get_bool() ? MSG_PROPERTY_TRUE : MSG_PROPERTY_FALSE);
      else if (prop.is_uint())
        attr.value = format_uint_prop(prop);
      else if (prop.is_int())
        attr.value = format_int_prop(prop);
      else if (prop.is_filetime())
        attr.value = format_filetime_prop(prop);
    }

    attr_list.push_back(attr);
  }

  return attr_list;
}

void Archive::load_arc_attr() {
  arc_attr.clear();
  UInt32 num_props;
  CHECK_COM(in_arc->GetNumberOfArchiveProperties(&num_props));
  for (unsigned i = 0; i < num_props; i++) {
    Attr attr;
    BStr name;
    PROPID prop_id;
    VARTYPE vt;
    CHECK_COM(in_arc->GetArchivePropertyInfo(i, name.ref(), &prop_id, &vt));
    const PropInfo* prop_info = find_prop_info(prop_id);
    if (prop_info)
      attr.name = Far::get_msg(prop_info->name_id);
    else if (name)
      attr.name.assign(name, name.size());
    else
      attr.name = int_to_str(prop_id);

    PropVariant prop;
    CHECK_COM(in_arc->GetArchiveProperty(prop_id, prop.ref()));

    if (prop_info && prop_info->prop_to_string) {
      attr.value = prop_info->prop_to_string(prop);
    }
    else {
      if (prop.is_str())
        attr.value = format_str_prop(prop);
      else if (prop.is_bool())
        attr.value = Far::get_msg(prop.get_bool() ? MSG_PROPERTY_TRUE : MSG_PROPERTY_FALSE);
      else if (prop.is_uint())
        attr.value = format_uint_prop(prop);
      else if (prop.is_int())
        attr.value = format_int_prop(prop);
      else if (prop.is_filetime())
        attr.value = format_filetime_prop(prop);
    }

    arc_attr.push_back(attr);
  }
}
