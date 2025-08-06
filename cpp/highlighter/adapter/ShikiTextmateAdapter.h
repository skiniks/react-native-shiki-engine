#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// Current highlighter includes
#include "highlighter/theme/ThemeStyle.h"
#include "highlighter/platform/PlatformHighlighter.h"

// Shikitori includes
#include "shikitori/theme.h"

// Define our own simple Token structure for the adapter interface
namespace shiki {
    struct Token {
        size_t start;
        size_t length;
        std::vector<std::string> scopes;
        ThemeStyle style;

        void addScope(const std::string& scope) {
            if (!scope.empty()) {
                scopes.push_back(scope);
            }
        }

        std::string getCombinedScope() const {
            std::string combined;
            for (size_t i = 0; i < scopes.size(); ++i) {
                if (i > 0) combined += " ";
                combined += scopes[i];
            }
            return combined;
        }
    };
}

// Forward declarations to avoid header conflicts
namespace shikitori {
    class Registry;
    class IGrammar;
    struct IRawGrammar;
}

// Note: Full shiki-textmate integration now active!

namespace shiki {

/**
 * Adapter class that bridges the current highlighter interface with shiki-textmate
 */
class ShikiTextmateAdapter {
public:
    ShikiTextmateAdapter();
    ~ShikiTextmateAdapter();

    // Grammar management
    bool loadGrammar(const std::string& scopeName, const std::string& grammarContent);
    bool setGrammar(const std::string& scopeName);

    // Theme management
    bool loadTheme(const std::string& themeName, const std::string& themeContent);
    bool setTheme(const std::string& themeName);

    // Tokenization
    std::vector<Token> tokenize(const std::string& code);
    std::vector<shiki::PlatformStyledToken> tokenizeWithStyles(const std::string& code);

    // Theme style resolution
    ThemeStyle resolveStyle(const std::string& scope) const;

    // Debug test method
    bool testMethod() const;

    // Configuration
    void setConfiguration(const std::string& config);

private:
    std::unique_ptr<shikitori::Registry> registry_;
    std::shared_ptr<shikitori::IGrammar> currentGrammar_;
    std::shared_ptr<shikitori::Theme> currentTheme_;

    std::string currentGrammarScope_;
    std::string currentThemeName_;

    // Cache for loaded content
    std::unordered_map<std::string, std::shared_ptr<shikitori::IRawGrammar>> grammarCache_;
    std::unordered_map<std::string, std::string> themeCache_;

        // Helper methods
    ThemeStyle getDefaultStyle() const;
    Token createDefaultToken(const std::string& code, size_t line) const;
    PlatformStyledToken createDefaultStyledToken(const std::string& code) const;
};

}  // namespace shiki
