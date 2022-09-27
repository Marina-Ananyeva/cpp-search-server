#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"


using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;
constexpr double EPSILON() { return 1e-6; }

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON()) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus status_, int rating) {
            return status_ == status; 
            });
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
    
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            if (predicate (document_id, documents_.at(document_id).status, documents_.at(document_id).rating) == true) {    
            matched_documents.push_back(
                {document_id, relevance, documents_.at(document_id).rating});
            }
        }
        return matched_documents;
    }
};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(const Func& func, const string& expr) {
    func();
    cerr << expr << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl ((func), (#func))
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система исключает минус-слова при поиске документов
void TestExcludeMinusWordsFromFindDocuments() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_EQUAL(server.FindTopDocuments("in"s).size(), 1u);
        ASSERT_EQUAL(server.FindTopDocuments("in"s)[0].id, doc_id);
    }
    
    {
        SearchServer server;
        string query_minus_Word = "-cat"s;        
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("-cat"s).empty(), 
                    "Documents with minus words must be excluded from the search"s);
    }
}

// Тест проверяет, что поисковая система корректно мэтчит слова из поиска со словами из документа и исключает все слова документа
// при совпадении хоть одного с минус-словом
void TextMatchingWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        vector<string> test_match_words = {"cat"s};
        const auto [match_words, status] = server.MatchDocument("cat mat rat"s, doc_id);
        ASSERT(match_words == test_match_words);
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [match_words, status] = server.MatchDocument("-cat city"s, doc_id);
        ASSERT_HINT(match_words.empty(), "Minus words must be excluded from the query"s);
    }
}

//Тест проверяет, что документы правильно сортируются по возрастанию релевантности
    void TestSortDocumentsByRelevance() {
        const int doc_id_1 = 42;
        const string content_1 = "cat in the city"s;
        const vector<int> ratings_1 = {1, 2, 3};

        const int doc_id_2 = 41;
        const string content_2 = "cat in the city and country"s;
        const vector<int> ratings_2 = {1, 2, 3};

        const int doc_id_3 = 40;
        const string content_3 = "dog in the town"s;
        const vector<int> ratings_3 = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
            server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
            server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings_3);
            ASSERT_EQUAL_HINT(server.FindTopDocuments("cat city country"s)[0].id, doc_id_2, 
                                "Documents sort by relevance incorrectly");
        }
    }

//Тест проверяет, что средний рейтинг документа считается корректно (как среднеарифметическое из заданных при вводе документа значений)
    void TestCalculationRatingDocument() {
        const int doc_id = 42;
        const string content = "cat in the city"s;
        const vector<int> ratings = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            ASSERT_EQUAL(server.FindTopDocuments("cat"s)[0].rating, ((ratings[0] + ratings[1] + ratings[2]) / 3));
        }
    }

//Тест проверяет, что документы ищутся по предикату
    void TestUsingPredicate() {
        const int doc_id = 42;
        const string content = "cat in the city"s;
        const vector<int> ratings = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
            ASSERT(server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; }).empty());
        }
    }

//Тест проверяет, что документы ищутся по статусу
    void TestStatusFindDocuments() {
        const int doc_id_1 = 42;
        const string content_1 = "cat in the city"s;
        const vector<int> ratings_1 = {1, 2, 3};

        const int doc_id_2 = 41;
        const string content_2 = "cat in the city and country"s;
        const vector<int> ratings_2 = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
            server.AddDocument(doc_id_2, content_2, DocumentStatus::BANNED, ratings_2);
            ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL).size(), 1U);
            ASSERT(server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL)[0].id == doc_id_1);
        }
    }

//Тест проверяет корректность расчета релевантности документов
    void TestCalculationRelevanceDocument() {
        const int doc_id_1 = 42;
        const string content_1 = "cat in the city"s;
        const vector<int> ratings_1 = {1, 2, 3};

        const int doc_id_2 = 41;
        const string content_2 = "dog in the city"s;
        const vector<int> ratings_2 = {1, 2, 3};
        {
            SearchServer server;
            server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
            server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings_2);
            const string term = "cat";
            vector<double> tf_idfs;
            double tf = 0.0;
            double idf = 0.0;
            int document_freq = 0;
            const vector<vector<string>> documents = {
                {"cat"s, "in"s, "the"s, "city"s},
                {"dog"s, "in"s, "the"s, "city"s}
            }; 
            for (const auto& document : documents) {
                if ((static_cast<int>(count(document.begin(), document.end(), term))) > 0) {
            document_freq += 1;
                } 
            }
            for (const auto& document : documents) {
                idf = log ((documents.size() * 1.0) / document_freq);
                tf = static_cast<int>(count(document.begin(), document.end(), term)) * 1.0 / static_cast<int>(document.size());
                tf_idfs.push_back(tf * idf);
            } 
       
        ASSERT((abs(server.FindTopDocuments("cat"s)[0].relevance - tf_idfs[0]) < EPSILON())); 
        }
    }

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromFindDocuments);
    RUN_TEST(TextMatchingWords);
    RUN_TEST(TestSortDocumentsByRelevance);
    RUN_TEST(TestCalculationRatingDocument);
    RUN_TEST(TestUsingPredicate);
    RUN_TEST(TestStatusFindDocuments);
    RUN_TEST(TestCalculationRelevanceDocument);
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}