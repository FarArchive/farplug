m4_include(`version.m4')m4_dnl
.Language=English,English
.PluginContents=NTFS File Information

@Contents
$^#NTFS File Information __VER_MAJOR__.__VER_MINOR__.__VER_PATCH__#

This plugin provides various information about files on NTFS file system. Features:

1. Determine file size on disk. Especially useful for compressed and sparse files. Total size is shown for multiple files / directories.

2. List NTFS attributes (file components on NTFS file system). Can be used to get information about:
  - alternate file names (hard links)
  - alternate data streams (named DATA attribute)
  - symbolic link targets

3. Analyze file fragmentation and perform defragmentation.

4. Analyze file contents:
  - estimate if compression of file data is possible (using very FAST LZO algorithm)
  - calculate most useful file hashes: crc32, md5, sha1, ed2k (eMule variation).

@metadata
$ #File information#
    Displays NTFS file attribute (file component) table.

Basic attributes:
    #FILE_NAME# - file name (short, long or hard link).
Full path is shown for every hard link.
    #DATA# - data stream. Name is shown for alternate data streams.
You can use <file name>:<stream name> syntax to access data streams.
    #REPARSE_POINT# - reparse point (directory junction, mount point, symbolic link).
Target path is shown in Name/Data column.

Attribute information:
    #RXCES# - flags. R - resident attribute (this attribute is located entirely inside MFT).
X - attribute uses more than one MFT record.
CES - attribute is compressed / encrypted / sparse.
    #Data Size# - amount of data stored in this attribute. Hard links are counted separately.
    #Disk Size# - amount of disk space consumed by attribute.
    #Fragments# - number of fragments (continuous cluster chains). 1 means no fragmentation.
Total number of excess fragments is shown in the summary (empty value means file is not fragmented).
    #Name/Data# - attribute specific data like file name, symbolic link target, etc.

Summary is shown for directories and multiple files:
    number of files and directories processed, symbolic and hard links (+rp and +hl)
    #Unnamed DATA Total# - files totals.
    #Named DATA Total# - alternate streams totals.
    #Non-Resident Total# - non-resident attributes totals.

@file_panel_mode
$ #File panel mode#
~Customizing file panel view modes~@<..\..\>PanelViewModes@

Custom columns:
    #DSZ# - file data size i.e. size of DATA attribute(s). For attributes - attribute data size.
    #RSZ# - disk space consumed by file.
    #FRG# - number of excess file fragments.
    #STM# - number of data streams (DATA attributes).
    #LNK# - number of hard links (alternative file names).
    #MFT# - number of MFT records consumed by file.
    #VSZ# - valid size of DATA attribute(s). For attributes - attribute valid size.

    Sorting option #Sort by:# is applyed when 'Unsorted' mode (Ctrl+F7) is selected in FAR.
    #Access volume contents via MFT# - enables alternative way of getting file lists. All information is read
from MFT instead of using traditional directory listing methods.
    #Use highlighting# - enables file highlighting. Disable to speed up processing of large file lists.

@plugin_menu
$ #Plugin menu#

    #Flat mode# - enables simultaneous display of all files found in current directory and its subdirectories.
    #MFT index# - enables alternative way of getting file lists. All information is read
from MFT instead of using traditional directory listing methods.
