#pragma once

void error_dlg(const Error& e);
void error_dlg(const std::exception& e);
void info_dlg(const wstring& msg);
bool config_dialog(Options& options);
