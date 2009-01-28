#include <stdio.h>

#include "col/AnsiString.h"
using namespace col;

#include "error.h"

bool str_to_uint(const AnsiString& str, unsigned& uint) {
  uint = 0;
  for (unsigned i = 0; i < str.size(); i++) {
    if ((str[i] >= '0') && (str[i] <= '9')) {
      uint = uint * 10 + str[i] - '0';
    }
    else return false;
  }
  return true;
}

AnsiString uint_to_str(unsigned uint) {
  AnsiString str;
  do {
    str.insert(0, (uint % 10) + '0');
    uint /= 10;
  }
  while (uint != 0);
  return str;
}

struct RevInfo {
  bool modified;
  bool switched;
  bool mixed;
  union {
    unsigned revision;
    struct {
      unsigned min_rev;
      unsigned max_rev;
    };
  };
};

RevInfo parse_rev_info(const AnsiString& input) {
  RevInfo rev_info;

  if (input.size() == 0) FAIL(MsgError("Empty revision info"));

  if (input == "exported") FAIL(MsgError("Not a working copy"));

  unsigned n = 0;
  rev_info.modified = (input[input.size() - 1] == 'M');
  if (rev_info.modified) n++;
  rev_info.switched = (input[input.size() - n - 1] == 'S');
  if (rev_info.switched) n++;

  unsigned colon_idx = input.search(':');
  rev_info.mixed = (colon_idx != -1);
  bool valid = true;
  if (rev_info.mixed) {
    valid = valid && str_to_uint(input.left(colon_idx), rev_info.min_rev);
    valid = valid && str_to_uint(input.slice(colon_idx + 1, input.size() - n - colon_idx - 1), rev_info.max_rev);
  }
  else {
    valid = valid && str_to_uint(input.left(input.size() - n), rev_info.revision);
  }
  if (!valid) FAIL(MsgError("Invalid revision info"));
  return rev_info;
}

AnsiString read_rev_str() {
  char in_buf[64];
  unsigned in_cnt = fread(in_buf, 1, sizeof(in_buf), stdin);
  if (in_cnt == 0) FAIL(MsgError("No input detected"));
  if (in_cnt == sizeof(in_buf)) FAIL(MsgError("Input is too long"));
  return AnsiString(in_buf, in_cnt);
}

AnsiString load_rev_str(const AnsiString& file_name) {
  char in_buf[64];
  unsigned in_cnt;
  FILE* f = fopen(file_name.data(), "rb");
  if (f == NULL) FAIL(MsgError(strerror(errno)));
  try {
    in_cnt = fread(in_buf, 1, sizeof(in_buf), f);
    if ((in_cnt == 0) || (in_cnt == sizeof(in_buf))) FAIL(MsgError("Invalid revision info"));
  }
  finally (fclose(f));
  return AnsiString(in_buf, in_cnt);;
}

struct VersionInfo {
  unsigned major;
  unsigned minor;
  unsigned patch;
};

VersionInfo load_version_info(const AnsiString& file_name) {
  VersionInfo ver_info;
  FILE* f = fopen(file_name.data(), "rb");
  if (f == NULL) FAIL(MsgError(strerror(errno)));
  try {
    char in_buf[64];
    unsigned in_cnt = fread(in_buf, 1, sizeof(in_buf), f);
    if ((in_cnt < 5) || (in_cnt == sizeof(in_buf))) FAIL(MsgError("Invalid version info"));
    AnsiString verstr(in_buf, in_cnt);
    unsigned dot_idx1 = verstr.search('.');
    if (dot_idx1 == -1) FAIL(MsgError("Invalid version info"));
    unsigned dot_idx2 = verstr.search(dot_idx1 + 1, '.');
    if (dot_idx2 == -1) FAIL(MsgError("Invalid version info"));
    bool valid = true;
    valid = valid && str_to_uint(verstr.left(dot_idx1), ver_info.major);
    valid = valid && str_to_uint(verstr.slice(dot_idx1 + 1, dot_idx2 - dot_idx1 - 1), ver_info.minor);
    valid = valid && str_to_uint(verstr.right(verstr.size() - dot_idx2 - 1), ver_info.patch);
    if (!valid) FAIL(MsgError("Invalid version info"));
  }
  finally (fclose(f));
  return ver_info;
}

AnsiString replace_file_name(const AnsiString& path, const AnsiString file_name) {
  unsigned idx1 = path.rsearch('\\');
  if (idx1 == -1) idx1 = 0; else idx1++;
  unsigned idx2 = path.rsearch('/');
  if (idx2 == -1) idx2 = 0; else idx2++;
  unsigned idx;
  if (idx1 > idx2) idx = idx1; else idx = idx2;
  return path.left(idx) + file_name;
}

AnsiString load_template(const AnsiString& file_name) {
  AnsiString tmpl;
  FILE* f = fopen(file_name.data(), "rb");
  if (f == NULL) FAIL(MsgError(strerror(errno)));
  try {
    char in_buf[1024];
    while (!feof(f)) {
      unsigned in_cnt = fread(in_buf, 1, sizeof(in_buf), f);
      if (ferror(f)) FAIL(MsgError(strerror(errno)));
      tmpl.add(in_buf, in_cnt);
    }
  }
  finally (fclose(f));
  return tmpl;
}

void write_output(const AnsiString& str, const AnsiString& file_name) {
  FILE* f = fopen(file_name.data(), "wb");
  if (f == NULL) FAIL(MsgError(strerror(errno)));
  try {
    fwrite(str.data(), 1, str.size(), f);
    if (ferror(f)) FAIL(MsgError(strerror(errno)));
  }
  finally (fclose(f));
}

void subst_var(AnsiString& str, const AnsiString& name, const AnsiString& value) {
  unsigned idx = 0;
  while (true) {
    idx = str.search(idx, name);
    if (idx == -1) break;
    str.replace(idx, name.size(), value);
    idx += value.size();
  }
}

AnsiString process_template(const AnsiString& tmpl, const VersionInfo& ver_info, const RevInfo& rev_info) {
  AnsiString result = tmpl;
  if (rev_info.mixed) subst_var(result, "$WCREV$", uint_to_str(rev_info.max_rev));
  else subst_var(result, "$WCREV$", uint_to_str(rev_info.revision));
  subst_var(result, "$WCMOD$", rev_info.modified ? "1" : "0");
  subst_var(result, "$WCSW$", rev_info.switched ? "1" : "0");
  subst_var(result, "$VER_MAJOR$", uint_to_str(ver_info.major));
  subst_var(result, "$VER_MINOR$", uint_to_str(ver_info.minor));
  subst_var(result, "$VER_PATCH$", uint_to_str(ver_info.patch));
  return result;
}

bool file_exists(const AnsiString& file_name) {
  FILE* f = fopen(file_name.data(), "rb");
  bool res = (f != NULL);
  if (res) fclose(f);
  return res;
}

int main(int argc, char* argv[]) {
  try {
    // command line params
    if (argc < 2) FAIL(MsgError("Not enough command line parameters"));
    if (argc == 2) {
      // update revision number
      AnsiString rev_file_name = argv[1];
      AnsiString old_rev_str;
      if (file_exists(rev_file_name)) old_rev_str = load_rev_str(rev_file_name);
      AnsiString new_rev_str = read_rev_str();
      parse_rev_info(new_rev_str);
      if (old_rev_str != new_rev_str) write_output(new_rev_str, rev_file_name);
    }
    else {
      // process template
      AnsiString tmpl_file_name = argv[1];
      AnsiString out_file_name = argv[2];
      AnsiString ver_file_name;
      if (argc <= 3) ver_file_name = replace_file_name(tmpl_file_name, "version");
      else ver_file_name = argv[3];
      AnsiString rev_file_name;
      if (argc <= 4) rev_file_name = replace_file_name(out_file_name, "revision");
      else rev_file_name = argv[4];

      // read new revision string
      AnsiString rev_str = load_rev_str(rev_file_name);
      RevInfo rev_info = parse_rev_info(rev_str);

      // read version information from file
      VersionInfo ver_info = load_version_info(ver_file_name);

      // process template
      AnsiString tmpl = load_template(tmpl_file_name);
      AnsiString output = process_template(tmpl, ver_info, rev_info);
      write_output(output, out_file_name);
    }
  }
  catch (Error& e) {
    fputs(AnsiString::format("File: %s Line: %u: %S\n", e.file, e.line, &e.message()).data(), stderr);
    return 1;
  }
  return 0;
}
