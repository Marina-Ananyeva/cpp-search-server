#include "search_server.h"
#include "log_duration.h"

#include <execution>
#include <map>
#include <string_view>

using namespace std;

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status,
                 const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Ошибка добавления документа"s);
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status, string(move(document))});

    const vector<string_view> words = SplitIntoWordsNoStop(documents_[document_id].document_text_);

    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_with_word_and_freqs_[document_id][word] += inv_word_count;
    }

    document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}
vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query,
                                                    int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}

//последовательный метод
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, 
                                                                        const string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) {
        throw out_of_range ("Document id is not valid"s);
    }

    const auto query = ParseQuery(true, raw_query);

    vector<string_view> matched_words;

    for (const string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return {vector<string_view> {}, documents_.at(document_id).status};
        }
    }

    for (const string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return {matched_words, documents_.at(document_id).status};
}

//параллельный метод
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, 
                                                                        const string_view raw_query, int document_id) const {
    if (document_ids_.count(document_id) == 0) {
        throw out_of_range ("Document id is not valid"s);
    }

    const auto query = ParseQuery(false, raw_query);

    if (any_of(execution::par, query.minus_words.begin(), query.minus_words.end(), [&, document_id](const string_view word) 
            { if (word_to_document_freqs_.count(word) > 0) {
                return word_to_document_freqs_.at(word).count(document_id) > 0;
            }
                return false; })) {
        return {vector<string_view> {}, documents_.at(document_id).status};
    }

    vector<string_view> matched_words(query.plus_words.size());

    auto it = copy_if(execution::par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&, document_id](const string_view word)
              { if (word_to_document_freqs_.count(word) > 0) {
                    return word_to_document_freqs_.at(word).count(document_id) > 0;
                }
                    return false;});
    
    sort(execution::par, matched_words.begin(), it);
    auto last = unique(execution::par, matched_words.begin(), it);
    matched_words.erase(last, matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

set<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}

size_t SearchServer::size() {
    return document_ids_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty_frequencies;
    if (!id_with_word_and_freqs_.count(document_id)) {
        return empty_frequencies;
    }

    return id_with_word_and_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(execution::seq, document_id);
}

//последовательный метод
void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
        if (document_ids_.count(document_id) == 0) {
        throw invalid_argument("Document id is not valid"s);
    }

    for (const auto &[word, tf] : id_with_word_and_freqs_[document_id]) {
        word_to_document_freqs_[word].erase(document_id);
    }

    id_with_word_and_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

//параллельный метод
void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (document_ids_.count(document_id) == 0) {
        throw invalid_argument("Document id is not valid"s);
    }
        vector<const string_view*> remove_words(id_with_word_and_freqs_[document_id].size());
        transform(execution::par, id_with_word_and_freqs_[document_id].begin(), id_with_word_and_freqs_[document_id].end(), remove_words.begin(), [](const auto &it)
                  { auto p = &it.first;
                    return p; });

        for_each(execution::par, remove_words.begin(), remove_words.end(), [&, document_id](const auto &it) 
        { word_to_document_freqs_[*it].erase(document_id); });

        id_with_word_and_freqs_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Words in document not valid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word{text};
    
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(bool flag, const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }

    Query result;
    for (const string_view word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    //для параллельного метода Match не делаем сортировку в ParseQuery
    if(flag){
        sort(result.minus_words.begin(), result.minus_words.end());
        auto it1 = unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(it1, result.minus_words.end());
 
        sort(result.plus_words.begin(), result.plus_words.end());
        auto it2 = unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(it2, result.plus_words.end());
    }

    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

