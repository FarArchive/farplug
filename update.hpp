#pragma once

namespace Updater {

void initialize();
void finalize();
wstring get_update_url();
bool check(const string& update_info);
void execute();

}
