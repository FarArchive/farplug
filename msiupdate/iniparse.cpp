#include "utils.hpp"
#include "iniparse.hpp"

namespace Ini {

const wchar_t* c_update_info_format = L"Invalid update info format";

string File::get(const string& section_name, const string& key_name) {
  const_iterator section_pos = find(section_name);
  if (section_pos == end())
    FAIL_MSG(c_update_info_format);
  const Section& section = section_pos->second;
  Section::const_iterator key_pos = section.find(key_name);
  if (key_pos == section.end())
    FAIL_MSG(c_update_info_format);
  return key_pos->second;
}

void File::parse(const string& str) {
  clear();
  string section_name;
  Section section;
  size_t begin_pos = 0;
  while (begin_pos < str.size()) {
    size_t end_pos = str.find('\n', begin_pos);
    if (end_pos == string::npos)
      end_pos = str.size();
    else
      end_pos++;
    string line = strip(str.substr(begin_pos, end_pos - begin_pos));
    if ((line.size() > 2) && (line[0] == '[') && (line[line.size() - 1] == ']')) {
      // section header
      if (!section.empty()) {
        (*this)[section_name] = section;
        section.clear();
      }
      section_name = strip(line.substr(1, line.size() - 2));
    }
    if ((line.size() > 0) && (line[0] == ';')) {
      // comment
    }
    else {
      size_t delim_pos = line.find('=');
      if (delim_pos != string::npos) {
        // name = value pair
        string name = strip(line.substr(0, delim_pos));
        string value = strip(line.substr(delim_pos + 1, line.size() - delim_pos - 1));
        // remove quotes if needed
        if ((value.size() >= 2) && (value[0] == '"') && (value[value.size() - 1] == '"'))
          value = value.substr(1, value.size() - 2);
        if (!name.empty() && !value.empty())
          section[name] = value;
      }
    }
    begin_pos = end_pos;
  }
  if (!section.empty()) {
    (*this)[section_name] = section;
  }
}

}
