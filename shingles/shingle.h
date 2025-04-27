#pragma once

#include "../index/index.h"
#include <cf/searchstring.h>
#include <cf/vec.h>

#ifdef __AVX2__
#include <immintrin.h>  // For AVX2 intrinsics
#endif

#include <unordered_map>
#include <string>

const int SHINGLE_SIZE = 8;
const int SIGNATURE_SIZE = 64;
const int NUM_BANDS = 4;
constexpr int BAND_SIZE = SIGNATURE_SIZE / NUM_BANDS;
const size_t fnvMod = ~size_t(0);
constexpr float SIMILARITY_THRESHOLD = 0.8;
constexpr int NUM_SIMILARITY = SIMILARITY_THRESHOLD * SIGNATURE_SIZE;

// FNV hash constants
const size_t fnvPrime = 16777619;
const size_t fnvOffset = 2166136261;

//just gettin my shit down to a char
const size_t p = 255;

//constants for the signature hash function
const size_t a[64] = {
    0x12345678, 0x23456789, 0x3456789A, 0x456789AB,
    0x56789ABC, 0x6789ABCD, 0x789ABCDE, 0x89ABCDEF,
    0x9ABCDEF0, 0xABCDEF01, 0xBCDEF012, 0xCDEF0123,
    0xDEF01234, 0xEF012345, 0xF0123456, 0x01234567,
    0x12345678, 0x23456789, 0x3456789A, 0x456789AB,
    0x56789ABC, 0x6789ABCD, 0x789ABCDE, 0x89ABCDEF,
    0x9ABCDEF0, 0xABCDEF01, 0xBCDEF012, 0xCDEF0123,
    0xDEF01234, 0xEF012345, 0xF0123456, 0x01234567,
    0x12345678, 0x23456789, 0x3456789A, 0x456789AB,
    0x56789ABC, 0x6789ABCD, 0x789ABCDE, 0x89ABCDEF,
    0x9ABCDEF0, 0xABCDEF01, 0xBCDEF012, 0xCDEF0123,
    0xDEF01234, 0xEF012345, 0xF0123456, 0x01234567,
    0x12345678, 0x23456789, 0x3456789A, 0x456789AB,
    0x56789ABC, 0x6789ABCD, 0x789ABCDE, 0x89ABCDEF,
    0x9ABCDEF0, 0xABCDEF01, 0xBCDEF012, 0xCDEF0123,
    0xDEF01234, 0xEF012345, 0xF0123456, 0x01234567
};

//constants for the signature hash function
const size_t b[64] = {
    0x87654321, 0x98765432, 0xA9876543, 0xBA987654,
    0xCBA98765, 0xDCBA9876, 0xEDCBA987, 0xFEDCBA98,
    0x0FEDCBA9, 0x10FEDCBA, 0x210FEDCB, 0x3210FEDC,
    0x43210FED, 0x543210FE, 0x6543210F, 0x76543210,
    0x87654321, 0x98765432, 0xA9876543, 0xBA987654,
    0xCBA98765, 0xDCBA9876, 0xEDCBA987, 0xFEDCBA98,
    0x0FEDCBA9, 0x10FEDCBA, 0x210FEDCB, 0x3210FEDC,
    0x43210FED, 0x543210FE, 0x6543210F, 0x76543210,
    0x87654321, 0x98765432, 0xA9876543, 0xBA987654,
    0xCBA98765, 0xDCBA9876, 0xEDCBA987, 0xFEDCBA98,
    0x0FEDCBA9, 0x10FEDCBA, 0x210FEDCB, 0x3210FEDC,
    0x43210FED, 0x543210FE, 0x6543210F, 0x76543210,
    0x87654321, 0x98765432, 0xA9876543, 0xBA987654,
    0xCBA98765, 0xDCBA9876, 0xEDCBA987, 0xFEDCBA98,
    0x0FEDCBA9, 0x10FEDCBA, 0x210FEDCB, 0x3210FEDC,
    0x43210FED, 0x543210FE, 0x6543210F, 0x76543210
};

class ShingleMap {
    public:
    /*
    *add a document to the shingle map
    *@param document: the document as a vector of strings
    *modifies: bandTables, shingles_signatures
    *effects: adds the document to the shingle map, stores the signature in shingles_signatures
    */
    void addDocument(const vector<string> &document);
    //directly add a signature to the shingle map
    void addDocument(char* signature);
    /*
    *check if a document is similar to any of the documents in the shingle map
    *@param document: the document as a vector of strings
    *@returns: true if the document's signature meets the SIMILARITY_THRESHOLD
    *with any candidate signatures found through LSH, false otherwise
    */
    bool isSimilar (const vector<string> &document);
    //directly check if a signature is similar to any of the signatures in the shingle map
    bool isSimilar(char* signature);
    ~ShingleMap() {
        for (char* signature : shingles_signatures)
            if (signature != nullptr)
            delete[] signature;
    }

    private:
    std::unordered_map<size_t, vector<size_t>> bandTables[NUM_BANDS];
    vector<char*> shingles_signatures;
    size_t shingleHash(char (&band)[BAND_SIZE]);
    size_t shingleHash(const vector<string> &words, size_t start, size_t shingleSize = SHINGLE_SIZE);
    void sign(const vector<size_t> &shingles, char* signature);
    vector<size_t> createShingles(const vector<string> &words);
};
