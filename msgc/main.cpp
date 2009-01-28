#include <stdio.h>

#include "col/AnsiString.h"
#include "col/PlainArray.h"
#include "col/ObjectArray.h"
using namespace col;

#define _ERROR_STDIO
#include "error.h"

struct MsgPair {
  AnsiString id;
  AnsiString phrase;
  bool operator==(const MsgPair& lp) const {
    return id == lp.id;
  }
  bool operator>(const MsgPair& lp) const {
    return id > lp.id;
  }
  bool operator<(const MsgPair& lp) const {
    return id < lp.id;
  }
};

long get_file_size(FILE* file) {
  long curr_pos = ftell(file);
  CHECK_STD(curr_pos != -1);
  CHECK_STD(fseek(file, 0, SEEK_END) == 0);
  long size = ftell(file);
  CHECK_STD(fseek(file, curr_pos, SEEK_SET) == 0);
  return size;
}

struct MsgFile {
  ObjectArray<MsgPair> list;
  AnsiString lang_tag;
};

#define PARSE_ERROR FAIL(MsgError(AnsiString::format("Parse error at %s:%u", file_path.data(), line_cnt + 1)))
MsgFile load_msg_file(const AnsiString& file_path) {
  MsgFile msg_file;
  msg_file.list.set_inc(20);
  FILE* file = fopen(file_path.data(), "rb");
  CHECK_STD(file != NULL);
  try {
    AnsiString file_data;
    long file_size = get_file_size(file);
    fread(file_data.buf(file_size), sizeof(char), file_size, file);
    CHECK_STD(ferror(file) == 0);
    file_data.set_size(file_size);

    MsgPair lp;
    unsigned line_cnt = 0;
    unsigned pos_start = 0;
    while (pos_start < file_data.size()) {
      unsigned pos_end = file_data.search(pos_start, '\n');
      if (pos_end == -1) pos_end = file_data.size();
      if (file_data[pos_start] == '#') {
        // skip comment
      }
      else if (file_data.slice(pos_start, pos_end - pos_start).strip().size() == 0) {
        // skip empty line
      }
      else {
        unsigned pos_sep = file_data.search(pos_start, '=');
        if ((pos_sep == -1) || (pos_sep > pos_end)) PARSE_ERROR;
        lp.id = file_data.slice(pos_start, pos_sep - pos_start).strip();
        if (line_cnt == 0) {
          // first line must be language tag
          if (lp.id != ".Language") PARSE_ERROR;
          msg_file.lang_tag = file_data.slice(pos_start, pos_end - pos_start).strip();
        }
        else {
          // convert id
          for (unsigned i = 0; i < lp.id.size(); i++) {
            if (((lp.id[i] >= 'A') && (lp.id[i] <= 'Z')) || ((lp.id[i] >= '0') && (lp.id[i] <= '9')) || (lp.id[i] == '_')) {
            }
            else if ((lp.id[i] >= 'a') && (lp.id[i] <= 'z')) {
              lp.id.item(i) = lp.id[i] - 'a' + 'A';
            }
            else if (lp.id[i] == '.') {
              lp.id.item(i) = '_';
            }
            else PARSE_ERROR;
          }
          lp.id.insert(0, "MSG_");
          lp.phrase = '"' + file_data.slice(pos_sep + 1, pos_end - pos_sep - 1).strip() + '"';
          msg_file.list += lp;
        }
      }
      pos_start = pos_end + 1;
      line_cnt++;
    }
  }
  finally (fclose(file));
  msg_file.list.sort();
  return msg_file;
}

void save_file(const AnsiString& file_name, const AnsiString& file_data) {
  FILE* file = fopen(file_name.data(), "wb");
  CHECK_STD(file != NULL);
  try {
    fwrite(file_data.data(), sizeof(char), file_data.size(), file);
    CHECK_STD(ferror(file) == 0);
  }
  finally (fclose(file));
}

struct FileNamePair {
  AnsiString in;
  AnsiString out;
};

#define CMD_ERROR FAIL(MsgError("Usage: msgc -in msg_file [msg_file2 ...] -out header_file lng_file [lng_file2 ...]"))
void parse_cmd_line(const ObjectArray<AnsiString>& params, ObjectArray<FileNamePair>& files, AnsiString& header_file) {
  if (params.size() == 0) CMD_ERROR;
  unsigned idx = 0;
  if (params[idx] != "-in") CMD_ERROR;
  idx++;
  files.clear();
  FileNamePair fnp;
  while ((idx < params.size()) && (params[idx] != "-out")) {
    fnp.in = params[idx];
    files += fnp;
    idx++;
  }
  if (files.size() == 0) CMD_ERROR;
  if (idx == params.size()) CMD_ERROR;
  idx++;
  if (idx == params.size()) CMD_ERROR;
  header_file = params[idx];
  idx++;
  for (unsigned i = 0; i < files.size(); i++) {
    if (idx == params.size()) CMD_ERROR;
    files.item(i).out = params[idx];
    idx++;
  }
}

int main(int argc, char* argv[]) {
  try {
    ObjectArray<AnsiString> params;
    for (int i = 1; i < argc; i++) {
      params += argv[i];
    }
    ObjectArray<FileNamePair> files;
    AnsiString header_file;
    parse_cmd_line(params, files, header_file);
    // load message files
    ObjectArray<MsgFile> msgs;
    for (unsigned i = 0; i < files.size(); i++) {
      msgs += load_msg_file(files[i].in);
      if (i != 0) {
        if (msgs[i].list != msgs[i - 1].list) FAIL(MsgError(AnsiString::format("Message files '%s' and '%s' do not match", files[i].in.data(), files[i - 1].in.data())));
      }
    }
    // create header file
    AnsiString header_data;
    for (unsigned i = 0; i < msgs[0].list.size(); i++) {
      header_data.add_fmt("#define %S %u\n", &msgs[0].list[i].id, i);
    }
    save_file(header_file, header_data);
    // create Far language files
    AnsiString lng_data;
    for (unsigned i = 0; i < msgs.size(); i++) {
      lng_data = msgs[i].lang_tag + '\n';
      for (unsigned j = 0; j < msgs[i].list.size(); j++) {
        lng_data += msgs[i].list[j].phrase + '\n';
      }
      save_file(files[i].out, lng_data);
    }
    return 0;
  }
  catch (Error& e) {
    puts(AnsiString::format("File: %s Line: %u: %S", e.file, e.line, &e.message()).data());
    return 1;
  }
}
