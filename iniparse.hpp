#pragma once

namespace Ini {

typedef map<string, string> Section;
class File: public map<string, Section> {
public:
  string get(const string& section_name, const string& key_name);
  void parse(const string& str);
};

}
