#pragma once

#include "error.hpp"

bool substr_match(const wstring& str, wstring::size_type pos, wstring::const_pointer mstr);
wstring word_wrap(const wstring& str, wstring::size_type wrap_bound);
wstring fit_str(const wstring& str, wstring::size_type size);
wstring strip(const wstring& str);
int str_to_int(const wstring& str);
wstring int_to_str(int val);
wstring widen(const string& str);

wstring long_path(const wstring& path);

wstring add_trailing_slash(const wstring& path);
wstring del_trailing_slash(const wstring& path);

wstring extract_path_root(const wstring& path);
wstring extract_file_name(const wstring& path);
wstring extract_file_path(const wstring& path);
bool is_root_path(const wstring& path);
bool is_unc_path(const wstring& path);
bool is_absolute_path(const wstring& path);
wstring remove_path_root(const wstring& path);
