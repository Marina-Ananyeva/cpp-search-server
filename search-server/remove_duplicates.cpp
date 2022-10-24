#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    map<int, set<string>> id_documents_with_words;

    for (const int document_id : search_server) {
        map<string, double> word_freq = search_server.GetWordFrequencies(document_id);
        for (const auto& [word, tf] : word_freq) {
            id_documents_with_words[document_id].insert(word);
        }
    }

    set<int> id_duplicates;
    int id_duplicate;
    for (int i = 0; i < (id_documents_with_words.size() - 1); ++i) {
        for (int j = i + 1; j < id_documents_with_words.size(); ++j) {
            if (id_documents_with_words[i] == id_documents_with_words[j] && i != j) {
                id_duplicate = *(next(search_server.begin(), j - 1));
                id_duplicates.insert(id_duplicate);
            }
        }
    }

    for (const auto& id : id_duplicates) {
        cout << "Found duplicate document id "s << id << endl;
        search_server.RemoveDocument(id);
    }
}