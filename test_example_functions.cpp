#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "test_example_functions.h"

using namespace std;

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const invalid_argument& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void GetDocumentId(const SearchServer& search_server, const int index) {
    try {
        search_server.GetDocumentId(index);
    } catch (const out_of_range& e) {
        cout << "Индекс переданного документа: "s << index << " выходит за пределы допустимого диапазона"s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const invalid_argument& e) {
        cout << "Ошибка в поисковом запросе: "s << e.what() << endl;
    }
}
/*
void SplitIntoWordsNoStop(const SearchServer& search_server, const string& text) {
    try {
        search_server.SplitIntoWordsNoStop(text);
    } catch (const invalid_argument& e) {
        cout << "Ошибка в поисковом запросе"s << e.what() << endl;
    }
}

void ParseQueryWord(const SearchServer& search_server, const string& text) {
    try {
        search_server.ParseQueryWord(text);
    } catch (const invalid_argument& e) {
        cout << "Ошибка в поисковом запросе"s << e.what() << endl;
    }
}
*/


