#include "json_util.h"

int jsonInt(const String& body, const char* key) {
  String pattern = String("\"") + key + "\"";
  int idx = body.indexOf(pattern);
  if (idx < 0) return 0;
  idx += pattern.length();
  while (idx < (int)body.length() && (body[idx] == ':' || body[idx] == ' ')) idx++;
  int end = idx;
  if (end < (int)body.length() && body[end] == '-') end++;
  while (end < (int)body.length() && isDigit(body[end])) end++;
  return body.substring(idx, end).toInt();
}

float jsonFloat(const String& body, const char* key) {
  String pattern = String("\"") + key + "\"";
  int idx = body.indexOf(pattern);
  if (idx < 0) return NAN;
  idx += pattern.length();
  while (idx < (int)body.length() && (body[idx] == ':' || body[idx] == ' ')) idx++;
  int end = idx;
  while (end < (int)body.length()) {
    char c = body[end];
    if (!isDigit(c) && c != '-' && c != '+' && c != '.' && c != 'e' && c != 'E') break;
    end++;
  }
  if (end == idx) return NAN;
  return body.substring(idx, end).toFloat();
}
