#pragma once

#include "read_input_functions.h"
#include "document.h"
#include "search_server.h"
#include "log_duration.h"
#include "remove_duplicates.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cassert>

void AddDocument(SearchServer &search_server, int document_id, const std::string &document, DocumentStatus status,
                 const std::vector<int> &ratings);

void FindTopDocuments(const SearchServer &search_server, const std::string &raw_query);

void BeginEndSizeTest();

void TestGetWordFrequencies();

void RemoveDocument(SearchServer &search_server, int document_id);

void TestRemoveDocument();

void TestRemoveDuplicates();

void TestSearchServer();



