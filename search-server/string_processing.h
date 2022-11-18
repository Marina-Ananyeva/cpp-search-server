#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <string_view>

std::vector<std::string> SplitIntoWords(const std::string &text);

std::vector<std::string_view> SplitIntoWordsView(const std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.emplace(str);
        }
    }
    return non_empty_strings;
}