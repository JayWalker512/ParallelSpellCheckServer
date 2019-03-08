#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "trie.h"

/* Allocates a new Trie_t data structure and returns a pointer to it. */
Trie_t * newTrie(TrieValue_t value, bool endOfString) {
    Trie_t * tree = (Trie_t *) malloc(sizeof (Trie_t));
    tree->value = value;
    tree->endOfString = endOfString;
    tree->numChildren = 0;
    tree->childrenCapacity = INITIAL_TRIE_CHILDREN;
    tree->children = (Trie_t **) malloc(INITIAL_TRIE_CHILDREN * sizeof (Trie_t *));
    for (size_t i = 0; i < INITIAL_TRIE_CHILDREN; i++) {
        tree->children[i] = NULL;
    }
    return tree;
}

/* Deallocates a Trie_t data structure, and recursively deallocates all of
it's children as well. */
void destroyTrie(Trie_t * tree) {
    if (tree == NULL) { return; }
    
    //recursively destroy children
    for (size_t i = 0; i < tree->numChildren; i++) {
        destroyTrie(tree->children[i]);
    }
    free(tree->children);
    free(tree);
}

/* Search a Trie_t for a particular value specified by val. Returns a pointer
to the child if it is found, otherwise NULL. */
static Trie_t * getChildOfTrie(Trie_t * tree, TrieValue_t val) {
    for (size_t i = 0; i < tree->numChildren; i++) {
        if (tree->children[i]->value == val) {
            return tree->children[i];
        }
    }
    return NULL;
}

/* Adds a new Trie_t child to an existing trie if the child did not
exist. Otherwise returns a pointer to the existing child with matching
value. */
static Trie_t * addChildToTrie(Trie_t * tree, TrieValue_t val, bool endOfString) {
    Trie_t * existingChild = NULL;
    existingChild = getChildOfTrie(tree, val);
    if (existingChild != NULL) {
        if (endOfString) {
            existingChild->endOfString = true; //update this flag if the "new" child has it
        }
        return existingChild;
    }

    if (tree->numChildren >= tree->childrenCapacity - 1) {
        Trie_t ** newChildren = (Trie_t **) malloc(sizeof (Trie_t *) * (tree->childrenCapacity * 2));
        memcpy(newChildren, tree->children, sizeof (Trie_t *) * tree->numChildren);
        free(tree->children);
        tree->children = newChildren;

        if (newChildren == NULL) {
            puts("Memory allocation failed!");
            exit(EXIT_FAILURE);
        }
        tree->childrenCapacity *= 2;
    }

    tree->children[tree->numChildren] = newTrie(val, endOfString);
    tree->numChildren++;

    return tree->children[tree->numChildren - 1];
}

/* Should always insert to the tree who's root node is NULL/0/empty
string value. The root node is essentially ignored when looking up
strings in the trie. 

Note: string must be null-terminated or this function will have
undefined behavior. */
bool insertStringToTrie(Trie_t * tree, TrieValue_t * string) {
    Trie_t * currentNode = tree;
    for (size_t i = 0; string[i] != '\0'; i++) {
        currentNode = addChildToTrie(currentNode, string[i], false);
    }
    currentNode->endOfString = true;
    return true;
}

bool stringExistsInTrie(Trie_t * tree, TrieValue_t * string) {
    Trie_t * currentNode = tree;
    for (size_t i = 0; string[i] != '\0'; i++) {
        currentNode = getChildOfTrie(currentNode, string[i]);
        if (currentNode == NULL) {
            return false;
        }
    }
    if (currentNode->endOfString == false) {
        return false;
    }

    return true;
}

/* Creates a new Trie_t from a dictionary. The provided dictionary should be a 
text file full of words delimited by newline characters */
Trie_t * newTrieFromDictionary(char * dictionaryFileName) {
    Trie_t * tree = newTrie(0, false);

    char * string = NULL;
    size_t len = 0;
    ssize_t bytesRead = 0;
    FILE * fp = NULL;
    fp = fopen(dictionaryFileName, "r");
    if (fp == NULL) {
        destroyTrie(tree);
        return NULL;
    }
    do {
        bytesRead = getline(&string, &len, fp);
        if (bytesRead > 0) {

            //need to replace newlines, carriage returns, etc.
            for (unsigned int i = 0; string[i] != '\0'; i++) {
                if (string[i] == '\n' || string[i] == '\r') {
                    string[i] = '\0';
                }
            }
            insertStringToTrie(tree, string);

        }
    } while (bytesRead != -1);
    fclose(fp);

    return tree;
}

/* Test cases for the Trie_t association functions. */
void testTrie() {
    Trie_t * tree = newTrie(0, false);
    assert(stringExistsInTrie(tree, "test") == false);

    insertStringToTrie(tree, "test");
    assert(stringExistsInTrie(tree, "test") == true);
    assert(stringExistsInTrie(tree, "tes") == false);

    assert(getChildOfTrie(tree, 'd') == NULL);

    destroyTrie(tree);

    Trie_t * dictionary = newTrieFromDictionary("words");
    assert(stringExistsInTrie(dictionary, "hello") == true);
    assert(stringExistsInTrie(dictionary, "guise") == true);

    destroyTrie(dictionary);
}
