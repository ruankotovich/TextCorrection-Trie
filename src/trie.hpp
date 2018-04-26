#ifndef _ZYNTHETIC_TRIE_
#define _ZYNTHETIC_TRIE_
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace trie {

  #define IGNORE_WORD_LENGTH 3

  struct TrieResponse_t {
    std::string content;
    int editDistance;
    TrieResponse_t(std::string _content, int _editDistance)
    : content(_content)
    , editDistance(_editDistance)
    {
    }
  };

  struct TrieResponseComparator_t {
    bool operator()(const TrieResponse_t& comparison1, const TrieResponse_t& comparison2)
    {
      return comparison1.editDistance > comparison2.editDistance;
    }
  };

  class TrieNode_t {
    std::map<unsigned int, TrieNode_t*> m_childrenMap; // used to save all the children (access / insertion O(log n))
    unsigned int m_content; // used to store the current character code in it's structure
    bool m_endOfWord;
    std::vector<std::string>* m_nodeContent;

  public:
    TrieNode_t(unsigned int val)
    : m_content(val)
    , m_endOfWord(false)
    {
    }

    void buildContent()
    {
      m_nodeContent = new std::vector<std::string>();
    }

    std::vector<TrieNode_t*> getChildren()
    {
      std::vector<TrieNode_t*> nodes;
      for (auto cur : this->m_childrenMap) {
        nodes.push_back(cur.second);
      }
      return nodes;
    }

    TrieNode_t* insertNReturnChild(unsigned int value)
    {
      TrieNode_t* node = new TrieNode_t(value);
      this->m_childrenMap.insert({ value, node });
      return node;
    }

    TrieNode_t* getChild(unsigned int value)
    {
      auto child = this->m_childrenMap.find(value);
      if (child != this->m_childrenMap.end()) {
        return child->second;
      }
      return nullptr;
    }

    unsigned int getContent()
    {
      return this->m_content;
    }

    bool isEndOfWord()
    {
      return this->m_endOfWord;
    }

    void setEndOfWord(bool eow)
    {
      this->m_endOfWord = eow;
    }

    void addValue(std::string content)
    {
      this->m_nodeContent->push_back(content);
    }

    const std::vector<std::string>* getValues()
    {
      return this->m_nodeContent;
    }
  };

  struct ActiveNode_t {
    TrieNode_t* node;
    mutable int editDistance;
    mutable int positionDistance;

    ActiveNode_t(TrieNode_t* nd, int ed)
    : node(nd)
    , editDistance(ed)
    , positionDistance(0)
    {
    }

    ActiveNode_t(TrieNode_t* nd, int ed, int pos)
    : node(nd)
    , editDistance(ed)
    , positionDistance(pos)
    {
    }

    bool operator<(const ActiveNode_t& anode) const
    {
      return this->node < anode.node;
    }
  };

  struct ActiveNodeComparator_t {
    bool operator()(const ActiveNode_t& n1, const ActiveNode_t& n2) const
    {
      return n1.editDistance > n2.editDistance;
    }
  };

  class Trie_t {
    TrieNode_t* m_lambdaNode; // used to indicate the first node
    std::unordered_map<unsigned int, unsigned int> m_characterMap; // used to map all the characters to it's defined codes
    std::unordered_map<unsigned int, char> m_reverseCharacterMap; // used to map all the defined codes to it's characters (4fun)
    std::set<ActiveNode_t> m_activeNodeSet; // uset to save the main activeNode set
    std::set<std::string> m_stopWords; // set of stopwords
    int m_searchLimitThreshold; // threshold used for delimit the answers amount (default : 5)
    int m_fuzzyLimitThreshold; // threshold used for delimit the edit distance from node (default : 1)
    const std::vector<std::string> m_emptyResponse;
    void push_string_to_wchar(wchar_t* w, std::string& a)
    {
      setlocale(LC_ALL, "");
      const char* nw = a.c_str();
      mbsrtowcs(w, &nw, a.length(), NULL);
      w[a.length()] = 0;
    }

    std::set<ActiveNode_t> buildNewSet(std::set<ActiveNode_t>& set, unsigned int curChar)
    {
      std::set<ActiveNode_t> activeNodeSet;

      /*
      ** Considering node N
      ** We can transform the content of this node for the currentWord by first adding a character. (Prebuilt Addiction)
      */
      // std::cout << "\nBuilding node for " << m_reverseCharacterMap[curChar] << '\n';
      for (auto curActiveNode = set.begin(); curActiveNode != set.end(); curActiveNode++) {
        if (curActiveNode->editDistance < m_fuzzyLimitThreshold) {
          // std::cout << " * Updating distance for node " << m_reverseCharacterMap[curActiveNode->node->getContent()] << " from " << curActiveNode->editDistance << " to " << curActiveNode->editDistance + 1 << '\n';
          activeNodeSet.emplace(curActiveNode->node, curActiveNode->editDistance + 1);
        }
      }

      /*
      ** Considering no+de P (child of N)
      ** -  C1. Different character from `curChar` : (Posbuilt Substitution Addiction)
      **    We can transform this character of P for the current character by substitution, relative to ED(N), then, we have to add P to
      **    the set if and only if ED(N)+1 [previously calculated and updated] < threshold
      **
      ** -  C2. Equals character from `curChar` : (Posbuilt Match Addiction)
      **    If this node character match with `curChar`, it means that we got to add it with ED(N) (indeed, when we got a match, we have ED=0) [1]
      **    Once doing it, we have to add the descendant nodes of P (called here `PChild`) with distance PChild = ED(P) + DeepthFrom(P, PChild) < threshold
      **
      ** Addendum [1] - I must update the distance only if the new distance is lower than the last distance (it happens when a previously father node have been calculated)
      */

      for (auto curActiveNode = set.begin(); curActiveNode != set.end(); curActiveNode++) {

        // std::cout << "\nScanning children of node " << m_reverseCharacterMap[curActiveNode->node->getContent()] << '\n';

        for (auto childOfcurActiveNode : curActiveNode->node->getChildren()) {
          // std::cout << " * Node " << m_reverseCharacterMap[childOfcurActiveNode->getContent()] << " found ";
          if (childOfcurActiveNode->getContent() != curChar) { // case 1
            // std::cout << " and haven't matched with the character,";
            if (curActiveNode->editDistance < m_fuzzyLimitThreshold) { // verify if ED(N)+1 < P
              // std::cout << " but it's edit distance can be increased and it would be added to the set";
              int newEditDistance = curActiveNode->editDistance + 1; // the new edit distance for the child

              // retrieves to add the child, return FALSE if it was not possible, cuz the child was already added before (so, i've to verify which edit distance is lower : the current or the previous?)
              auto childIteratorOnSet = activeNodeSet.emplace(childOfcurActiveNode, newEditDistance);

              // if the child was not added (it was cuz' there was a activeNode marked on this node before) and the last ED is bigger than the new ED
              if (!childIteratorOnSet.second) {
                if (childIteratorOnSet.first->editDistance > newEditDistance) {
                  childIteratorOnSet.first->editDistance = newEditDistance; // guaranteed by addendum 1
                }
              }
            }
            // std::cout << '\n';
          } else { // case 2
            // std::cout << " and it matches with the character, so will be added to the set\n";
            // retrieves to add the child, return FALSE if it was not possible, cuz the child was already added before (so, i've to verify which edit distance is lower : the current or the previous?)
            auto childIteratorOnSet = activeNodeSet.emplace(childOfcurActiveNode, curActiveNode->editDistance);

            int currentChildDistance = childIteratorOnSet.first->editDistance;

            if (!childIteratorOnSet.second) { /* processor thanks! */
              if (childIteratorOnSet.first->editDistance > curActiveNode->editDistance) {
                currentChildDistance = childIteratorOnSet.first->editDistance = curActiveNode->editDistance;
              }
            }

            /*
            **  I've to fetch the children and the entire set of children to the active node if
            **  and only if the dist( father, getChildRecursive(father)) ) < threshold;
            */
            // std::cout << "\t\tIt's children will be verifieds to be added to the set : \n";

            std::queue<TrieNode_t*> toRecover; // a queue to save which node is on the way
            toRecover.push(childIteratorOnSet.first->node); // adding the current matched node to the queue

            // while the distance is lesser than the limit and we got some node to recover...
            while (currentChildDistance < m_fuzzyLimitThreshold && !toRecover.empty()) {
              // recover the current node from the queue
              TrieNode_t*& currentNode = toRecover.front();
              // std::cout << "\t\tCurrent node : " << m_reverseCharacterMap[currentNode->getContent()] << '\n';
              // and update the distance to one more
              ++currentChildDistance;

              // for each child of the current node
              for (auto child : currentNode->getChildren()) {

                // we add this child to the active node set, once we can face it as a addiction operation inside the boundary imposed by the search
                auto currentChildIterator = activeNodeSet.emplace(child, currentChildDistance);
                // std::cout << "\t\t * Children " << m_reverseCharacterMap[currentChildIterator.first->node->getContent()] << " added to the set ";
                // if there was this child within the activeNode, we have to keep the minor operation distance
                if (!currentChildIterator.second) {
                  // std::cout << "by update/permanence ";
                  if (currentChildIterator.first->editDistance > currentChildDistance) {
                    currentChildIterator.first->editDistance = currentChildDistance;
                  }
                }
                // std::cout<<" with edit distance " << currentChildIterator.first->editDistance << '\n';

                // and put it if and only if the currentDistance is lesser than the thresould (the memory and processment thank!)
                if (currentChildDistance < m_fuzzyLimitThreshold) {
                  toRecover.push(child);
                }
              }
              toRecover.pop();
            }
          }
        }
      }
      // std::cout << "\n\n";
      return activeNodeSet;
    }

  public:
    void putIndividualWord(std::string& str, const std::string& content)
    {
      TrieNode_t*currentRoot, *lastRoot;
      currentRoot = this->m_lambdaNode;

      wchar_t chart[str.size()];
      push_string_to_wchar(chart, str);

      for (unsigned int i = 0; i < wcslen(chart); i++) {

        unsigned int code = this->m_characterMap[chart[i]];

        lastRoot = currentRoot;
        currentRoot = currentRoot->getChild(code);
        if (!currentRoot) {
          currentRoot = lastRoot->insertNReturnChild(code);
        }
      }

      if (currentRoot != this->m_lambdaNode) {

        if (!currentRoot->isEndOfWord()) {
          currentRoot->buildContent();
          currentRoot->setEndOfWord(true);
        }

        currentRoot->addValue(content);
      }
    }

    void buildActiveNodeSet(bool _onlyFinalWords)
    {
      std::queue<std::pair<TrieNode_t*, int>> seekQueue;
      std::unordered_map<TrieNode_t*, TrieNode_t*> father;
      std::set<TrieNode_t*> visited;
      seekQueue.emplace(this->m_lambdaNode, 0); // add the lambdaNode to the seek
      m_activeNodeSet.emplace(this->m_lambdaNode, 0); // add the lambdaNode to the activeSet
      father.emplace(this->m_lambdaNode, nullptr); // set lambdaNode father as nullptr

      /* initialization */

      while (!seekQueue.empty()) {
        auto currentNode = seekQueue.front(); // get currentNode
        seekQueue.pop();

        for (TrieNode_t* child : currentNode.first->getChildren()) {

          // verify if the child hasn't been visited recently
          if (visited.find(child) == visited.end()) {
            int currentLevel = currentNode.second + 1;

            visited.emplace(child); // add child to the visited set
            father.emplace(child, currentNode.first); // set current vertex as father

            if (currentLevel < m_fuzzyLimitThreshold) { // add the child on the seek only if it has a threshold < fuzzy
              seekQueue.emplace(child, currentLevel);
            }

            if (!_onlyFinalWords || child->isEndOfWord()) { // verify if this child is end of word
              TrieNode_t* currentChild = child;
              m_activeNodeSet.emplace(currentChild, currentLevel); // add the child to the activeSet

              auto currentChild_it = father.find(child);

              // add all the father's father's father's ... father's of this child to the activeSet
              while (currentChild_it->second != nullptr) {

                if (m_activeNodeSet.emplace(currentChild_it->second, --currentLevel).second) {
                  currentChild_it = father.find(currentChild_it->second);
                } else {
                  break;
                }
              }
            }
          }
        }
      }
    }

    void encodeCharacters(const std::string& filename)
    {
      // std::cout << "[LOG] Encoding characters." << std::endl;
      unsigned int lineCode = 1;
      std::string currentLine;
      std::ifstream fileInputStream(filename);

      while (std::getline(fileInputStream, currentLine)) {

        m_reverseCharacterMap.emplace(lineCode, currentLine.back());

        wchar_t anomalousCharacters[currentLine.size()];
        push_string_to_wchar(anomalousCharacters, currentLine);

        for (unsigned int i = 0; i < wcslen(anomalousCharacters); i++) {
          m_characterMap.emplace(anomalousCharacters[i], lineCode);
        }

        lineCode++;
      }

      m_reverseCharacterMap[0] = '#'; // lambda character
      std::cout << "Charmap has been mapped.\n";
    }

    void addStopWords(const std::string& filename)
    {
      std::string currentLine;
      std::ifstream fileInputStream(filename);

      while (std::getline(fileInputStream, currentLine)) {
        std::transform(currentLine.begin(), currentLine.end(), currentLine.begin(), ::tolower);
        m_stopWords.insert(currentLine);
      }
    }

    bool isStopWord(std::string str)
    {
      std::transform(str.begin(), str.end(), str.begin(), ::tolower);
      return m_stopWords.find(str) != m_stopWords.end();
    }

    // Trie_t public methods

    Trie_t()
    : m_searchLimitThreshold(5)
    , m_fuzzyLimitThreshold(1)
    {
      this->m_lambdaNode = new TrieNode_t(0);

      DIR* dirp;
      struct dirent* directory;
      dirp = opendir("./stopwords");
      if (dirp) {
        while ((directory = readdir(dirp)) != NULL) {
          if (directory->d_type == DT_REG) {
            std::cout << "The stopword file \'" << directory->d_name << "\' has been detected.\n";
            this->addStopWords(std::string("./stopwords/") + (directory->d_name));
          }
        }
        closedir(dirp);
      }

      encodeCharacters("charmap.cm");
    }

    void printTrie()
    {
      TrieNode_t* currentNode;
      std::set<TrieNode_t*> visited;

      std::stack<TrieNode_t*> nodeStack;
      nodeStack.push(this->m_lambdaNode);

      while (!nodeStack.empty()) {
        currentNode = nodeStack.top();

        if (visited.find(currentNode) == visited.end()) {
          visited.insert(currentNode);
          std::cout << "[" << this->m_reverseCharacterMap[currentNode->getContent()] << (currentNode->isEndOfWord() ? "'" : " ");
          std::vector<TrieNode_t*> children = currentNode->getChildren();

          while (!children.empty()) {
            nodeStack.push(children.back());
            children.pop_back();
          }

        } else {
          nodeStack.pop();
          std::cout << "]";
        }
      }
    }

    std::pair<bool, std::vector<std::string>> searchKeyword(std::string& keyword)
    {
      std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
      wchar_t chart[keyword.size()];
      push_string_to_wchar(chart, keyword);

      TrieNode_t* currentNode = this->m_lambdaNode;

      for (unsigned int i = 0; i < wcslen(chart); i++) {
        currentNode = currentNode->getChild(m_characterMap[chart[i]]);

        if (currentNode == nullptr) {
          break;
        }
      }

      if (currentNode != nullptr) {
        if (currentNode->isEndOfWord()) {
          return { true, *currentNode->getValues() };
        }
      }

      return { false, this->m_emptyResponse };
    }

    std::priority_queue<TrieResponse_t, std::vector<TrieResponse_t>, TrieResponseComparator_t> searchSimilarKeyword(std::string& keyword)
    {
      std::priority_queue<TrieResponse_t, std::vector<TrieResponse_t>, TrieResponseComparator_t> ocurrencesQueue;

      std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);

      std::set<ActiveNode_t> lastActiveNodes = this->m_activeNodeSet;

      wchar_t chart[keyword.size()];
      push_string_to_wchar(chart, keyword);

      for (unsigned int i = 0; i < wcslen(chart); i++) {
        lastActiveNodes = buildNewSet(lastActiveNodes, m_characterMap[chart[i]]);
      }

      for (auto node : lastActiveNodes) {

        if (node.node->isEndOfWord()) {
          for (auto& _target : *node.node->getValues()) {
            ocurrencesQueue.emplace(_target, node.editDistance);
          }
        }
      }

      return ocurrencesQueue;
    }

    std::priority_queue<TrieResponse_t, std::vector<TrieResponse_t>, TrieResponseComparator_t> autocomplete(std::string& keyword)
    {
      std::priority_queue<TrieResponse_t, std::vector<TrieResponse_t>, TrieResponseComparator_t> ocurrencesQueue;
      std::priority_queue<ActiveNode_t, std::vector<ActiveNode_t>, ActiveNodeComparator_t> activeList;

      std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);

      std::set<ActiveNode_t> lastActiveNodes = this->m_activeNodeSet;

      wchar_t chart[keyword.size()];
      push_string_to_wchar(chart, keyword);

      for (unsigned int i = 0; i < wcslen(chart); i++) {
        lastActiveNodes = buildNewSet(lastActiveNodes, m_characterMap[chart[i]]);
      }

      for (auto node : lastActiveNodes) {
        activeList.push(node);
      }

      std::unordered_set<TrieNode_t*> visitedNode;

      while (!activeList.empty()) {

        auto aNode = activeList.top();
        activeList.pop();

        if (visitedNode.find(aNode.node) == visitedNode.end()) {
          std::queue<TrieNode_t*> pQueue;
          pQueue.push(aNode.node);

          while (!pQueue.empty()) {
            auto currentSeeker = pQueue.front();
            visitedNode.insert(currentSeeker);
            pQueue.pop();

            if (currentSeeker->isEndOfWord()) {
              for (auto oValue : *currentSeeker->getValues()) {
                ocurrencesQueue.emplace(oValue, aNode.editDistance);
              }
            }

            for (TrieNode_t* curChild : currentSeeker->getChildren()) {
              if (visitedNode.find(curChild) == visitedNode.end()) {
                pQueue.emplace(curChild);
              }
            }
          }
        }
      }

      return ocurrencesQueue;
    }

    void setSearchLimitThreshold(int limit)
    {
      this->m_searchLimitThreshold = limit;
    }

    void setFuzzyLimitThreshold(int limit)
    {
      this->m_fuzzyLimitThreshold = limit;
    }
  };
}
#endif
