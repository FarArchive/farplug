#pragma once

#include "plugin.hpp"

namespace Far {

void init(const PluginStartupInfo *psi);
wstring get_plugin_module_path();
const wchar_t* msg_ptr(int id);
wstring get_msg(int id);

unsigned get_optimal_msg_width();
int message(const wstring& msg, int button_cnt, DWORD flags);

};
