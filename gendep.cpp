#include <windows.h>

#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

#include <assert.h>

#include "error.hpp"
#include "utils.hpp"
#include "sysutils.hpp"

#include "strutils.cpp"
#include "pathutils.cpp"
#include "sysutils.cpp"

const wchar_t* c_ext_list[] = {
  L"c", L"h", L"cpp", L"hpp", L"rc",
};

bool is_valid_ext(const wchar_t* file_name) {
  const wchar_t* ext = wcsrchr(file_name, L'.');
  if (ext) ext++;
  else ext = L"";
  for (unsigned i = 0; i < ARRAYSIZE(c_ext_list); i++) {
    if (_wcsicmp(ext, c_ext_list[i]) == 0)
      return true;
  }
  return false;
}

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

class Parser {
private:
  const wstring& text;
  size_t pos;
  bool is_one_of(wchar_t ch, const wchar_t* char_set) {
    const wchar_t* c = char_set;
    while (*c && *c != ch) c++;
    return *c == ch;
  }
  bool end() const {
    return pos == text.size();
  }
public:
  Parser(const wstring& text, size_t pos): text(text), pos(pos) {
  }
  void ws() {
    while (!end() && is_one_of(text[pos], L" \t"))
      pos++;
  }
  bool ch(wchar_t c) {
    if (end() || text[pos] != c) return false;
    pos++;
    return true;
  }
  bool str(const wchar_t* str) {
    while (!end() && *str && *str == text[pos]) {
      pos++;
      str++;
    }
    return *str == 0;
  }
  wstring extract(const wchar_t* end_chars) {
    size_t start = pos;
    while (!end() && !is_one_of(text[pos], end_chars))
      pos++;
    return wstring(text.data() + start, pos - start);
  }
};

bool is_include_directive(const wstring& text, size_t pos, wstring& file_name) {
  Parser parser(text, pos);
  parser.ws();
  if (!parser.ch(L'#')) return false;
  parser.ws();
  if (!parser.str(L"include")) return false;
  parser.ws();
  if (!parser.ch(L'"')) return false;
  file_name = parser.extract(L"\"\n");
  if (!parser.ch(L'"')) return false;
  return true;
}

bool file_exists(const wstring& file_path) {
  return GetFileAttributesW(long_path(get_full_path_name(file_path)).c_str()) != INVALID_FILE_ATTRIBUTES;
}

list<wstring> get_file_list(const list<wstring>& include_dirs, const wstring& text) {
  list<wstring> file_list;
  wstring file_name;
  size_t pos = 0;
  while (true) {
    if (is_include_directive(text, pos, file_name)) {
      bool found = false;
      for (list<wstring>::const_iterator inc_dir = include_dirs.begin(); !found && inc_dir != include_dirs.end(); inc_dir++) {
        wstring rel_path = add_trailing_slash(*inc_dir) + file_name;
        if (file_exists(rel_path)) {
          file_list.push_back(rel_path);
          found = true;
        }
      }
      if (!found) FAIL_MSG(L"Header \"" + file_name + L"\" not found");
    }
    pos = text.find(L'\n', pos);
    if (pos == wstring::npos) break;
    else pos++;
  }
  return file_list;
}

#define CHECK_CMD(code) if (!(code)) FAIL_MSG(L"Usage: gendep [-I<include> ...]")
void parse_cmd_line(const list<wstring>& params, list<wstring>& include_dirs) {
  include_dirs.clear();
  for (list<wstring>::const_iterator param = params.begin(); param != params.end(); param++) {
    CHECK_CMD(substr_match(*param, 0, L"-I"));
    wstring inc_dir = param->substr(2);
    CHECK_CMD(!inc_dir.empty());
    include_dirs.push_back(inc_dir);
  }
}

int wmain(int argc, wchar_t* argv[]) {
  try {
    list<wstring> params;
    for (unsigned i = 1; i < argc; i++) params.push_back(argv[i]);
    list<wstring> include_dirs;
    parse_cmd_line(params, include_dirs);
    include_dirs.push_front(wstring());
    wstring output;
    wstring cur_dir = get_current_directory();
    FindFile files(cur_dir);
    WIN32_FIND_DATAW find_data;
    while (files.next(find_data)) {
      if (is_valid_ext(find_data.cFileName)) {
        wstring text = load_file(add_trailing_slash(cur_dir) + find_data.cFileName);
        list<wstring> include_file_list = get_file_list(include_dirs, text);
        if (include_file_list.size()) {
          output.append(find_data.cFileName).append(1, L':');
          for (list<wstring>::const_iterator fn = include_file_list.begin(); fn != include_file_list.end(); fn++) {
            output.append(1, L' ').append(*fn);
          }
          output.append(1, L'\n');
        }
      }
    }
    cout << unicode_to_ansi(output, CP_ACP);
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
