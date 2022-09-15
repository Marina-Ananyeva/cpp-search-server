#include <algorithm>

#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
  string s;
  getline(cin, s);
  return s;
}

int ReadLineWithNumber() {
  int result = 0;
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
};

class SearchServer {
public:
  void SetStopWords(const string& text) {
    for (const string& word : SplitIntoWords(text)) {
      stop_words_.insert(word);
    }
  }
   
  void AddDocument(int document_id, const string& document) {
    ++document_count_;
    const vector<string> words = SplitIntoWordsNoStop(document);        
    const double tf = 1.0 / words.size();
    for (const auto& word : words) {
      documents_tf[word][document_id] += tf;
    }
  }
   
  vector<Document> FindTopDocuments(const string& raw_query) const {
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query);
    sort(matched_documents.begin(), matched_documents.end(),
      [](const Document& lhs, const Document& rhs) {
        return lhs.relevance > rhs.relevance;
      });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
      matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
  }

private:
  int document_count_ = 0;
  set<string> stop_words_;
  map<string, map<int, double>> documents_tf;

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

  struct Query {
    set<string> plus_words;
    set<string> minus_words;
  };
   
  Query ParseQuery(string text) const {
    Query query;
    for (auto& s : SplitIntoWords(text)) {
      if (s[0] == '-') {
        s = s.substr(1, s.size() - 1); 
        if (!IsStopWord(s)) {
          query.minus_words.insert(s);    
        } else continue;                  
      } else if (!IsStopWord(s)) {
          query.plus_words.insert(s);
        }   
    }        
    return query;          
  }  
        
  vector<Document> FindAllDocuments(const Query& query) const {    
    map<int, double> index;
    for (const string &word : query.plus_words) {
      if (documents_tf.count(word) == 0) {
        continue;
      }
      const double idf = log(static_cast <double> (document_count_)/ documents_tf.at(word).size());         
        for (const auto [document_id, tf] : documents_tf.at(word)) {
          index[document_id] += tf * idf;
        }
    }
      for (const string& word : query.minus_words) {
        if (documents_tf.count(word) == 0) {
          continue;
        }
        for (const auto [document_id, tf] : documents_tf.at(word)) {
          index.erase(document_id);
        }
      }    
    vector<Document> matched_documents;    
      for (const auto& [document_id, relevance] : index) {
        matched_documents.push_back({document_id, relevance});
      }
      return matched_documents;
  }  
};

SearchServer CreateSearchServer() {
  SearchServer search_server;
  search_server.SetStopWords(ReadLine());
  int document_count = ReadLineWithNumber();
  for (int document_id = 0; document_id < document_count; ++document_id) {
    search_server.AddDocument(document_id, ReadLine());        
  }
  return search_server;
}

int main() {
  const SearchServer search_server = CreateSearchServer();

  const string raw_query = ReadLine();
  for (auto& [document_id, relevance] : search_server.FindTopDocuments(raw_query)) {
    cout << "{ document_id = "s << document_id << ", "
         << "relevance = "s << relevance << " }"s << endl;
  }   
}
