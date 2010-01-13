m4_include(`version.m4')m4_dnl
.Language=English,English
.PluginContents=MsiUpdate

@Contents
$^#MsiUpdate __VER_MAJOR__.__VER_MINOR__.__VER_PATCH__#

    Plugin notifies about new versions of Far Manager and performs automatic updates using official MSI packages.
    ~Updates~@http://forum.farmanager.com/viewtopic.php?t=4843@
    ~Duscussion~@http://forum.farmanager.com/viewtopic.php?t=4844@

@config
$^#MsiUpdate __VER_MAJOR__.__VER_MINOR__.__VER_PATCH__#
$ #Configuration#

    #Use full install UI# - full UI installation mode will let you review and select Far components.
    #Update stable builds# - only stable builds are checked for updates.
    #Log installation process# - save installation log file (MsiUpdate_2_x86.log) into system temporary directory.
    #Additional parameters# - space separated values passed to installer. Examples:
    LAUNCHAPP=1 - start Far after automatic installation is complete.
    MSIFASTINSTALL=1 - do not create system restore points during installation (Windows 7 or higher).
    #Cache install packages locally# - installation packages will be saved into a local directory.
    #Max. number of packages# - maximum number of packages to save locally. Old packages are deleted when this limit is exceeded.
    #Cache directory# - directory name for installation package storage. It can contain environment variables. Examle: %FARHOME%\msicache

@update
$^#MsiUpdate __VER_MAJOR__.__VER_MINOR__.__VER_PATCH__#
$ #Update dialog#

    This dialog is shown when new version is available. #View changelog# button shows changes done between current and new version.
#Yes# button starts update process. Plugin tries to close Far when update is started. #No# button cancels update and forbids
future automatic notifications about this specific version. #Cancel# just closes update dialog.
