#include "remove_duplicates.h"

#include <map>
#include <set>
#include <string>
#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    map<set<string_view>, int> id_documents_with_words; //в словаре автоматически останутся только недублированные наборы слов и id документов
    set<string_view> set_of_words; //собираем наборы слов - ключи словаря
    set<int> id_duplicates; //собираем id дубликатов
    for (const int document_id : search_server) {
        map<string_view, double> word_freq = search_server.GetWordFrequencies(document_id);
        for (const auto& [word, tf] : word_freq) {
            set_of_words.insert(word);
        }
        if (id_documents_with_words.count(set_of_words) == 0) {
            id_documents_with_words.insert({set_of_words, document_id});
        } else {
            id_duplicates.insert(document_id);
            }
        set_of_words.clear();
    }
    
    for (const auto& id : id_duplicates) {
        cout << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}
