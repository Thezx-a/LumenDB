#pragma once
#include <vector>
#include <functional>
#include <optional>
#include <random>

template <typename Key, typename Value, typename Compare = std::less<Key>>
class SkipList {
    struct Node {
        Key key;
        Value value;
        std::vector<Node*> next;
        Node(const Key& k, const Value& v, int height)
            : key(k), value(v), next(height, nullptr) {}
    };

public:
    explicit SkipList(int max_height = 12)
        : max_height_(max_height), head_(new Node(Key{}, Value{}, max_height)),
          rng_(std::random_device{}()), dist_(0.0, 1.0) {}

    ~SkipList() {
        Node* curr = head_;
        while (curr) {
            Node* next = curr->next[0];
            delete curr;
            curr = next;
        }
    }

    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    void Insert(const Key& key, const Value& value) {
        std::vector<Node*> update(max_height_, head_);
        Node* curr = head_;
        for (int i = max_height_ - 1; i >= 0; --i) {
            while (curr->next[i] && comp_(curr->next[i]->key, key)) {
                curr = curr->next[i];
            }
            update[i] = curr;
        }

        int height = randomHeight();
        Node* newNode = new Node(key, value, height);
        for (int i = 0; i < height; ++i) {
            newNode->next[i] = update[i]->next[i];
            update[i]->next[i] = newNode;
        }
    }

    std::optional<Value> Get(const Key& key) const {
        Node* curr = head_;
        for (int i = max_height_ - 1; i >= 0; --i) {
            while (curr->next[i] && comp_(curr->next[i]->key, key)) {
                curr = curr->next[i];
            }
        }
        curr = curr->next[0];
        if (curr && !comp_(key, curr->key) && !comp_(curr->key, key)) {
            return curr->value;
        }
        return std::nullopt;
    }

    void Scan(const Key& start, const Key& end,
              std::function<void(const Key&, const Value&)> callback) const {
        Node* curr = head_;
        for (int i = max_height_ - 1; i >= 0; --i) {
            while (curr->next[i] && comp_(curr->next[i]->key, start)) {
                curr = curr->next[i];
            }
        }
        curr = curr->next[0];
        while (curr && !comp_(end, curr->key)) {
            callback(curr->key, curr->value);
            curr = curr->next[0];
        }
    }

    size_t Size() const {
        size_t count = 0;
        Node* curr = head_->next[0];
        while (curr) {
            ++count;
            curr = curr->next[0];
        }
        return count;
    }

    bool Empty() const { return head_->next[0] == nullptr; }

private:
    int randomHeight() {
        int height = 1;
        while (height < max_height_ && dist_(rng_) < 0.5) {
            ++height;
        }
        return height;
    }

    int max_height_;
    Node* head_;
    Compare comp_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;
};
