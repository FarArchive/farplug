#include "msg.h"
#include "error.hpp"
#include "common_types.hpp"
#include "comutils.hpp"
#include "utils.hpp"
#include "farutils.hpp"
#include "sysutils.hpp"
#include "ui.hpp"
#include "archive.hpp"

typedef wstring (*PropToString)(const PROPVARIANT& var);

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
  { kpidSize, MSG_KPID_SIZE, nullptr },
  { kpidPackSize, MSG_KPID_PACKSIZE, nullptr },
  { kpidAttrib, MSG_KPID_ATTRIB, nullptr },
  { kpidCTime, MSG_KPID_CTIME, nullptr },
  { kpidATime, MSG_KPID_ATIME, nullptr },
  { kpidMTime, MSG_KPID_MTIME, nullptr },
  { kpidSolid, MSG_KPID_SOLID, nullptr },
  { kpidCommented, MSG_KPID_COMMENTED, nullptr },
  { kpidEncrypted, MSG_KPID_ENCRYPTED, nullptr },
  { kpidSplitBefore, MSG_KPID_SPLITBEFORE, nullptr },
  { kpidSplitAfter, MSG_KPID_SPLITAFTER, nullptr },
  { kpidDictionarySize, MSG_KPID_DICTIONARYSIZE, nullptr },
  { kpidCRC, MSG_KPID_CRC, nullptr },
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
  { kpidPhySize, MSG_KPID_PHYSIZE, nullptr },
  { kpidHeadersSize, MSG_KPID_HEADERSSIZE, nullptr },
  { kpidChecksum, MSG_KPID_CHECKSUM, nullptr },
  { kpidCharacts, MSG_KPID_CHARACTS, nullptr },
  { kpidVa, MSG_KPID_VA, nullptr },
  { kpidId, MSG_KPID_ID, nullptr },
  { kpidShortName, MSG_KPID_SHORTNAME, nullptr },
  { kpidCreatorApp, MSG_KPID_CREATORAPP, nullptr },
  { kpidSectorSize, MSG_KPID_SECTORSIZE, nullptr },
  { kpidPosixAttrib, MSG_KPID_POSIXATTRIB, nullptr },
  { kpidLink, MSG_KPID_LINK, nullptr },
  { kpidTotalSize, MSG_KPID_TOTALSIZE, nullptr },
  { kpidFreeSpace, MSG_KPID_FREESPACE, nullptr },
  { kpidClusterSize, MSG_KPID_CLUSTERSIZE, nullptr },
  { kpidVolumeName, MSG_KPID_VOLUMENAME, nullptr },
  { kpidLocalName, MSG_KPID_LOCALNAME, nullptr },
  { kpidProvider, MSG_KPID_PROVIDER, nullptr },
};

const PropInfo* fund_prop_info(PROPID prop_id) {
  for (unsigned i = 0; i < ARRAYSIZE(c_prop_info); i++) {
    if (c_prop_info[i].prop_id == prop_id)
      return c_prop_info + i;
  }
  return nullptr;
}

wstring uint_to_str(unsigned __int64 value) {
  wchar_t str[64];
  _ui64tow_s(value, str, ARRAYSIZE(str), 10);
  return str;
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
    const PropInfo* prop_info = fund_prop_info(prop_id);
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
      if (prop.vt == VT_BSTR)
        attr.value = prop.bstrVal;
      else if (prop.vt == VT_BOOL)
        attr.value = Far::get_msg(prop.boolVal == VARIANT_TRUE ? MSG_PROPERTY_TRUE : MSG_PROPERTY_FALSE);
      else if (prop.vt == VT_UI4)
        attr.value = uint_to_str(prop.ulVal);
      else if (prop.vt == VT_UI8)
        attr.value = uint_to_str(prop.uhVal.QuadPart);
    }

    attr_list.push_back(attr);
  }

  return attr_list;
}
