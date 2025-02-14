#pragma once
#include <stdexcept>
#include <string>

namespace shiki {

enum class HighlightErrorCode {
  None = 0,
  InvalidGrammar,
  InvalidTheme,
  RegexCompilationFailed,
  TokenizationFailed,
  InputTooLarge,
  ResourceLoadFailed,
  OutOfMemory,
  InvalidInput,
  GrammarError
};

class HighlightError : public std::runtime_error {
public:
  HighlightError(HighlightErrorCode code, const std::string& message)
      : std::runtime_error(message), code_(code) {}

  HighlightErrorCode code() const {
    return code_;
  }

private:
  HighlightErrorCode code_;
};

class ErrorUtils {
public:
  static void throwError(HighlightErrorCode code, const std::string& message) {
    throw HighlightError(code, message);
  }

  static void throwIfNull(const void* ptr, const std::string& what) {
    if (!ptr) {
      throwError(HighlightErrorCode::InvalidInput, "Null pointer provided for " + what);
    }
  }

  static void throwIfEmpty(const std::string& str, const std::string& what) {
    if (str.empty()) {
      throwError(HighlightErrorCode::InvalidInput, "Empty string provided for " + what);
    }
  }
};

} // namespace shiki
