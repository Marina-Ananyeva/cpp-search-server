#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "document.h"
#include "paginator.h"

std::string ReadLine();

int ReadLineWithNumber();

std::ostream &operator<<(std::ostream &os, const Document &document);

void PrintDocument(const Document &document);

void PrintMatchDocumentResult(int document_id, const std::vector<std::string> &words, DocumentStatus status);

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& page) {
    for (Iterator it = page.begin(); it < page.end(); advance(it, 1)) {
        os << *it;
    }
    return os;
}