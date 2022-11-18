#pragma once

#include <mutex>
#include <future>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex_map;
        std::map<Key, Value> map;
    };
 
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);
 
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
 
        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex_map)
            , ref_to_value(bucket.map[key]) {
        }
    };
 
    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }
 
    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {key, bucket};
    }
 
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex_map, map] : buckets_) {
            std::lock_guard guard(mutex_map);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    void Erase(const Key& key) {
        buckets_[static_cast<uint64_t>(key) % buckets_.size()].map.erase(key);
    }

private:
    std::vector<Bucket> buckets_;
};