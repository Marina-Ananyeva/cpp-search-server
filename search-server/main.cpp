#include "constants.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "log_duration.h"
#include "remove_duplicates.h"

using namespace std;

int main() {
    TestSearchServer();

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

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
    /*
    SearchServer search_server1("and in at"s);
    RequestQueue request_queue(search_server1);
    search_server1.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server1.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server1.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server1.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server1.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    
    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    {
        LOG_DURATION_STREAM("Operation time", cout);
        const string query = "curly -dog"s;
        cout << "Матчинг документов по запросу: "s << query << endl;
        for (int i = 0; i < search_server1.GetDocumentCount(); ++i)
        {
            int document_id = *next(search_server1.begin(), i);
            const auto [words, status] = search_server1.MatchDocument(query, document_id);

            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    {
        LOG_DURATION_STREAM("Operation time", cout);
        const string query = "big -collar"s;
        cout << "Результаты поиска по запросу: "s << query << endl;
        const auto documents = search_server1.FindTopDocuments(query); 
        for (const auto& document : documents) {
            PrintDocument(document);
        }
    }
    */
    return 0;
} 
/*
Пример вывода
Total empty requests: 1437 
*/

/*
Before duplicates removed: 9
Found duplicate document id 3
Found duplicate document id 4
Found duplicate document id 5
Found duplicate document id 7
After duplicates removed: 5 
*/
/*
Матчинг документов по запросу: пушистый -пёс
{ document_id = 1, status = 0, words = пушистый}
...
{ document_id = 4, status = 0, words =}
Operation time: 15 ms
...
Результаты поиска по запросу: пушистый -кот
{ document_id = 1, relevance = 0.143844, rating = 5 }
...
{ document_id = 17, relevance = 0.143844, rating = 5 }
Operation time: 76 ms 
*/

