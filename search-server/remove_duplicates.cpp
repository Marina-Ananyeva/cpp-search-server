#include "remove_duplicates.h"

#include <map>
#include <set>
#include <string>
#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    map<set<string>, int> id_documents_with_words; //в словаре автоматически останутся только недублированные наборы слов и id документов
    set<string> set_of_words; //собираем наборы слов - ключи словаря

    for (const int document_id : search_server) {
        map<string, double> word_freq = search_server.GetWordFrequencies(document_id);
        for (const auto& [word, tf] : word_freq) {
            set_of_words.insert(word);
        }
        id_documents_with_words.insert({set_of_words, document_id});
        set_of_words.clear();
    }

    set<int> id_without_duplicates; //собираем id оригинальных документов
    for (const auto [words, id] : id_documents_with_words) {
        id_without_duplicates.insert(id);
    }

    set<int> id_duplicates; //собираем id дубликатов
    for (const int document_id : search_server) {
        if (!id_without_duplicates.count(document_id)) {
            id_duplicates.insert(document_id);
        }
    }

    for (const auto& id : id_duplicates) {
        cout << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}