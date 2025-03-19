// Simple HashBlob application

// Nicole Hamilton  nham@umich.edu

#include <cassert>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstring>

#include "HashBlob.h"
#include "Common.h"

using namespace std;

static void Usage()
{
    cout <<
        "Usage:  HashBlob [ -L ] wordsin.txt [ blobfile ]\n"
        "\n"
        "Reads words from wordsin.txt, creates a hashblob,\n"
        "then searches it for words read from cin.\n"
        "\n"
        "If blobfile is specified, it writes the hashblob\n"
        "into the blobfile.\n"
        "\n"
        "-v means verbose output\n"
        "-L means read whole lines as words\n";

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