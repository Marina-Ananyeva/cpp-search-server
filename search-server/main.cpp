#include "concurrent_map.h"
#include "constants.h"
#include "document.h"
#include "log_duration.h"
#include "paginator.h"
#include "process_queries.h"
#include "read_input_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"
#include "test_example_functions.h"

#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}
#define TEST(policy) Test(#policy, search_server, queries, execution::policy)

int main() {
    //TestSearchServer();
    {
        SearchServer search_server("and with"s);
        
        int id = 0;
        for (
            const string& text : {
                "white cat and yellow hat"s,
                "curly cat curly tail"s,
                "nasty dog with big eyes"s,
                "nasty pigeon john"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }
        cout << "ACTUAL by default:"s << endl;
        // последовательная версия
        for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
            PrintDocument(document);
        }
        cout << "BANNED:"s << endl;
        //последовательная версия
        for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        //    PrintDocument(document);
        }
        cout << "Even ids:"s << endl;
        // параллельная версия
        for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
            PrintDocument(document);
        }
    }
/*
    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 100, 70);
        TEST(seq);
        TEST(par);
    }
*/
/*
    SearchServer search_server("and with"sv);
    //SearchServer search_server("and with"s);
    
        int id = 0;
    for (
        const string_view& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string_view query{"curly and funny -not"};

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl; // ожидаем 1
        for (auto w : words) {
            std::cout << w << " ";
        }
        std::cout << std::endl;
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2"s << endl; // ожидаем 2
        for (auto w : words) {
            std::cout << w << " ";
        }
        std::cout << std::endl;
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl; // ожидаем 0
        for (auto w : words) {
            std::cout << w << " ";
        }
        std::cout << std::endl;
    }
*/
/*
    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }

        const string query = "curly curly and funny -not"s;

        {
            const auto [words, status] = search_server.MatchDocument(query, 1);
            cout << words.size() << " words for document 1"s << endl;
            // 1 words for document 1

        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
            cout << words.size() << " words for document 2"s << endl;
            // 2 words for document 2
            for (auto w : words) {
            std::cout << w << " ";
        }
            std::cout << std::endl;
        }

        {
            const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
            cout << words.size() << " words for document 3"s << endl;
            // 0 words for document 3
        }
    }
*/
/*
    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

        const string query = GenerateQuery(generator, dictionary, 500, 0.1);

        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }

        TEST(seq);
        TEST(par);
    }
*/
/*
    {
        SearchServer search_server("and with"s);

        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }

        const string query = "curly and funny"s;

        auto report = [&search_server, &query] {
            cout << search_server.GetDocumentCount() << " documents total, "s
                << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
        };

        report();
        // однопоточная версия
        search_server.RemoveDocument(5);
        report();
        // однопоточная версия
        search_server.RemoveDocument(execution::seq, 1);
        report();
        // многопоточная версия
        search_server.RemoveDocument(execution::par, 2);
        report();
    }

    {
        mt19937 generator;

        const auto dictionary = GenerateDictionary(generator, 10'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

        {
            SearchServer search_server(dictionary[0]);
            for (size_t i = 0; i < documents.size(); ++i) {
                search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
            }

            TEST(seq);
        }
        {
            SearchServer search_server(dictionary[0]);
            for (size_t i = 0; i < documents.size(); ++i) {
                search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
            }

            TEST(par);
        }
    } 
*/
/*
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }
        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };
        for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
            cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
        }
    }

    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        TEST(ProcessQueriesJoined);
    }
*/
/*
    {
        SearchServer search_server("and with"s);
        int id = 0;
        for (
            const string& text : {
                "funny pet and nasty rat"s,
                "funny pet with curly hair"s,
                "funny pet and not very nasty rat"s,
                "pet with rat and rat and rat"s,
                "nasty rat with curly hair"s,
            }
        ) {
            search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
        }
        const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
        };

        id = 0;
        for (const auto& documents : ProcessQueries(search_server, queries)) {
            cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
        }
    }
    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 2'000, 25);
        const auto documents = GenerateQueries(generator, dictionary, 20'000, 10);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 2'000, 7);
        TEST(ProcessQueries);
    }
*/

/*
SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});//вызываем тестовую функцию, через нее вызовется метод
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    

    //FindTopDocuments(search_server, ""s); //проверка невалидного запроса
    //RemoveDocument(search_server, 0);//проверка невалидного документа

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
   
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

//Пример вывода
/*
ACTUAL by default:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 }
{ document_id = 1, relevance = 0.173287, rating = 1 }
{ document_id = 3, relevance = 0.173287, rating = 1 }
BANNED:
Even ids:
{ document_id = 2, relevance = 0.866434, rating = 1 }
{ document_id = 4, relevance = 0.231049, rating = 1 } 
*/
/*
1 words for document 1
2 words for document 2
0 words for document 3 
*/
/*
5 documents total, 4 documents for query [curly and funny]
4 documents total, 3 documents for query [curly and funny]
3 documents total, 2 documents for query [curly and funny]
2 documents total, 1 documents for query [curly and funny] 
*/
/*
Document 1 matched with relevance 0.183492
Document 5 matched with relevance 0.183492
Document 4 matched with relevance 0.167358
Document 3 matched with relevance 0.743945
Document 1 matched with relevance 0.311199
Document 2 matched with relevance 0.183492
Document 5 matched with relevance 0.127706
Document 4 matched with relevance 0.0557859
Document 2 matched with relevance 0.458145
Document 5 matched with relevance 0.458145 
*/
/*
3 documents for query [nasty rat -not]
5 documents for query [not very funny nasty pet]
2 documents for query [curly hair] 
*/
/*
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

