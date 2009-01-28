#ifndef _FILEPATH_H
#define _FILEPATH_H

class FilePath: public ObjectArray<UnicodeString> {
private:
  void convert_path(const UnicodeString& _path);
  void normalize();
public:
  bool is_absolute;
  UnicodeString root;
  FilePath(const UnicodeString& path) {
    convert_path(path);
  }
  FilePath(const FilePath& fp) {
    *this = fp;
  }
  FilePath& operator=(const FilePath& fp) {
    (*this).ObjectArray<UnicodeString>::operator=(fp);
    is_absolute = fp.is_absolute;
    root = fp.root;
    return *this;
  }
  FilePath& operator=(const UnicodeString& path) {
    convert_path(path);
    return *this;
  }
  bool is_root_path() const {
    return is_absolute && (size() == 0);
  }
  UnicodeString get_dir_path() const {
    if (size() != 0) return get_partial_path(size() - 1);
    else return get_partial_path(0);
  }
  UnicodeString get_file_name() const {
    if (size() != 0) return last();
    else return UnicodeString();
  }
  UnicodeString get_partial_path(unsigned num_part) const {
    UnicodeString path;
    if (is_absolute) path = root + L'\\';
    for (unsigned i = 0; i < num_part; i++) {
      path += (*this)[i];
      if (i + 1 < num_part) path += L'\\';
    }
    return path;
  }
  UnicodeString get_full_path() const {
    return get_partial_path(size());
  }
  FilePath& combine(const FilePath& fp) {
    if (fp.is_absolute) *this = fp;
    else {
      add(fp);
      normalize();
    }
    return *this;
  }
};

#endif // _FILEPATH_H
