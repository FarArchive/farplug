#include <windows.h>

#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

#include "error.hpp"
#include "utils.hpp"
#include "sysutils.hpp"

#include "strutils.cpp"
#include "pathutils.cpp"
#include "sysutils.cpp"

struct MsgPair {
  wstring id;
  wstring phrase;
  bool operator==(const MsgPair& lp) const {
    return id == lp.id;
  }
  bool operator<(const MsgPair& lp) const {
    return id < lp.id;
  }
};

struct MsgFile {
  deque<MsgPair> msg_list;
  wstring lang_tag;
};

wstring load_file(const wstring& file_name) {
  File file(get_full_path_name(file_name), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
  Buffer<char> buffer(file.size());
  unsigned size = file.read(buffer);
  return ansi_to_unicode(string(buffer.data(), size), CP_ACP);
}

void save_file(const wstring& file_name, const wstring& text) {
  File file(get_full_path_name(file_name), GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS, 0);
  string data = unicode_to_ansi(text, CP_ACP);
  file.write(data.data(), data.size());
}

#define CHECK_PARSE(code) if (!(code)) FAIL_MSG(L"Parse error at " + file_path + L":" + int_to_str(line_cnt + 1))
MsgFile load_msg_file(const wstring& file_path) {
  MsgFile msg_file;
  wstring text = load_file(file_path);
  MsgPair mp;
  unsigned line_cnt = 0;
  size_t pos_start = 0;
  while (pos_start < text.size()) {
    size_t pos_end = text.find(L'\n', pos_start);
    if (pos_end == string::npos) pos_end = text.size();
    if (text[pos_start] == L'#') {
      // skip comment
    }
    else if (strip(text.substr(pos_start, pos_end - pos_start)).size() == 0) {
      // skip empty line
    }
    else {
      size_t pos_sep = text.find(L'=', pos_start);
      CHECK_PARSE((pos_sep != string::npos) && (pos_sep <= pos_end));
      mp.id = strip(text.substr(pos_start, pos_sep - pos_start));
      if (line_cnt == 0) {
        // first line must be language tag
        CHECK_PARSE(mp.id == L".Language");
        msg_file.lang_tag = strip(text.substr(pos_start, pos_end - pos_start));
      }
      else {
        // convert id
        for (unsigned i = 0; i < mp.id.size(); i++) {
          if (((mp.id[i] >= L'A') && (mp.id[i] <= L'Z')) || ((mp.id[i] >= L'0') && (mp.id[i] <= L'9')) || (mp.id[i] == L'_')) {
          }
          else if ((mp.id[i] >= L'a') && (mp.id[i] <= L'z')) {
            mp.id[i] = mp.id[i] - L'a' + L'A';
          }
          else if (mp.id[i] == L'.') {
            mp.id[i] = L'_';
          }
          else {
            CHECK_PARSE(false);
          }
        }
        mp.id.insert(0, L"MSG_");
        mp.phrase = L'"' + strip(text.substr(pos_sep + 1, pos_end - pos_sep - 1)) + L'"';
        msg_file.msg_list.push_back(mp);
      }
    }
    pos_start = pos_end + 1;
    line_cnt++;
  }

  sort(msg_file.msg_list.begin(), msg_file.msg_list.end());
  return msg_file;
}

struct FileNamePair {
  wstring in;
  wstring out;
};

#define CHECK_CMD(code) if (!(code)) FAIL_MSG(L"Usage: msgc -in msg_file [msg_file2 ...] -out header_file lng_file [lng_file2 ...]")
void parse_cmd_line(const deque<wstring>& params, deque<FileNamePair>& files, wstring& header_file) {
  CHECK_CMD(params.size());
  unsigned idx = 0;
  CHECK_CMD(params[idx] == L"-in");
  idx++;
  files.clear();
  FileNamePair fnp;
  while ((idx < params.size()) && (params[idx] != L"-out")) {
    fnp.in = params[idx];
    files.push_back(fnp);
    idx++;
  }
  CHECK_CMD(files.size());
  CHECK_CMD(idx != params.size());
  idx++;
  CHECK_CMD(idx != params.size());
  header_file = params[idx];
  idx++;
  for (unsigned i = 0; i < files.size(); i++) {
    CHECK_CMD(idx != params.size());
    files[i].out = params[idx];
    idx++;
  }
}

int wmain(int argc, wchar_t* argv[]) {
  try {
    deque<wstring> params;
    for (int i = 1; i < argc; i++) {
      params.push_back(argv[i]);
    }
    deque<FileNamePair> files;
    wstring header_file;
    parse_cmd_line(params, files, header_file);
    // load message files
    deque<MsgFile> msgs;
    for (unsigned i = 0; i < files.size(); i++) {
      msgs.push_back(load_msg_file(files[i].in));
      if (i) {
        if (msgs[i].msg_list != msgs[i - 1].msg_list) FAIL_MSG(L"Message files '" + files[i].in + L"' and '" + files[i - 1].in + L"' do not match");
      }
    }
    // create header file
    wstring header_data;
    for (unsigned i = 0; i < msgs[0].msg_list.size(); i++) {
      header_data.append(L"#define " + msgs[0].msg_list[i].id + L" " + int_to_str(i) + L"\n");
    }
    save_file(header_file, header_data);
    // create Far language files
    wstring lng_data;
    for (unsigned i = 0; i < msgs.size(); i++) {
      lng_data = msgs[i].lang_tag + L'\n';
      for (unsigned j = 0; j < msgs[i].msg_list.size(); j++) {
        lng_data += msgs[i].msg_list[j].phrase + L'\n';
      }
      save_file(files[i].out, lng_data);
    }
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
