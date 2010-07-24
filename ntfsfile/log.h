#pragma once

struct LogRecord {
  UnicodeString object;
  UnicodeString message;
};

struct Log: public ObjectArray<LogRecord> {
  void add(const UnicodeString& object, const UnicodeString& message) {
    LogRecord log_rec;
    log_rec.object = object;
    log_rec.message = message;
    ObjectArray<LogRecord>::add(log_rec);
  }
  void show();
};
