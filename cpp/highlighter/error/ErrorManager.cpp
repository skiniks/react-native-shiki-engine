#include "ErrorManager.h"
#include <sstream>

namespace shiki {

ErrorManager& ErrorManager::getInstance() {
  static ErrorManager instance;
  return instance;
}

void ErrorManager::reportError(const ErrorInfo& error) {
  // Always notify JS layer first
  notifyJSLayer(error);

  // Queue the error
  errorQueue_.push(error);

  // Handle based on severity
  switch (error.severity) {
    case ErrorSeverity::Warning:
      // Just notify callback
      if (errorCallback_) {
        errorCallback_(error);
      }
      break;

    case ErrorSeverity::Error:
      if (shouldAttemptRecovery(error)) {
        if (!attemptRecovery(error)) {
          handleUnrecoverableError(error);
        }
      }
      break;

    case ErrorSeverity::Critical:
      handleUnrecoverableError(error);
      break;
  }
}

void ErrorManager::notifyJSLayer(const ErrorInfo& error) {
  if (!bridgeErrorCallback_)
    return;

  std::stringstream ss;
  ss << "{ \"code\": " << static_cast<int>(error.code) << ", \"severity\": \""
     << static_cast<int>(error.severity) << "\", \"message\": \"" << error.message
     << "\", \"context\": \"" << error.context
     << "\", \"requiresReset\": " << (error.requiresReset ? "true" : "false") << " }";

  bridgeErrorCallback_(ss.str());
}

bool ErrorManager::attemptRecovery(const ErrorInfo& error) {
  auto it = recoveryStrategies_.find(error.code);
  if (it != recoveryStrategies_.end()) {
    try {
      return it->second();
    } catch (...) {
      return false;
    }
  }
  return false;
}

void ErrorManager::registerRecoveryStrategy(HighlightErrorCode code, RecoveryCallback strategy) {
  recoveryStrategies_[code] = std::move(strategy);
}

bool ErrorManager::shouldAttemptRecovery(const ErrorInfo& error) const {
  return !error.requiresReset && recoveryStrategies_.find(error.code) != recoveryStrategies_.end();
}

void ErrorManager::handleUnrecoverableError(const ErrorInfo& error) {
  hasUnrecoverableError_ = true;
  if (errorCallback_) {
    errorCallback_(error);
  }
}

void ErrorManager::reset() {
  hasUnrecoverableError_ = false;
  while (!errorQueue_.empty()) {
    errorQueue_.pop();
  }
}

} // namespace shiki
