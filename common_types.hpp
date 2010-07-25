#pragma once

class ErrorLog: public map<wstring, Error> {
public:
  void add(const wstring& file_path, const Error& e) {
    (*this)[file_path] = e;
  }
};
