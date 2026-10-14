// Harness checks for the world dumps: a strict JSON syntax validator and a
// non-finite token scan (nan, inf, -nan(ind), -inf) that skips quoted strings.
#pragma once
#include <cctype>
#include <cstring>
#include <string>

class Validator {
 public:
  explicit Validator(const std::string& text) : s_(text) {}

  // True when the text is exactly one JSON value. Error names the first problem.
  bool Run() {
    const bool ok = Value() && (Ws(), i_ == s_.size() || Fail("trailing text"));
    return ok;
  }
  std::string error() const { return error_; }

 private:
  bool Fail(const char* what) {
    if (error_.empty()) error_ = std::string(what) + " at offset " + std::to_string(i_);
    return false;
  }
  void Ws() {
    while (i_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[i_]))) ++i_;
  }
  bool Digits() {
    const size_t start = i_;
    while (i_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[i_]))) ++i_;
    return i_ > start;
  }
  bool Str() {
    ++i_;
    while (i_ < s_.size()) {
      const char c = s_[i_++];
      if (c == '\\') {
        if (i_ >= s_.size()) return Fail("unterminated escape");
        ++i_;
      } else if (c == '"') {
        return true;
      }
    }
    return Fail("unterminated string");
  }
  bool Number() {
    if (i_ < s_.size() && s_[i_] == '-') ++i_;
    if (!Digits()) return Fail("number");
    if (i_ < s_.size() && s_[i_] == '.') {
      ++i_;
      if (!Digits()) return Fail("fraction");
    }
    if (i_ < s_.size() && (s_[i_] == 'e' || s_[i_] == 'E')) {
      ++i_;
      if (i_ < s_.size() && (s_[i_] == '+' || s_[i_] == '-')) ++i_;
      if (!Digits()) return Fail("exponent");
    }
    return true;
  }
  bool Literal(const char* word) {
    const size_t n = std::strlen(word);
    if (s_.compare(i_, n, word) != 0) return Fail("literal");
    i_ += n;
    return true;
  }
  bool Value() {
    Ws();
    if (i_ >= s_.size()) return Fail("missing value");
    switch (s_[i_]) {
      case '{': return Object();
      case '[': return Array();
      case '"': return Str();
      case 't': return Literal("true");
      case 'f': return Literal("false");
      case 'n': return Literal("null");
      default: return Number();
    }
  }
  bool Object() {
    ++i_;
    Ws();
    if (i_ < s_.size() && s_[i_] == '}') {
      ++i_;
      return true;
    }
    for (;;) {
      Ws();
      if (i_ >= s_.size() || s_[i_] != '"') return Fail("object key");
      if (!Str()) return false;
      Ws();
      if (i_ >= s_.size() || s_[i_] != ':') return Fail("colon");
      ++i_;
      if (!Value()) return false;
      Ws();
      if (i_ >= s_.size()) return Fail("unterminated object");
      if (s_[i_] == '}') {
        ++i_;
        return true;
      }
      if (s_[i_] != ',') return Fail("object separator");
      ++i_;
    }
  }
  bool Array() {
    ++i_;
    Ws();
    if (i_ < s_.size() && s_[i_] == ']') {
      ++i_;
      return true;
    }
    for (;;) {
      if (!Value()) return false;
      Ws();
      if (i_ >= s_.size()) return Fail("unterminated array");
      if (s_[i_] == ']') {
        ++i_;
        return true;
      }
      if (s_[i_] != ',') return Fail("array separator");
      ++i_;
    }
  }

  const std::string& s_;
  size_t i_ = 0;
  std::string error_;
};

// Empty when the text is one well-formed JSON value; otherwise the first problem.
inline std::string JsonError(const std::string& text) {
  Validator validator(text);
  if (validator.Run()) return "";
  return validator.error();
}

// nan, inf, -nan(ind) and -inf outside quoted strings (a word, not part of a longer one).
inline bool HasNonFiniteToken(const std::string& text) {
  std::string bare = text;
  bool in_string = false;
  for (size_t i = 0; i < bare.size(); ++i) {
    if (in_string) {
      if (bare[i] == '\\') {
        bare[i] = ' ';
        if (i + 1 < bare.size()) bare[++i] = ' ';
      } else if (bare[i] == '"') {
        in_string = false;
        bare[i] = ' ';
      } else {
        bare[i] = ' ';
      }
    } else if (bare[i] == '"') {
      in_string = true;
      bare[i] = ' ';
    } else {
      bare[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(bare[i])));
    }
  }
  for (const std::string token : {"nan", "inf"}) {
    for (size_t at = bare.find(token); at != std::string::npos; at = bare.find(token, at + 1)) {
      const bool before = at == 0u || !std::isalnum(static_cast<unsigned char>(bare[at - 1]));
      const size_t end = at + token.size();
      const bool after = end >= bare.size() || !std::isalnum(static_cast<unsigned char>(bare[end]));
      if (before && after) return true;
    }
  }
  return false;
}
