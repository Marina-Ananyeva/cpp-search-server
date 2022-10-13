#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>
#include <iterator>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }

    Iterator begin() const {
        return first_;
    }

    Iterator end() const {
        return last_;
    }

    size_t size() const {
        return size_;
    }

private:
    Iterator first_, last_;
    size_t size_;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (Iterator it = begin; it != end; ) {
            const size_t current_page_size = distance(it, end);
            if (current_page_size >= page_size) {
                pages_.push_back(IteratorRange(it, next(it, page_size)));
                advance(it, page_size);
            } else {
                    pages_.push_back(IteratorRange(it, next(it, current_page_size)));
                    advance(it, current_page_size);
                }
        }
    }

    auto begin() const {
        return pages_.begin();
    }
 
    auto end() const {
        return pages_.end();
    }
 
    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}