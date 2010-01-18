#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

#include "error.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "iniparse.hpp"

#include "strutils.cpp"
#include "pathutils.cpp"
#include "sysutils.cpp"
#include "iniparse.cpp"

#define CP_UTF16 1200
wstring load_file(const wstring& file_name, unsigned* code_page = NULL) {
  File file(get_full_path_name(file_name), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  Buffer<char> buffer(file.size());
  unsigned size = file.read(buffer);
  if ((size >= 2) && (buffer.data()[0] == '\xFF') && (buffer.data()[1] == '\xFE')) {
    if (code_page) *code_page = CP_UTF16;
    return wstring(reinterpret_cast<wchar_t*>(buffer.data() + 2), (buffer.size() - 2) / 2);
  }
  else if ((size >= 3) && (buffer.data()[0] == '\xEF') && (buffer.data()[1] == '\xBB') && (buffer.data()[2] == '\xBF')) {
    if (code_page) *code_page = CP_UTF8;
    return ansi_to_unicode(string(buffer.data() + 3, size - 3), CP_UTF8);
  }
  else {
    if (code_page) *code_page = CP_ACP;
    return ansi_to_unicode(string(buffer.data(), size), CP_ACP);
  }
}

void save_file(const wstring& file_name, const wstring& text, unsigned code_page = 0) {
  File file(get_full_path_name(file_name), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
  if (code_page == CP_UTF16) {
    const char c_sig[] = { '\xFF', '\xFE' };
    file.write(c_sig, sizeof(c_sig));
    file.write(reinterpret_cast<const char*>(text.data()), text.size() * 2);
  }
  else {
    if (code_page == CP_UTF8) {
      const char c_sig[] = { '\xEF', '\xBB', '\xBF' };
      file.write(c_sig, sizeof(c_sig));
    }
    string data = unicode_to_ansi(text, code_page);
    file.write(data.data(), data.size());
  }
}

void search_and_replace(wstring& text, const wstring& name, const wstring& value) {
  size_t pos = 0;
  while (true) {
    pos = text.find(name, pos);
    if (pos == wstring::npos) break;
    text.replace(pos, name.size(), value);
    pos += value.size();
  }
}

const wchar_t* prefix = L"<(";
const wchar_t* suffix = L")>";

int wmain(int argc, wchar_t* argv[]) {
  try {
    if (argc != 4)
      FAIL_MSG(L"Usage: preproc ini_file in_file out_file");
    unsigned code_page;
    wstring text = load_file(argv[2], &code_page);
    Ini::File ini_file;
    ini_file.parse(load_file(argv[1]));
    for (Ini::File::const_iterator section = ini_file.begin(); section != ini_file.end(); section++) {
      for (Ini::Section::const_iterator item = section->second.begin(); item != section->second.end(); item++) {
        search_and_replace(text, prefix + item->first + suffix, item->second);
      }
    }
    save_file(argv[3], text, code_page);
    return 0;
  }
  catch (const Error& e) {
    if (e.code != NO_ERROR) {
      wstring sys_msg = get_system_message(e.code);
      if (!sys_msg.empty())
        wcerr << sys_msg << endl;
    }
    for (list<wstring>::const_iterator message = e.messages.begin(); message != e.messages.end(); message++) {
      wcerr << *message << endl;
    }
    wcerr << extract_file_name(widen(e.file)) << L':' << e.line << endl;
  }
  catch (const exception& e) {
    cerr << typeid(e).name() << ": " << e.what() << endl;
  }
  catch (...) {
    cerr << "unknown error" << endl;
  }
  return 1;
}
