#include "ShikiTextmateAdapter.h"

#include <iostream>
#include <sstream>

// Include shikitori main API (uses RegistryOptions pattern)
#include "shikitori/main.h"
#include "shikitori/raw_grammar.h"
#include "shikitori/theme.h"
#include "shikitori/theme_provider.h"
#include "shikitori/utils/json_parser.h"

namespace shiki {

ShikiTextmateAdapter::ShikiTextmateAdapter() {
    std::cout << "[C++ DEBUG] ShikiTextmateAdapter CONSTRUCTOR called" << std::endl;
    std::cerr << "[C++ DEBUG] ShikiTextmateAdapter CONSTRUCTOR called via STDERR" << std::endl;

    try {
        // Create RegistryOptions
        shikitori::RegistryOptions options;

        // Set up the loadGrammar function - this will be called by the registry when it needs to load a grammar
        options.loadGrammar = [this](const std::string& scopeName) -> std::shared_ptr<shikitori::IRawGrammar> {
            auto it = grammarCache_.find(scopeName);
            if (it != grammarCache_.end()) {
                return it->second;
            }
            return nullptr;
        };

        // Create the registry with our options
        registry_ = std::make_unique<shikitori::Registry>(options);

        std::cout << "ShikiTextmateAdapter: Registry initialized successfully" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize shikitori::Registry: " << e.what() << std::endl;
        // Keep registry_ as nullptr, methods will check for this
    }
}

ShikiTextmateAdapter::~ShikiTextmateAdapter() = default;

bool ShikiTextmateAdapter::loadGrammar(const std::string& scopeName, const std::string& grammarContent) {
    if (!registry_) {
        std::cerr << "Registry not initialized, cannot load grammar" << std::endl;
        return false;
    }

    try {
        // Parse the JSON grammar content using utils::ParseJson
        auto parseResult = shikitori::utils::ParseJson(grammarContent);
        if (!parseResult.success) {
            std::cerr << "Failed to parse grammar JSON for " << scopeName << ": " << parseResult.error_message << std::endl;
            return false;
        }

        // Cache the parsed grammar (wrap in shared_ptr)
        grammarCache_[scopeName] = std::make_shared<shikitori::IRawGrammar>(std::move(parseResult.grammar));

        std::cout << "Grammar loaded and cached: " << scopeName << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to load grammar " << scopeName << ": " << e.what() << std::endl;
        return false;
    }
}

bool ShikiTextmateAdapter::setGrammar(const std::string& scopeName) {
    if (!registry_) {
        std::cerr << "Registry not initialized, cannot set grammar" << std::endl;
        return false;
    }

    try {
        // Use the registry to load the grammar (it will call our loadGrammar function)
        auto grammar = registry_->loadGrammar(scopeName);
        if (!grammar) {
            std::cerr << "Failed to load grammar from registry: " << scopeName << std::endl;
            return false;
        }

        // Set as current grammar
        currentGrammar_ = grammar;
        currentGrammarScope_ = scopeName;

        std::cout << "Grammar set successfully: " << scopeName << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error setting grammar " << scopeName << ": " << e.what() << std::endl;
        return false;
    }
}

bool ShikiTextmateAdapter::loadTheme(const std::string& themeName, const std::string& themeContent) {
    if (!registry_) {
        std::cerr << "Registry not initialized, cannot load theme" << std::endl;
        return false;
    }

    try {
        // Parse the JSON theme content
        auto themeResult = shikitori::utils::ParseThemeJson(themeContent);
        if (!themeResult.success) {
            std::cerr << "Failed to parse theme JSON for " << themeName << ": " << themeResult.error_message << std::endl;
            return false;
        }

        // Create theme object for direct style resolution
        fprintf(stderr, "[THEME LOAD DEBUG] Creating theme object from raw theme\n");
        currentTheme_ = shikitori::Theme::CreateFromRawTheme(themeResult.theme, nullptr);
        if (!currentTheme_) {
            std::cerr << "Failed to create theme object for " << themeName << std::endl;
            return false;
        }
        fprintf(stderr, "[THEME LOAD DEBUG] Theme object created successfully\n");

        // Apply the theme to the registry
        fprintf(stderr, "[THEME LOAD DEBUG] Applying theme to registry\n");
        registry_->setTheme(themeResult.theme, nullptr);

        // Check color map after theme is applied
        auto colorMap = registry_->getColorMap();
        fprintf(stderr, "[THEME LOAD DEBUG] Color map size after theme load: %zu\n", colorMap.size());

        // Print first few colors for verification
        for (size_t i = 0; i < std::min(size_t(10), colorMap.size()); ++i) {
            fprintf(stderr, "[THEME LOAD DEBUG] ColorMap[%zu] = %s\n", i, colorMap[i].c_str());
        }

        // Cache the theme content
        themeCache_[themeName] = themeContent;
        currentThemeName_ = themeName;

        std::cout << "Theme loaded and applied: " << themeName << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to load theme " << themeName << ": " << e.what() << std::endl;
        return false;
    }
}

bool ShikiTextmateAdapter::setTheme(const std::string& themeName) {
    if (!registry_) {
        std::cerr << "Registry not initialized, cannot set theme" << std::endl;
        return false;
    }

    // Check if we have it cached
    auto it = themeCache_.find(themeName);
    if (it != themeCache_.end()) {
        // Re-load and apply the cached theme
        return loadTheme(themeName, it->second);
    }

    std::cerr << "Theme not found in cache: " << themeName << std::endl;
    return false;
}

std::vector<Token> ShikiTextmateAdapter::tokenize(const std::string& code) {
    std::vector<Token> result;

    if (!currentGrammar_) {
        std::cerr << "No grammar set for tokenization" << std::endl;
        return result;
    }

    try {
        // Tokenize line by line using real shikitori grammar
        std::istringstream stream(code);
        std::string line;
        size_t lineNumber = 0;
        std::shared_ptr<shikitori::StateStack> ruleStack = nullptr;
        size_t absoluteOffset = 0;

        while (std::getline(stream, line)) {
            // Use shikitori's real tokenization
            auto lineResult = currentGrammar_->tokenizeLine(line, ruleStack);
            ruleStack = lineResult.ruleStack;

            // Convert shikitori tokens to our Token format
            for (const auto& shikitoriToken : lineResult.tokens) {
                Token token;
                token.start = absoluteOffset + shikitoriToken.startIndex;
                token.length = shikitoriToken.endIndex - shikitoriToken.startIndex;
                token.scopes = shikitoriToken.scopes;
                // Resolve style from the registry's theme using all scopes
                std::string allScopes = "";
                for (size_t i = 0; i < shikitoriToken.scopes.size(); ++i) {
                    allScopes += shikitoriToken.scopes[i];
                    if (i < shikitoriToken.scopes.size() - 1) allScopes += " ";
                }
                token.style = resolveStyle(allScopes);

                result.push_back(token);
            }

            absoluteOffset += line.length() + 1; // +1 for newline
            lineNumber++;
        }

        std::cout << "=== REAL TOKENIZATION DEBUG ===" << std::endl;
        std::cout << "Generated " << result.size() << " tokens for " << code.length() << " characters" << std::endl;
        for (size_t i = 0; i < std::min(result.size(), size_t(5)); ++i) {
            const auto& token = result[i];
            std::cout << "Token " << i << ": start=" << token.start << " length=" << token.length
                      << " scopes=" << (token.scopes.empty() ? "empty" : token.scopes[0])
                      << " color=" << token.style.foreground << std::endl;
        }
        std::cout << "===============================" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Failed to tokenize code: " << e.what() << std::endl;
        // Fallback to single token
        result.push_back(createDefaultToken(code, 0));
    }

    return result;
}

std::vector<shiki::PlatformStyledToken> ShikiTextmateAdapter::tokenizeWithStyles(const std::string& code) {
    std::vector<shiki::PlatformStyledToken> result;

    if (!currentGrammar_) {
        std::cerr << "No grammar set for styled tokenization" << std::endl;
        return result;
    }

    try {
        // Use the standard tokenize method and then convert to styled tokens
        auto tokens = tokenize(code);

        for (const auto& token : tokens) {
            shiki::PlatformStyledToken styledToken;
            styledToken.start = token.start;
            styledToken.length = token.length;
            styledToken.scope = token.getCombinedScope();

            // Resolve style using the token's scope
            auto themeStyle = resolveStyle(styledToken.scope);

            // Convert ThemeStyle to PlatformStyledToken style field
            styledToken.style = themeStyle;

            result.push_back(styledToken);
        }

        std::cout << "Generated " << result.size() << " styled tokens" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Failed to tokenize code with styles: " << e.what() << std::endl;
        // Fallback to single token
        result.push_back(createDefaultStyledToken(code));
    }

    return result;
}

ThemeStyle ShikiTextmateAdapter::resolveStyle(const std::string& scope) const {
    if (!registry_) {
        std::cerr << "Registry not initialized, cannot resolve style for scope: " << scope << std::endl;
        return getDefaultStyle();
    }

        try {
        // Use the stored theme object for proper vscode-textmate compatible theme resolution
        if (!currentTheme_) {
            fprintf(stderr, "[THEME DEBUG] No theme loaded for scope: %s\n", scope.c_str());
            return getDefaultStyle();
        }

        fprintf(stderr, "[THEME DEBUG] Resolving scope: '%s'\n", scope.c_str());

        // Use proper theme resolution via the Theme object
        auto styleAttrsOpt = currentTheme_->Match(scope);

        ThemeStyle style;
        style.background = "#1e1e1e";  // Default background
        style.bold = false;
        style.italic = false;
        style.underline = false;
        style.foreground = "#C9D1D9";  // Default foreground
        style.fontStyle = "normal";

        if (styleAttrsOpt.has_value()) {
            fprintf(stderr, "[THEME DEBUG] Match found for scope '%s'\n", scope.c_str());

            // Convert StyleAttributes to ThemeStyle using the color map
            auto colorMap = registry_->getColorMap();
            const auto& styleAttrs = styleAttrsOpt.value();

            fprintf(stderr, "[THEME DEBUG] StyleAttrs: fg_id=%d, bg_id=%d, font_style=%d\n",
                   styleAttrs.foreground_id, styleAttrs.background_id, static_cast<int>(styleAttrs.font_style));
            fprintf(stderr, "[THEME DEBUG] ColorMap size: %zu\n", colorMap.size());

            // Convert foreground color ID to hex color
            if (styleAttrs.foreground_id < colorMap.size()) {
                style.foreground = colorMap[styleAttrs.foreground_id];
                fprintf(stderr, "[THEME DEBUG] Resolved foreground: %s (from id %d)\n",
                       style.foreground.c_str(), styleAttrs.foreground_id);
            } else {
                fprintf(stderr, "[THEME DEBUG] Foreground ID %d out of bounds (colorMap size: %zu)\n",
                       styleAttrs.foreground_id, colorMap.size());
            }

            // Convert background color ID to hex color
            if (styleAttrs.background_id < colorMap.size()) {
                style.background = colorMap[styleAttrs.background_id];
                fprintf(stderr, "[THEME DEBUG] Resolved background: %s (from id %d)\n",
                       style.background.c_str(), styleAttrs.background_id);
            } else {
                fprintf(stderr, "[THEME DEBUG] Background ID %d out of bounds (colorMap size: %zu)\n",
                       styleAttrs.background_id, colorMap.size());
            }

            // Convert font style
            switch (styleAttrs.font_style) {
                case shikitori::FontStyle::Bold:
                    style.bold = true;
                    style.fontStyle = "bold";
                    fprintf(stderr, "[THEME DEBUG] Applied bold style\n");
                    break;
                case shikitori::FontStyle::Italic:
                    style.italic = true;
                    style.fontStyle = "italic";
                    fprintf(stderr, "[THEME DEBUG] Applied italic style\n");
                    break;
                case shikitori::FontStyle::Underline:
                    style.underline = true;
                    fprintf(stderr, "[THEME DEBUG] Applied underline style\n");
                    break;
                case shikitori::FontStyle::NotSet:
                default:
                    fprintf(stderr, "[THEME DEBUG] No special font style\n");
                    break;
            }
        } else {
            fprintf(stderr, "[THEME DEBUG] No match found for scope '%s', using defaults\n", scope.c_str());
        }

        return style;

    } catch (const std::exception& e) {
        std::cerr << "Error resolving style for scope " << scope << ": " << e.what() << std::endl;
        return getDefaultStyle();
    }
}

void ShikiTextmateAdapter::setConfiguration(const std::string& config) {
    // Apply configuration settings to the adapter
    // This could include cache settings, memory limits, etc.
}

// Helper method implementations

ThemeStyle ShikiTextmateAdapter::getDefaultStyle() const {
    ThemeStyle style;
    style.foreground = "#d4d4d4";  // VS Code default foreground
    style.background = "#1e1e1e";  // VS Code default background
    style.fontStyle = "normal";
    return style;
}

Token ShikiTextmateAdapter::createDefaultToken(const std::string& code, size_t line) const {
    Token token;
    token.start = 0;
    token.length = code.length();
    token.scopes = {currentGrammarScope_};
    token.style = getDefaultStyle();
    return token;
}

PlatformStyledToken ShikiTextmateAdapter::createDefaultStyledToken(const std::string& code) const {
    PlatformStyledToken token;
    token.start = 0;
    token.length = code.length();
    token.scope = currentGrammarScope_;
    token.style = getDefaultStyle();
    return token;
}

bool ShikiTextmateAdapter::testMethod() const {
    std::cout << "[C++ DEBUG] testMethod called!" << std::endl;
    std::cerr << "[C++ DEBUG] testMethod called via STDERR!" << std::endl;
    return true;
}

}  // namespace shiki

