#pragma once

#include "plugin.hpp"

namespace Far {

void init(const PluginStartupInfo *psi);
wstring get_plugin_path();
const wchar_t* msg_ptr(int id);

};
