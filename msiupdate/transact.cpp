#include "error.hpp"
#include "utils.hpp"
#include "sysutils.hpp"
#include "transact.hpp"

typedef HANDLE (APIENTRY *FCreateTransaction)(LPSECURITY_ATTRIBUTES lpTransactionAttributes, LPGUID UOW, DWORD CreateOptions, DWORD IsolationLevel, DWORD IsolationFlags, DWORD Timeout, LPWSTR Description);
typedef BOOL (APIENTRY *FCommitTransaction)(HANDLE TransactionHandle);
typedef HANDLE (WINAPI *FCreateFileTransactedW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile, HANDLE hTransaction, PUSHORT pusMiniVersion, PVOID lpExtendedParameter);
typedef BOOL (WINAPI *FDeleteFileTransactedW)(LPCWSTR lpFileName, HANDLE hTransaction);
typedef LSTATUS (APIENTRY *FRegCreateKeyTransactedW)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, const LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition, HANDLE hTransaction, PVOID  pExtendedParemeter);
typedef LSTATUS (APIENTRY *FRegOpenKeyTransactedW)(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult, HANDLE hTransaction, PVOID  pExtendedParemeter);

HMODULE h_ktmw32 = NULL;
HMODULE h_kernel32 = NULL;
HMODULE h_advapi32 = NULL;

FCreateTransaction fCreateTransaction;
FCommitTransaction fCommitTransaction;
FCreateFileTransactedW fCreateFileTransactedW;
FDeleteFileTransactedW fDeleteFileTransactedW;
FRegCreateKeyTransactedW fRegCreateKeyTransactedW;
FRegOpenKeyTransactedW fRegOpenKeyTransactedW;

#define LOAD_MODULE(name) h_##name = LoadLibraryA(#name ".dll");
#define UNLOAD_MODULE(name) if (h_##name) { FreeLibrary(h_##name); h_##name = NULL; }
#define LOAD_API_ENTRY(module, name) if (h_##module) f##name = reinterpret_cast<F##name>(GetProcAddress(h_##module, #name));

class ApiLoader {
public:
  ApiLoader() {
    LOAD_MODULE(ktmw32);
    LOAD_MODULE(kernel32);
    LOAD_MODULE(advapi32);
    LOAD_API_ENTRY(ktmw32, CreateTransaction);
    LOAD_API_ENTRY(ktmw32, CommitTransaction);
    LOAD_API_ENTRY(kernel32, CreateFileTransactedW);
    LOAD_API_ENTRY(kernel32, DeleteFileTransactedW);
    LOAD_API_ENTRY(advapi32, RegCreateKeyTransactedW);
    LOAD_API_ENTRY(advapi32, RegOpenKeyTransactedW);
  }
  ~ApiLoader() {
    UNLOAD_MODULE(ktmw32);
    UNLOAD_MODULE(kernel32);
    UNLOAD_MODULE(advapi32);
  }
};

ApiLoader api_loader;

Transaction::Transaction() {
  if (fCreateTransaction) {
    h_trans = fCreateTransaction(NULL, 0, 0, 0, 0, NULL, NULL);
    CHECK_SYS(h_trans != INVALID_HANDLE_VALUE);
  }
}

Transaction::~Transaction() {
  if (fCreateTransaction) {
    CloseHandle(h_trans);
  }
}

void Transaction::commit() {
  if (fCommitTransaction) {
    CHECK_SYS(fCommitTransaction(h_trans));
  }
}

HANDLE Transaction::handle() const {
  return h_trans;
}

#ifndef TXFS_MINIVERSION_DEFAULT_VIEW
#define TXFS_MINIVERSION_DEFAULT_VIEW (0xFFFE)
#endif

TransactedFile::TransactedFile(const wstring& file_path, DWORD desired_access, DWORD share_mode, DWORD creation_disposition, DWORD flags_and_attributes, HANDLE h_transaction) {
  if (fCreateFileTransactedW) {
    USHORT mv = TXFS_MINIVERSION_DEFAULT_VIEW;
    h_file = fCreateFileTransactedW(file_path.c_str(), desired_access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr, h_transaction, &mv, nullptr);
    CHECK_SYS(h_file != INVALID_HANDLE_VALUE);
  }
  else {
    open(file_path, desired_access, share_mode, creation_disposition, flags_and_attributes);
  }
}

BOOL delete_file_transacted(LPCWSTR lpFileName, HANDLE hTransaction) {
  if (fDeleteFileTransactedW) {
    return fDeleteFileTransactedW(lpFileName, hTransaction);
  }
  else {
    return DeleteFileW(lpFileName);
  }
}

TransactedKey::TransactedKey(HKEY h_parent, LPCWSTR sub_key, REGSAM sam_desired, bool create, HANDLE h_transaction) {
  if (fRegCreateKeyTransactedW) {
    if (create) {
      CHECK_ADVSYS(fRegCreateKeyTransactedW(h_parent, sub_key, 0, nullptr, REG_OPTION_NON_VOLATILE, sam_desired, nullptr, &h_key, nullptr, h_transaction, nullptr));
    }
    else {
      CHECK_ADVSYS(fRegOpenKeyTransactedW(h_parent, sub_key, 0, sam_desired, &h_key, h_transaction, nullptr));
    }
  }
  else {
    open(h_parent, sub_key, sam_desired, create);
  }
}
