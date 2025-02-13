#pragma once
#include "HighlightError.h"
#include <functional>
#include <memory>
#include <queue>
#include <string>

namespace shiki {

enum class ErrorSeverity { Warning, Error, Critical };

struct ErrorInfo {
  HighlightErrorCode code;
  ErrorSeverity severity;
  std::string message;
  std::string context;
  bool requiresReset;
};

using ErrorCallback = std::function<void(const ErrorInfo&)>;
using RecoveryCallback = std::function<bool()>;

class ErrorManager {
public:
  static ErrorManager& getInstance();

  // Error reporting
  void reportError(const ErrorInfo& error);
  void setErrorCallback(ErrorCallback callback);

  // Recovery management
  void registerRecoveryStrategy(HighlightErrorCode code, RecoveryCallback strategy);
  bool attemptRecovery(const ErrorInfo& error);

  // State management
  bool hasUnrecoverableError() const {
    return hasUnrecoverableError_;
  }
  void reset();

  // JS bridge error reporting
  void setBridgeErrorCallback(std::function<void(const std::string&)> callback) {
    bridgeErrorCallback_ = std::move(callback);
  }

private:
  ErrorManager() = default;

  bool hasUnrecoverableError_{false};
  ErrorCallback errorCallback_;
  std::function<void(const std::string&)> bridgeErrorCallback_;
  std::unordered_map<HighlightErrorCode, RecoveryCallback> recoveryStrategies_;
  std::queue<ErrorInfo> errorQueue_;

  void notifyJSLayer(const ErrorInfo& error);
  bool shouldAttemptRecovery(const ErrorInfo& error) const;
  void handleUnrecoverableError(const ErrorInfo& error);
};

} // namespace shiki
