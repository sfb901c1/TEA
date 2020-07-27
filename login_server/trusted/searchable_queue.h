/**
 * This handy class is taken from https://stackoverflow.com/a/16749994
 */

#ifndef SEARCHABLE_QUEUE_H_
#define SEARCHABLE_QUEUE_H_

template<
    class T,
    class Container = std::vector<T>,
    class Compare = std::less<typename Container::value_type>
>
class SearchableQueue : public std::priority_queue<T, Container, Compare> {
 public:
  typedef typename
  std::priority_queue<
      T,
      Container,
      Compare>::container_type::const_iterator const_iterator;

  const_iterator find(const T &val) const {
    auto first = this->c.cbegin();
    auto last = this->c.cend();
    while (first != last) {
      if (*first == val) return first;
      ++first;
    }
    return last;
  }

  const_iterator last() const {
    return this->c.cend();
  }
};

#endif //SEARCHABLE_QUEUE_H
