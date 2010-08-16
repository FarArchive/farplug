#pragma once

#include "error.hpp"

inline bool s_ok(HRESULT hr) {
  if (FAILED(hr))
    FAIL(hr);
  return hr == S_OK;
}

#define COM_ERROR_HANDLER_BEGIN \
  try { \
    try {

#define COM_ERROR_HANDLER_END \
    } \
    catch (const Error& e) { \
      error = e; \
    } \
    catch (const std::exception& e) { \
      error = e; \
    } \
  } \
  catch (...) { \
    error.code = E_FAIL; \
  } \
  return error.code;

class ComBase: public IUnknown {
protected:
  ULONG ref_cnt;
  Error& error;
public:
  ComBase(Error& error): ref_cnt(0), error(error) {
  }
};

#define UNKNOWN_DECL \
  STDMETHOD(QueryInterface)(REFIID riid, void** object); \
  STDMETHOD_(ULONG, AddRef)(); \
  STDMETHOD_(ULONG, Release)();

#define UNKNOWN_IMPL_BEGIN \
  STDMETHOD(QueryInterface)(REFIID riid, void** object) {

#define UNKNOWN_IMPL_ITF(iid) \
    if (riid == IID_##iid) { *object = static_cast<iid*>(this); AddRef(); return S_OK; }

#define UNKNOWN_IMPL_END \
    if (riid == IID_IUnknown) { *object = static_cast<IUnknown*>(static_cast<ComBase*>(this)); AddRef(); return S_OK; } \
    *object = nullptr; return E_NOINTERFACE; \
  } \
  STDMETHOD_(ULONG, AddRef)() { return ++ref_cnt; } \
  STDMETHOD_(ULONG, Release)() { if (--ref_cnt == 0) { delete this; return 0; } else return ref_cnt; }

template<class Itf> class ComObject {
private:
  Itf* obj;
public:
  ComObject(): obj(nullptr) {}
  ComObject(Itf* param): obj(param) {
    if (obj)
      obj->AddRef();
  }
  ComObject(const ComObject<Itf>& param): obj(param.obj) {
    if (obj)
      obj->AddRef();
  }
  ~ComObject() {
    Release();
  }
  void Release() {
    if (obj) {
      obj->Release();
      obj = nullptr;
    }
  }
  operator Itf*() const {
    return obj;
  }
  operator bool() const {
    return obj != nullptr;
  }
  Itf** operator&() {
    Release();
    return &obj;
  }
  Itf* operator->() const {
    return obj;
  }
  Itf* operator=(Itf* param) {
    Release();
    obj = param;
    if (obj)
      obj->AddRef();
    return obj;
  }
  Itf* operator=(const ComObject<Itf>& param) {
    return *this = param.obj;
  }
  void detach(Itf** param) {
    *param = obj;
    obj = nullptr;
  }
};

class PropVariant: public PROPVARIANT {
private:
  void clear() {
    if (vt != VT_EMPTY)
      CHECK_COM(PropVariantClear(this));
  }
public:
  PropVariant() {
    PropVariantInit(this);
  }
  ~PropVariant() {
    clear();
  }
  PROPVARIANT* ref() {
    clear();
    return this;
  }
  void detach(PROPVARIANT* var) {
    if (var->vt != VT_EMPTY)
      CHECK_COM(PropVariantClear(var));
    *var = *this;
    vt = VT_EMPTY;
  }

  PropVariant(const PropVariant& var) {
    CHECK_COM(PropVariantCopy(this, &var));
  }
  PropVariant(const wstring& val) {
    vt = VT_BSTR;
    bstrVal = SysAllocStringLen(val.data(), val.size());
    if (bstrVal == nullptr) {
      vt = VT_ERROR;
      FAIL(E_OUTOFMEMORY);
    }
  }
  PropVariant(const wchar_t* val) {
    vt = VT_BSTR;
    bstrVal = SysAllocStringLen(val, wcslen(val));
    if (bstrVal == nullptr) {
      vt = VT_ERROR;
      FAIL(E_OUTOFMEMORY);
    }
  }
  PropVariant(bool val) {
    vt = VT_BOOL;
    boolVal = val ? VARIANT_TRUE : VARIANT_FALSE;
  }
  PropVariant(UInt32 val) {
    vt = VT_UI4;
    ulVal = val;
  }
  PropVariant(UInt64 val) {
    vt = VT_UI8;
    uhVal.QuadPart = val;
  }
  PropVariant(const FILETIME &val) {
    vt = VT_FILETIME;
    filetime = val;
  }

  PropVariant& operator=(const PropVariant& var) {
    clear();
    CHECK_COM(PropVariantCopy(this, &var));
  }
  PropVariant& operator=(const wstring& val) {
    clear();
    vt = VT_BSTR;
    bstrVal = SysAllocStringLen(val.data(), val.size());
    if (bstrVal == nullptr) {
      vt = VT_ERROR;
      FAIL(E_OUTOFMEMORY);
    }
    return *this;
  }
  PropVariant& operator=(const wchar_t* val) {
    clear();
    vt = VT_BSTR;
    bstrVal = SysAllocStringLen(val, wcslen(val));
    if (bstrVal == nullptr) {
      vt = VT_ERROR;
      FAIL(E_OUTOFMEMORY);
    }
    return *this;
  }
  PropVariant& operator=(bool val) {
    if (vt != VT_BOOL) {
      clear();
      vt = VT_BOOL;
    }
    boolVal = val ? VARIANT_TRUE : VARIANT_FALSE;
    return *this;
  }
  PropVariant& operator=(UInt32 val) {
    if (vt != VT_UI4) {
      clear();
      vt = VT_UI4;
    }
    ulVal = val;
    return *this;
  }
  PropVariant& operator=(UInt64 val) {
    if (vt != VT_UI8) {
      clear();
      vt = VT_UI8;
    }
    uhVal.QuadPart = val;
    return *this;
  }
  PropVariant& operator=(const FILETIME &val) {
    if (vt != VT_FILETIME) {
      clear();
      vt = VT_FILETIME;
    }
    filetime = val;
    return *this;
  }
};

class BStr {
private:
  BSTR bstr;
  void clear() {
    if (bstr) {
      SysFreeString(bstr);
      bstr = nullptr;
    }
  }
public:
  BStr(): bstr(nullptr) {
  }
  ~BStr() {
    clear();
  }
  operator const wchar_t*() const {
    return bstr;
  }
  unsigned size() const {
    return SysStringLen(bstr);
  }
  BSTR* ref() {
    clear();
    return &bstr;
  }
  void detach(BSTR* str) {
    *str = bstr;
    bstr = nullptr;
  }

  BStr(const BStr& str) {
    bstr = SysAllocStringLen(str.bstr, SysStringLen(str.bstr));
    if (bstr == nullptr)
      FAIL(E_OUTOFMEMORY);
  }
  BStr(const wstring& str) {
    bstr = SysAllocStringLen(str.data(), str.size());
    if (bstr == nullptr)
      FAIL(E_OUTOFMEMORY);
  }
  BStr(const wchar_t* str) {
    bstr = SysAllocString(str);
    if (bstr == nullptr)
      FAIL(E_OUTOFMEMORY);
  }

  BStr& operator=(const BStr& str) {
    if (!SysReAllocStringLen(&bstr, str.bstr, SysStringLen(str.bstr)))
      FAIL(E_OUTOFMEMORY);
    return *this;
  }
  BStr& operator=(const wstring& str) {
    if (!SysReAllocStringLen(&bstr, str.data(), str.size()))
      FAIL(E_OUTOFMEMORY);
    return *this;
  }
  BStr& operator=(const wchar_t* str) {
    if (!SysReAllocString(&bstr, str))
      FAIL(E_OUTOFMEMORY);
    return *this;
  }
};
