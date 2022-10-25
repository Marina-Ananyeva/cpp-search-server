#pragma once

#include "document.h"
#include "search_server.h"

#include <string>
#include <vector>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer &search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string &raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string &raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int request_time = 0;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& request_queue_;
    QueryResult request_;
}; 

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    ++request_.request_time;
    if (request_.request_time > min_in_day_) {
    requests_.pop_front();
    }
    requests_.push_back(request_);
    const auto request_result_ = request_queue_.FindTopDocuments(raw_query, document_predicate);
    if (!request_result_.empty()) {
        requests_.pop_back();
    }
    return request_result_;
}
