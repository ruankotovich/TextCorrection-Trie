#include "person.hpp"
#include "trie.hpp"
#include <iostream>
#include <map>

int main()
{
  trie::Trie_t personTrie;

  std::string name = "Duan";
  std::string name2 = "Daniel";
  std::string name3 = "Soto";
  std::string name4 = "Roab";
  std::string name5 = "Bananation";
  std::string name6 = "SotoBanaNAO";
  std::string name7 = "Danyel";
  std::string name8 = "ovo";
  std::string name9 = "uva";

  personTrie.putIndividualWord(name, name);
  personTrie.putIndividualWord(name2, name2);
  personTrie.putIndividualWord(name3, name3);
  personTrie.putIndividualWord(name4, name4);
  personTrie.putIndividualWord(name5, name5);
  personTrie.putIndividualWord(name6, name6);
  personTrie.putIndividualWord(name7, name7);
  personTrie.putIndividualWord(name8, name8);
  personTrie.putIndividualWord(name9, name9);

  personTrie.buildActiveNodeSet(false);

  std::string search;
  std::cout << "\n >> ";
  while (std::getline(std::cin, search)) {
    std::cout << " --- Searching \"" << search << "\".\n";
    auto mSearch = personTrie.searchKeyword(search);
    if (mSearch.first) {
      for (auto element : mSearch.second) {
        std::cout << " - "<<  element << std::endl;
      }
    } else {
      std::cout << " --- Couldn't find an exact match for this word, searching with errors.\n";
      auto a = personTrie.searchSimilarKeyword(search);
      if (a.empty()) {
        std::cout << " --- Couldn't find any result on this query\n";
        std::cout << "\n >> ";
        continue;
      }

      while (!a.empty()) {
        // std::cout << " -- " << a.top().content->name << " - " << a.top().content->age << '\n';
        std::cout << " - " << a.top().content << std::endl;
        a.pop();
      }
    }
    std::cout << "\n >> ";
  }
}
