// Map a HashBlob into memory and then search it for words
// read from cin.

// Nicole Hamilton  nham@umich.edu

#include <cassert>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "HashBlob.h"

using namespace std;

extern bool optVerbose;

static void Usage()
{
    cout <<
        "Usage:  HashFile [ -v ] blobfile\n"
        "\n"
        "Map the blobfile into memory and then search it for words\n"
        "read from cin.\n"
        "-v means verbose output\n";

    exit(0);
}

using Hash = HashTable<const char *, size_t>;
using Pair = Tuple<const char *, size_t>;

static void Search(const HashBlob *hashblob)
{
    // Search the HashBlob for words read from cin.
    string word;
    while (cin >> word)
    {
        const SerialTuple *s = hashblob->Find(word.c_str());
        cout << (s ? s->Value : 0) << "   " << word << endl;
    }
}