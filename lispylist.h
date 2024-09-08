// `LispyList<Value>` is an immutable singly linked list of `const Value`.
//
// Each element in the list is reference counted. An element is destroyed when
// no more `LispyList` instances contain it.
//
// For example:
//
//     // y := 7 6 5
//     //           \.
//     //      x := 4 3 2 1
//     //           /
//     //   z := 9 8
//     {
//        using List = LispyList<int>;
//        List x = List().prepend(1).prepend(2).prepend(3).prepend(4);
//        List y = x.prepend(5).prepend(6).prepend(7);
//        List z = x.prepend(8).prepend(9);
//
//        x = List(); // This will destroy nothing.
//        y = List(); // This will destroy 7, 6, and 5.
//      } // This will destroy 9, 8, 4, 3, 2, and 1.

#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>

template <typename Value>
class LispyList;
template <typename Value>
class LispyListIterator;
template <typename Value>
class LispyListNode;

template <typename Value>
class LispyList {
  LispyListNode<Value> *node;
  explicit LispyList(LispyListNode<Value>*);
  void cleanup();

 public:
  LispyList();
  LispyList(const LispyList&);
  LispyList(LispyList&&);

  ~LispyList();

  LispyList& operator=(const LispyList& other);
  LispyList& operator=(LispyList&& other);

  const Value& head() const;
  LispyList tail() const;
  bool empty() const;

  LispyList prepend(Value value) const;

  LispyListIterator<Value> begin() const;
  LispyListIterator<Value> end() const;

  bool operator==(const LispyList& other) const;
  bool operator!=(const LispyList& other) const;
};

template <typename Value>
class LispyListIterator {
  LispyListNode<Value> *node;
  explicit LispyListIterator(LispyListNode<Value>*);
  friend class LispyList<Value>;
public:
  LispyListIterator();
  LispyListIterator(const LispyListIterator&) = default;
  LispyListIterator(LispyListIterator&&) = default;

  const Value& operator*() const;
  const Value *operator->() const;
  LispyListIterator& operator++();
  LispyListIterator operator++(int) const;

  bool operator==(LispyListIterator) const;
  bool operator!=(LispyListIterator) const;
};

namespace std {

template <typename Value>
struct iterator_traits<LispyListIterator<Value>> {
  using value_type = const Value;
  using pointer = const Value*;
  using reference = const Value&;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
};

} // namespace std

template <typename Value>
struct LispyListNode {
  const Value value;
  int refcount;
  LispyListNode *const next;
};

// Implementation
// ==============

// class LispyList<Value>
// ----------------------
template <typename Value>
void LispyList<Value>::cleanup() {
  LispyListNode<Value> *current = node;
  while (current && --current->refcount == 0) {
    auto *old = current;
    current = current->next;
    delete old;
  }
}

template <typename Value>
LispyList<Value>::LispyList(LispyListNode<Value> *node)
: node(node) {
}

template <typename Value>
LispyList<Value>::LispyList()
: node(nullptr) {}


template <typename Value>
LispyList<Value>::LispyList(const LispyList<Value>& other)
: node(other.node) {
  if (node) {
    ++node->refcount;
  }
}

template <typename Value>
LispyList<Value>::LispyList(LispyList<Value>&& other)
: node(other.node) {
  other.node = nullptr;
}

template <typename Value>
LispyList<Value>::~LispyList() {
  cleanup();
}

template <typename Value>
LispyList<Value>& LispyList<Value>::operator=(const LispyList<Value>& other) {
  if (&other == this) {
    return *this;
  }
  cleanup();
  node = other.node;
  if (node) {
    ++node->refcount;
  }
  return *this;
}

template <typename Value>
LispyList<Value>& LispyList<Value>::operator=(LispyList<Value>&& other) {
  if (&other == this) {
    return *this;
  }
  cleanup();
  node = other.node;
  other.node = nullptr;
  return *this;
}

template <typename Value>
const Value& LispyList<Value>::head() const {
  assert(node);
  return node->value;
}

template <typename Value>
LispyList<Value> LispyList<Value>::tail() const {
  assert(node);
  if (node->next) {
    ++node->next->refcount;
  }
  return LispyList<Value>(node->next);
}

template <typename Value>
bool LispyList<Value>::empty() const {
  return node == nullptr;
}

template <typename Value>
LispyList<Value> LispyList<Value>::prepend(Value value) const {
  if (node) {
    ++node->refcount;
  }

  return LispyList<Value>(new LispyListNode<Value>{
    .value = std::move(value),
    .refcount = 1,
    .next = node
  });
}

template <typename Value>
LispyListIterator<Value> LispyList<Value>::begin() const {
  return LispyListIterator<Value>(node);
}

template <typename Value>
LispyListIterator<Value> LispyList<Value>::end() const {
  return LispyListIterator<Value>();
}

template <typename Value>
bool LispyList<Value>::operator==(const LispyList<Value>& other) const {
  return node == other.node;
}

template <typename Value>
bool LispyList<Value>::operator!=(const LispyList<Value>& other) const {
  return node != other.node;
}

// class LispyListIterator<Value>
// ------------------------------
template <typename Value>
LispyListIterator<Value>::LispyListIterator(LispyListNode<Value> *node)
: node(node) {}

template <typename Value>
LispyListIterator<Value>::LispyListIterator()
: node(nullptr) {}

template <typename Value>
const Value& LispyListIterator<Value>::operator*() const {
  assert(node);
  return node->value;
}

template <typename Value>
const Value *LispyListIterator<Value>::operator->() const {
  return &**this;
}

template <typename Value>
LispyListIterator<Value>& LispyListIterator<Value>::operator++() {
  if (node) {
    node = node->next;
  }
  return *this;
}

template <typename Value>
LispyListIterator<Value> LispyListIterator<Value>::operator++(int) const {
  auto copy = *this;
  return ++copy;
}

template <typename Value>
bool LispyListIterator<Value>::operator==(LispyListIterator<Value> other) const {
  return node == other.node;
}

template <typename Value>
bool LispyListIterator<Value>::operator!=(LispyListIterator<Value> other) const {
  return node != other.node;
}
