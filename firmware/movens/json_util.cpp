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

// Append code point cp (BMP) as UTF-8
static void appendUtf8(String& out, uint16_t cp) {
  if (cp < 0x80) {
    out += (char)cp;
  } else if (cp < 0x800) {
    out += (char)(0xC0 | (cp >> 6));
    out += (char)(0x80 | (cp & 0x3F));
  } else {
    out += (char)(0xE0 | (cp >> 12));
    out += (char)(0x80 | ((cp >> 6) & 0x3F));
    out += (char)(0x80 | (cp & 0x3F));
  }
}

bool jsonString(const String& body, const char* key, String& out) {
  String pattern = String("\"") + key + "\"";
  int idx = body.indexOf(pattern);
  if (idx < 0) return false;
  idx += pattern.length();
  int n = body.length();
  while (idx < n && (body[idx] == ':' || body[idx] == ' ')) idx++;
  if (idx >= n || body[idx] != '"') return false;
  out = "";
  for (idx++; idx < n; idx++) {
    char c = body[idx];
    if (c == '"') return true;
    if (c != '\\') { out += c; continue; }
    if (++idx >= n) break;
    switch (body[idx]) {
      case 'b': out += '\b'; break;
      case 'f': out += '\f'; break;
      case 'n': out += '\n'; break;
      case 'r': out += '\r'; break;
      case 't': out += '\t'; break;
      case 'u':
        if (idx + 4 >= n) return false;
        appendUtf8(out, (uint16_t)strtoul(body.substring(idx + 1, idx + 5).c_str(), nullptr, 16));
        idx += 4;
        break;
      default:  out += body[idx]; break;  // \" \\ \/
    }
  }
  return false;  // unterminated
}

String jsonQuote(const String& s) {
  String out = "\"";
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if ((uint8_t)c < 0x20) {
      char buf[7];
      snprintf(buf, sizeof(buf), "\\u%04x", c);
      out += buf;
    } else {
      out += c;
    }
  }
  return out + "\"";
}
