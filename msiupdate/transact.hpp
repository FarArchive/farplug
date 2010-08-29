#pragma once

class Transaction: private NonCopyable {
private:
  HANDLE h_trans;
public:
  Transaction();
  ~Transaction();
  void commit();
  HANDLE handle() const;
};

class TransactedFile: public File {
public:
  TransactedFile(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE h_transaction);
};

class TransactedKey: public Key {
public:
  TransactedKey(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create, HANDLE h_transaction);
};

BOOL delete_file_transacted(LPCWSTR lpFileName, HANDLE hTransaction);
