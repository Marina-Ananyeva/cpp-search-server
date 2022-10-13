#include <string>
#include <vector>
#include <deque>

#include "request_queue.h"

using namespace std;


RequestQueue::RequestQueue(const SearchServer& search_server) : request_queue_(search_server) {
}

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
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

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return AddFindRequest(raw_query, 
                        [status](int document_id, DocumentStatus document_status, int rating) {
                        return document_status == status;
                        });
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}
    
int RequestQueue::GetNoResultRequests() const {
    return static_cast<int>(requests_.size());
}

