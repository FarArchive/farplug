#pragma once

#define E_BAD_FORMAT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_INTERNAL, 1)

enum CommandType {
  cmdOpen,
  cmdCreate,
  cmdExtract,
  cmdTest,
};

struct CommandArgs {
  CommandType cmd;
  vector<wstring> args;
};

CommandArgs parse_command(const wstring& cmd_text);

struct OpenCommand {
  wstring arc_path;
  bool detect;
  OpenCommand(): detect(false) {
  }
};

OpenCommand parse_open_command(const vector<wstring>& args);

struct CreateCommand {
  UpdateOptions options;
  vector<wstring> files;
  vector<wstring> listfiles;
};

CreateCommand parse_create_command(const vector<wstring>& args);

struct ExtractCommand {
  ExtractOptions options;
  vector<wstring> arc_list;
};

ExtractCommand parse_extract_command(const vector<wstring>& args);

struct TestCommand {
  vector<wstring> arc_list;
};

TestCommand parse_test_command(const vector<wstring>& args);
