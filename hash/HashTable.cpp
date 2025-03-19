// Sample HashTable application

// Nicole Hamilton  nham@umich.edu

#include <cassert>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <vector>

#include "HashTable.h"
#include "Common.h"

using namespace std;

static void Usage()
{
    cout <<
        "Usage:  HashTable [ -vL ] wordsin.txt\n"
        "\n"
        "Reads words from wordsin.txt, creates a HashTable\n"
        "in memory, optimizes it, then searches it for words\n"
        "read from cin.\n"
        "\n"
        "-v means verbose output\n"
        "-L means read whole lines as words\n";

    exit(0);
}

using Hash = HashTable<const char *, size_t>;
using Pair = Tuple<const char *, size_t>;

static void Search(const Hash *hashtable)
{
    // Search the HashTable for words read from cin.
    string word;
    while (cin >> word)
    {
        const Pair *p = hashtable->Find(word.c_str());
        cout << (p ? p->value : 0) << "   " << word << endl;
    }
}