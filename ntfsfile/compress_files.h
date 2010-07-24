#pragma once

void plugin_compress_files(const ObjectArray<UnicodeString>& file_list, const CompressFilesParams& params, Log& log);
bool show_compress_files_dialog(CompressFilesParams& params);
