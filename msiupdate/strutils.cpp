#include "utils.hpp"

bool substr_match(const wstring& str, wstring::size_type pos, wstring::const_pointer mstr) {
  size_t mstr_len = wcslen(mstr);
  if ((pos > str.length()) || (pos + mstr_len > str.length())) {
    return false;
  }
  return wmemcmp(str.data() + pos, mstr, mstr_len) == 0;
}

wstring word_wrap(const wstring& str, wstring::size_type wrap_bound) {
  wstring result;
  wstring::size_type begin_pos = 0;
  while (begin_pos < str.size()) {
    wstring::size_type end_pos = begin_pos + wrap_bound;
    if (end_pos < str.size()) {
      for (wstring::size_type i = end_pos; i > begin_pos; i--) {
        if (str[i - 1] == L' ') {
          end_pos = i;
          break;
        }
      }
    }
    else {
      end_pos = str.size();
    }
    if (!result.empty())
      result.append(1, L'\n');
    result.append(str.data() + begin_pos, end_pos - begin_pos);
    begin_pos = end_pos;
  }
  return result;
}

wstring fit_str(const wstring& str, wstring::size_type size) {
  if (str.size() <= size)
    return str;
  if (size <= 3)
    return str.substr(0, size);
  size -= 3; // place for ...
  return wstring(str).replace(size / 2, str.size() - size, L"...");
}

template<class CharType> basic_string<CharType> strip(const basic_string<CharType>& str) {
  basic_string<CharType>::size_type hp = 0;
  basic_string<CharType>::size_type tp = str.size();
  while ((hp < str.size()) && ((static_cast<unsigned>(str[hp]) <= 0x20) || (str[hp] == 0x7F)))
    hp++;
  if (hp < str.size())
    while ((static_cast<unsigned>(str[tp - 1]) <= 0x20) || (str[tp - 1] == 0x7F))
      tp--;
  return str.substr(hp, tp - hp);
}

string strip(const string& str) {
  return strip<string::value_type>(str);
}

wstring strip(const wstring& str) {
  return strip<wstring::value_type>(str);
}

int str_to_int(const string& str) {
  return atoi(str.c_str());
}

int str_to_int(const wstring& str) {
  return _wtoi(str.c_str());
}

wstring int_to_str(int val) {
  wchar_t str[64];
  return _itow(val, str, 10);
}

wstring widen(const string& str) {
  return wstring(str.begin(), str.end());
}

list<wstring> split(const wstring& str, wchar_t sep) {
  list<wstring> result;
  size_t begin = 0;
  while (begin < str.size()) {
    size_t end = str.find(sep, begin);
    if (end == wstring::npos)
      end = str.size();
    wstring sub_str = str.substr(begin, end - begin);
    result.push_back(sub_str);
    begin = end + 1;
  }
  return result;
}

wstring combine(const list<wstring>& lst, wchar_t sep) {
  size_t size = 0;
  for (list<wstring>::const_iterator str = lst.begin(); str != lst.end(); str++) {
    if (size)
      size++;
    size += str->size();
  }
  wstring result;
  result.reserve(size);
  for (list<wstring>::const_iterator str = lst.begin(); str != lst.end(); str++) {
    if (!result.empty())
      result.append(1, sep);
    result.append(*str);
  }
  return result;
}
