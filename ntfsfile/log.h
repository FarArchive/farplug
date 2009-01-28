#pragma once

enum LogOpType {
  lotDefragment,
};

struct LogRecord {
  LogOpType type;
  UnicodeString object;
  UnicodeString message;
};

struct Log: public ObjectArray<LogRecord> {
  void add(LogOpType type, const UnicodeString& object, const UnicodeString& message) {
    LogRecord log_rec;
    log_rec.type = type;
    log_rec.object = object;
    log_rec.message = message;
    ObjectArray<LogRecord>::add(log_rec);
  }
  void show();
};
