#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <iterator>
#include <execution>
#include <string_view>

#include "concurrent_map.h"
#include "constants.h"
#include "document.h"
#include "string_processing.h"

//const int MAX_RESULT_DOCUMENT_COUNT = 5;
//constexpr double EPSILON() { return 1e-6; }

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
        if (!std::all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            using namespace std;
            throw invalid_argument("Стоп-слова содержат спецсимволы"s);
        }
    }

    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWordsView(stop_words_text)) {
    }

    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(
            SplitIntoWordsView(stop_words_text)) {
    }

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int> &ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;
    template <class ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy &policy, const std::string_view raw_query,
                                    DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy &policy, const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy &policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, 
                                                                        const std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, 
                                                                        const std::string_view raw_query, int document_id) const;

    std::set<int>::const_iterator begin();

    std::set<int>::const_iterator end();

    std::size_t size();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
    void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string document_text_;
    };
    
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;//хранится текст документа в виде string
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> id_with_word_and_freqs_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int> &ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(bool flag, const std::string_view text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query,
                                           DocumentPredicate document_predicate) const;

    template <class ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy &policy, const Query &query,
                                                         DocumentPredicate document_predicate) const;
};

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
                                    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(true, raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON()) {
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

template <class ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query,
                                    DocumentPredicate document_predicate) const {
    if (std::is_same_v<ExecutionPolicy, std::execution:: sequenced_policy>) {
        return FindTopDocuments(raw_query, document_predicate);
    } else {
        const auto query = ParseQuery(false, raw_query);

        auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);

        std::sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (std::abs(lhs.relevance - rhs.relevance) < EPSILON()) {
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
}

template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query, DocumentStatus status) const {
    if (std::is_same_v<ExecutionPolicy, std::execution:: sequenced_policy>) {
        return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    } else {
        return FindTopDocuments(std::execution::par,
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }
}

template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view raw_query) const {
    if (std::is_same_v<ExecutionPolicy, std::execution:: sequenced_policy>) {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    } else {
        return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <class ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query,
                                DocumentPredicate document_predicate) const {
    if (std::is_same_v<ExecutionPolicy, std::execution:: sequenced_policy>) {
            return FindAllDocuments(query, document_predicate);
    } else {
        std::map<int, double> document_to_relevance;
        ConcurrentMap<int, double> map_document_to_relevance(5);

        for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&, document_to_relevance](const std::string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    const auto& document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        map_document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });

        for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), [&](const std::string_view word) {
            if (word_to_document_freqs_.count(word) > 0) {
                for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                    map_document_to_relevance.Erase(document_id);
                }
            }
        });

        document_to_relevance = map_document_to_relevance.BuildOrdinaryMap();

        std::vector<Document> matched_documents(document_to_relevance.size);
        transform(std::execution::par, document_to_relevance.begin(), document_to_relevance.end(), matched_documents.begin(), [&](const auto &pair_doc) {
                Document doc(pair_doc.first, pair_doc.second, documents_.at(pair_doc.first).rating);
                return doc; });

        return matched_documents;
    }
}

