/* This library implements a Trie data structure for storing a dictionary 
as described here: https://en.wikipedia.org/wiki/Trie 
See implementation file trie.c for function documentation. */


#ifndef TRIE_H
#define TRIE_H

#include <stdbool.h>

#define INITIAL_TRIE_CHILDREN 8

typedef char TrieValue_t;

typedef struct Trie_s {
    TrieValue_t value;
    bool endOfString;
    size_t numChildren;
    size_t childrenCapacity;
    struct Trie_s ** children; //these will point to Trie_t's
} Trie_t;

Trie_t * newTrie(TrieValue_t value, bool endOfString);
void destroyTrie(Trie_t * tree);

bool insertStringToTrie(Trie_t * tree, TrieValue_t * string);
bool stringExistsInTrie(Trie_t * tree, TrieValue_t * string);
Trie_t * newTrieFromDictionary(char * dictionaryFileName);

void testTrie();

#endif
