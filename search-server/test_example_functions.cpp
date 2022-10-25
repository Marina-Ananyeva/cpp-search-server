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

void BeginEndSizeTest() {
    SearchServer search_server("and in at"s);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    assert(*(search_server.begin()) == 1);
    assert(*prev(search_server.end()) == 5);
    assert(search_server.size() == 5);
}

void TestGetWordFrequencies() {
    SearchServer search_server("and in at"s);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    map<string, double> test;
    {
    LOG_DURATION_STREAM("Operation GetWordFrequencies time", cout);
    test = search_server.GetWordFrequencies(2);
    }
    const auto it = next(test.begin(), 1);

    assert(test.size() == 4);
    assert(it->first == "curly"s);
    assert(it->second == 0.25);
}

void RemoveDocument(SearchServer& search_server, int document_id) {
    cout << "ok"s << endl;
    try {
        search_server.RemoveDocument(document_id);
    } catch (const invalid_argument& e) {
        cout << "Некорректный id документа "s << document_id << ": "s << e.what() << endl;
    }
}

void TestRemoveDocument() {
    SearchServer search_server("and in at"s);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    {
    LOG_DURATION_STREAM("Operation RemoveDocument time", cout);
    search_server.RemoveDocument(2);
    }
    const auto it = next(search_server.begin());
    assert(search_server.size() == 4);
    assert(*it == 3);
}

void TestRemoveDuplicates() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    {
    LOG_DURATION_STREAM("Operation RemoveDuplicates time", cout);
    RemoveDuplicates(search_server);
    }
    set<int> test1 = {1, 2, 6, 8, 9};
    set<int> test2;
    auto it = search_server.begin();
    for (int i = 0; i < search_server.size(); ++i) {
    test2.insert(*it);
    it = next(it);
    }
    assert(search_server.size() == 5);
    assert(test1 == test2);
}

void TestSearchServer() {
    BeginEndSizeTest();
    TestGetWordFrequencies();
    TestRemoveDocument();
    TestRemoveDuplicates();

    cout << "TestSearchServer is ok"s << endl;
}
/*
void GetDocumentId(const SearchServer& search_server, const int index) {
    try {
        search_server.GetDocumentId(index);
    } catch (const out_of_range& e) {
        cout << "Индекс переданного документа: "s << index << " выходит за пределы допустимого диапазона"s << e.what() << endl;
    }
}
*/

/*
void SplitIntoWordsNoStop(const SearchServer& search_server, const string& text) {
    try {
        search_server.SplitIntoWordsNoStop(text);
    } catch (const invalid_argument& e) {
        cout << "Ошибка в документе"s << e.what() << endl;
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


