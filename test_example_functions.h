#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "read_input_functions.h"
#include "document.h"
#include "search_server.h"

void AddDocument(SearchServer &search_server, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings);

void GetDocumentId(const SearchServer &search_server, const int index);

void FindTopDocuments(const SearchServer &search_server, const std::string &raw_query);

//void SplitIntoWordsNoStop(const SearchServer &search_server, const string &text);

//void ParseQueryWord(const SearchServer &search_server, const string &text);


