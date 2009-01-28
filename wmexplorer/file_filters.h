#ifndef _FILE_FILTERS_H
#define _FILE_FILTERS_H

struct Filter {
  UnicodeString dst_ext;
  UnicodeString description;
  UnicodeString guid;
};

struct FileFilters: public ObjectArray<Filter> {
  UnicodeString src_ext;
  bool operator==(const FileFilters& filters) const {
    return src_ext == filters.src_ext;
  }
  bool operator>(const FileFilters& filters) const {
    return src_ext > filters.src_ext;
  }
};

struct FilterInterface {
  UnicodeString src_ext;
  UnicodeString dst_ext;
  ICeFileFilter* itf;
  FilterInterface(const UnicodeString& src_ext, const UnicodeString& dst_ext, const UnicodeString& guid_str);
  FilterInterface(const FilterInterface& filter_itf);
  ~FilterInterface();
  FilterInterface& operator=(const FilterInterface& filter_itf);
};

extern ObjectArray<FileFilters> export_filter_list;
extern ObjectArray<FileFilters> import_filter_list;

void load_file_filters();
void convert_file(ICeFileFilter* filter_itf, const UnicodeString& src_file_name, const UnicodeString& dst_file_name, bool import);

#endif // _FILE_FILTERS_H
