#include "process_queries.h"

#include <vector>
#include <string>
#include <string_view>
#include <execution>

using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> result(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const string_view& query)
              { return search_server.FindTopDocuments(query); });

    return result;
}

vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
    vector<Document> result_joined;
    for (const auto& documents : ProcessQueries(search_server, queries)) {
        for (const auto& document : documents) {
            result_joined.push_back(document);
        }
    }

    return result_joined;
}