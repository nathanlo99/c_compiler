
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using NFAEntry = std::map<char, std::set<int>>;

struct NFA {
  // Invariant: The first entry is the start state, the last is the only
  // accepting state
  std::vector<NFAEntry> entries;

  explicit NFA(const std::vector<NFAEntry> &entries) : entries(entries) {}
  explicit NFA(const char c) : entries(2) { entries[0][c] = {1}; }

  explicit NFA(const std::set<char> &chars) : entries(2) {
    for (char c : chars)
      entries[0][c] = {1};
  }

  explicit NFA(const std::string &s) : entries(s.size() + 1) {
    for (int i = 0; i < s.size(); ++i)
      entries[i][s[i]] = {1};
  }

  void print() const {
    for (int i = 0; i < entries.size(); ++i) {
      std::cout << "State " << i << ": " << std::endl;
      for (const auto &[c, offsets] : entries[i]) {
        const auto display_c = (c == 0) ? " eps" : std::string("   ") + c;
        std::cout << display_c << " -> { ";
        for (auto offset : offsets) {
          std::cout << (i + offset) << ", ";
        }
        std::cout << "\b\b }" << std::endl;
      }
    }
  }

  static NFA _union(const NFA &nfa1, const NFA &nfa2) {
    const int size1 = nfa1.entries.size(), size2 = nfa2.entries.size();
    auto entries = std::vector<NFAEntry>(size1 + size2 + 2);
    const int last = size1 + size2 + 1;
    // 0                           : start
    // 1 to size1                  : nfa1
    // size1 + 1 to size1 + size2  : nfa2
    // size1 + size2 + 1           : end
    entries[0][0] = {1, size1 + 1};
    for (int i = 0; i < nfa1.entries.size(); ++i) {
      entries[i + 1] = nfa1.entries[i];
    }
    entries[size1][0].insert(last - size1);
    for (int i = 0; i < nfa2.entries.size(); ++i) {
      entries[i + size1 + 1] = nfa2.entries[i];
    }
    entries[size1 + size2][0].insert(last - (size1 + size2));

    return NFA(entries);
  }
};

int main() {
  const auto nfa1 = NFA("Hello");
  const auto nfa2 = NFA("Bye");
  const auto combined = NFA::_union(nfa1, nfa2);
  combined.print();
}
