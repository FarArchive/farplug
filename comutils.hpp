#pragma once

#include "noncopyable.hpp"
#include "error.hpp"

#define CHECK_COM(code) { HRESULT __ret = (code); if (__ret != S_OK) FAIL(__ret); }

inline bool check_com(HRESULT hr) {
  if (FAILED(hr))
    FAIL(hr);
  return hr == S_OK;
}

#define COM_ERROR_HANDLER_BEGIN try {

#define COM_ERROR_HANDLER_END \
  } \
  catch (const Error& e) { \
    return e.hr; \
  } \
  catch (...) { \
    return E_FAIL; \
  }


class UnknownImpl {
protected:
  ULONG ref_cnt;
public:
  UnknownImpl(): ref_cnt(0) {}
};

#define UNKNOWN_IMPL_BEGIN \
  STDMETHOD(QueryInterface)(REFIID riid, void** object) {

#define UNKNOWN_IMPL_ITF(iid) \
    if (riid == IID_##iid) { *object = this; AddRef(); return S_OK; }

#define UNKNOWN_IMPL_END \
    if (riid == IID_IUnknown) { *object = this; AddRef(); return S_OK; } \
    *object = NULL; return E_NOINTERFACE; \
  } \
  STDMETHOD_(ULONG, AddRef)() { return ++ref_cnt; } \
  STDMETHOD_(ULONG, Release)() { if (--ref_cnt == 0) delete this; return ref_cnt; }

template<class Itf> class ComObject {
private:
  Itf* obj;
public:
  ComObject(): obj(NULL) {}
  ComObject(Itf* param): obj(param) {
    if (obj)
      obj->AddRef();
  }
  ComObject(const ComObject<Itf>& param): obj(param.obj) {
    if (obj)
      obj->AddRef();
  }
  ~ComObject() {
    if (obj)
      obj->Release();
  }
  operator Itf*() const {
    return obj;
  }
  Itf** operator&() {
    return &obj;
  }
  Itf* operator->() const {
    return obj;
  }
  Itf* operator=(Itf* param) {
    if (obj)
      obj->Release();
    obj = param;
    if (obj)
      obj->AddRef();
    return obj;
  }
  Itf* operator=(const ComObject<Itf>& param) {
    return *this = param.obj;
  }
};

class PropVariant: public PROPVARIANT, private NonCopyable {
public:
  PropVariant() {
    PropVariantInit(this);
  }
  ~PropVariant() {
    PropVariantClear(this);
  }
};
