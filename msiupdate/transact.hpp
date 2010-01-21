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

BOOL delete_file_transacted(LPCWSTR lpFileName, HANDLE hTransaction);
