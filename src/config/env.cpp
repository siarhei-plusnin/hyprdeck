#include "env.hpp"

#include <cctype>
#include <cstdlib>

namespace hyprdeck::env {
    namespace {

        bool nameCharacter(const char character) {
            return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
        }

        std::string variableValue(std::string_view name) {
            if (name.empty())
                return "";

            const auto variable = std::string{name};
            if (const auto* result = std::getenv(variable.c_str()))
                return result;

            return "";
        }

    } // namespace

    std::string expandVariables(std::string_view raw) {
        std::string expanded;
        expanded.reserve(raw.size());

        for (size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] != '$') {
                expanded.push_back(raw[i]);
                continue;
            }

            if (i + 1 >= raw.size()) {
                expanded.push_back(raw[i]);
                continue;
            }

            if (raw[i + 1] == '{') {
                const auto end = raw.find('}', i + 2);
                if (end == std::string_view::npos) {
                    expanded.push_back(raw[i]);
                    continue;
                }

                expanded += variableValue(raw.substr(i + 2, end - i - 2));
                i = end;
                continue;
            }

            if (!nameCharacter(raw[i + 1])) {
                expanded.push_back(raw[i]);
                continue;
            }

            size_t end = i + 1;
            while (end < raw.size() && nameCharacter(raw[end]))
                ++end;

            expanded += variableValue(raw.substr(i + 1, end - i - 1));
            i = end - 1;
        }

        return expanded;
    }

} // namespace hyprdeck::env
