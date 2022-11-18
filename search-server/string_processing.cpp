#include "string_processing.h"

#include <iostream>

using namespace std;

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

vector<string_view> SplitIntoWordsView(const string_view text) {
    string_view str{text};
    vector<string_view> result;
    str.remove_prefix(min(str.find_first_not_of(" "), str.size()));
    int64_t pos = str.find_first_not_of(" ");
    const int64_t pos_end = str.npos;
    
    while (pos != pos_end) {
        int64_t space = str.find(' ', pos);
        result.push_back(space == pos_end ? str.substr(pos) : str.substr(pos, space - pos));
        pos = str.find_first_not_of(" ", space);
    }

    return result;
}