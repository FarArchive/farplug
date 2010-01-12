#pragma once

namespace ktm {

class Transaction: private NonCopyable {
private:
  HANDLE h_trans;
public:
  Transaction();
  ~Transaction();
  void commit();
  HANDLE handle() const;
};

class File: private NonCopyable {
private:
  HANDLE h_file;
public:
  File(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTransaction);
  ~File();
  HANDLE handle() const;
};

BOOL DeleteFile(LPCWSTR lpFileName, HANDLE hTransaction);

class Key: private NonCopyable {
private:
  HKEY h_key;
public:
  Key(HKEY hKey, LPCWSTR lpSubKey, REGSAM samDesired, HANDLE hTransaction);
  ~Key();
  HKEY handle() const;
};

}
