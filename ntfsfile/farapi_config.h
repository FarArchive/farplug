// configure for target Far API version

#if !defined(FARAPI17) && !defined(FARAPI18)
#  error FAR API version is not defined.
#endif

#ifdef FARAPI17
#  define FAR_T(str) str
#  define FarCh char
#  define FarStr AnsiString
#  define FARSTR_TO_UNICODE(str) oem_to_unicode(str)
#  define UNICODE_TO_FARSTR(str) unicode_to_oem(str)
#  define FAR_EXPORT(name) name
#  define FAR_FILE_NAME(find_data) (find_data).cFileName
#  define FAR_SHORT_FILE_NAME(find_data) (find_data).cAlternateFileName
#  define FAR_CUR_DIR(panel_info) (panel_info).CurDir
#  define FAR_DECODE_PATH(file_name) decode_fn(oem_to_unicode(file_name))
#  define FAR_FILE_SIZE(find_data) ((((unsigned __int64) (find_data).nFileSizeHigh) << 32) + (find_data).nFileSizeLow)
#  define FAR_STRLEN strlen
#  define FAR_STRCMP strcmp
#  define FAR_STRCPY strcpy
#  define PANEL_PASSIVE INVALID_HANDLE_VALUE
#  define FAR_SELECTED_ITEM(sel_item) (sel_item)
#  define FAR_FREE_PANEL_INFO(plugin, panel_info)
#endif // FARAPI17

#ifdef FARAPI18
#  define FAR_T(str) L##str
#  define FarCh wchar_t
#  define FarStr UnicodeString
#  define FARSTR_TO_UNICODE(str) (str)
#  define UNICODE_TO_FARSTR(str) (str)
#  define FAR_EXPORT(name) name##W
#  define FAR_FILE_NAME(find_data) (find_data).lpwszFileName
#  define FAR_SHORT_FILE_NAME(find_data) (find_data).lpwszAlternateFileName
#  define FAR_CUR_DIR(panel_info) (panel_info).lpwszCurDir
#  define FAR_DECODE_PATH(file_name) (file_name)
#  define FAR_FILE_SIZE(find_data) ((find_data).nFileSize)
#  define FAR_STRLEN wcslen
#  define FAR_STRCMP wcscmp
#  define FAR_STRCPY wcscpy
#  define FCTL_UPDATEANOTHERPANEL FCTL_UPDATEPANEL
#  define FCTL_REDRAWANOTHERPANEL FCTL_REDRAWPANEL
#  define FAR_SELECTED_ITEM(sel_item) (*(sel_item))
#  define FAR_FREE_PANEL_INFO(plugin, panel_info) g_far.Control(plugin, FCTL_FREEPANELINFO, &panel_info)
#endif // FARAPI18
