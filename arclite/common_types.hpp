#pragma once

struct Attr {
  wstring name;
  wstring value;
};
typedef list<Attr> AttrList;

class ErrorLog: public map<wstring, Error> {
public:
  void add(const wstring& file_path, const Error& e) {
    if (count(file_path) == 0)
      (*this)[file_path] = e;
  }
};
