// LinkedList.h
// A hand-written DOUBLY linked list (no std::list).
//
//   head_ <-> [A] <-> [B] <-> [C] <-> tail_
//
// Each node owns its data and points to its neighbours. The list owns every
// node and deletes them in its destructor (RAII), so there are no leaks.
// Used for: detected violations, audit history and repair execution logs.
//
// Operation costs (n = number of nodes):
//   pushFront / pushBack ... O(1)      insertAt ... O(n)
//   find (linear search) ... O(n)      removeAt / removeIf ... O(n)
//   traversal (forEach) .... O(n)
#pragma once

#include <cstddef>
#include <utility>
#include <vector>

template <typename T>
class DoublyLinkedList {
    struct Node {
        T data;
        Node* prev;
        Node* next;
        explicit Node(const T& value) : data(value), prev(nullptr), next(nullptr) {}
    };

public:
    // --- forward iterator so the list works in range-based for loops ---
    class ConstIterator {
    public:
        explicit ConstIterator(const Node* node) : node_(node) {}
        const T& operator*() const { return node_->data; }
        const T* operator->() const { return &node_->data; }
        ConstIterator& operator++() { node_ = node_->next; return *this; }
        bool operator!=(const ConstIterator& other) const { return node_ != other.node_; }
        bool operator==(const ConstIterator& other) const { return node_ == other.node_; }
    private:
        const Node* node_;
    };

    DoublyLinkedList() = default;
    ~DoublyLinkedList() { clear(); }

    // Deep copy: every node is duplicated, so two lists never share nodes.
    DoublyLinkedList(const DoublyLinkedList& other) {
        for (const Node* n = other.head_; n != nullptr; n = n->next) pushBack(n->data);
    }
    DoublyLinkedList(DoublyLinkedList&& other) noexcept { swap(other); }
    DoublyLinkedList& operator=(DoublyLinkedList other) {  // copy-and-swap
        swap(other);
        return *this;
    }

    void swap(DoublyLinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    // ---------- insertion ----------
    void pushBack(const T& value) {
        Node* node = new Node(value);
        if (tail_ == nullptr) {
            head_ = tail_ = node;
        } else {
            node->prev = tail_;
            tail_->next = node;
            tail_ = node;
        }
        ++size_;
    }

    void pushFront(const T& value) {
        Node* node = new Node(value);
        if (head_ == nullptr) {
            head_ = tail_ = node;
        } else {
            node->next = head_;
            head_->prev = node;
            head_ = node;
        }
        ++size_;
    }

    // Insert so that the new element ends up at `index` (0 = front, size() = back).
    bool insertAt(std::size_t index, const T& value) {
        if (index > size_) return false;
        if (index == 0) { pushFront(value); return true; }
        if (index == size_) { pushBack(value); return true; }
        Node* after = nodeAt(index - 1);  // node that will precede the new one
        Node* node = new Node(value);
        node->prev = after;
        node->next = after->next;
        after->next->prev = node;
        after->next = node;
        ++size_;
        return true;
    }

    // ---------- linear search ----------
    // Walks node by node from the head and returns the first match (or nullptr).
    // `comparisons` (optional) receives how many nodes were examined.
    template <typename Pred>
    T* find(Pred matches, std::size_t* comparisons = nullptr) {
        std::size_t steps = 0;
        for (Node* n = head_; n != nullptr; n = n->next) {
            ++steps;
            if (matches(n->data)) {
                if (comparisons) *comparisons = steps;
                return &n->data;
            }
        }
        if (comparisons) *comparisons = steps;
        return nullptr;
    }
    template <typename Pred>
    const T* find(Pred matches, std::size_t* comparisons = nullptr) const {
        return const_cast<DoublyLinkedList*>(this)->find(matches, comparisons);
    }

    T* at(std::size_t index) { return index < size_ ? &nodeAt(index)->data : nullptr; }
    const T* at(std::size_t index) const { return index < size_ ? &nodeAt(index)->data : nullptr; }

    // ---------- deletion ----------
    bool removeAt(std::size_t index) {
        if (index >= size_) return false;
        unlink(nodeAt(index));
        return true;
    }

    // Deletes every node for which `matches` is true; returns how many were removed.
    template <typename Pred>
    std::size_t removeIf(Pred matches) {
        std::size_t removed = 0;
        Node* n = head_;
        while (n != nullptr) {
            Node* next = n->next;  // remember before n is deleted
            if (matches(n->data)) {
                unlink(n);
                ++removed;
            }
            n = next;
        }
        return removed;
    }

    void clear() {
        Node* n = head_;
        while (n != nullptr) {
            Node* next = n->next;
            delete n;
            n = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    // ---------- traversal ----------
    template <typename Fn>
    void forEach(Fn fn) const {
        for (const Node* n = head_; n != nullptr; n = n->next) fn(n->data);
    }

    std::vector<T> toVector() const {
        std::vector<T> out;
        out.reserve(size_);
        for (const Node* n = head_; n != nullptr; n = n->next) out.push_back(n->data);
        return out;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    ConstIterator begin() const { return ConstIterator(head_); }
    ConstIterator end() const { return ConstIterator(nullptr); }

private:
    Node* nodeAt(std::size_t index) const {
        Node* n = head_;
        for (std::size_t i = 0; i < index; ++i) n = n->next;
        return n;
    }

    // Detach a node from its neighbours, fix head_/tail_, then delete it.
    void unlink(Node* node) {
        if (node->prev) node->prev->next = node->next; else head_ = node->next;
        if (node->next) node->next->prev = node->prev; else tail_ = node->prev;
        delete node;
        --size_;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
    std::size_t size_ = 0;
};
