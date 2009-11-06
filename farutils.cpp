#include <string>
using namespace std;

#include "utils.hpp"
#include "farutils.hpp"

namespace Far {

PluginStartupInfo g_far;
FarStandardFunctions g_fsf;

void init(const PluginStartupInfo *psi) {
  g_far = *psi;
  g_fsf = *psi->FSF;
}

wstring get_plugin_path() {
  return extract_file_path(g_far.ModuleName);
}

const wchar_t* msg_ptr(int id) {
  return g_far.GetMsg(g_far.ModuleNumber, id);
}

};
