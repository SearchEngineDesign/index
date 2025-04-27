#include "shingle.h"
#include <cstring>

//FNV hash for a vector of strings
//shingles SHINGLE_SIZE words starting at index start
size_t ShingleMap::shingleHash(const vector<string> &words, size_t start, size_t shingleSize)
    {
    unsigned long hash = fnvOffset;
    for (size_t i = start; i < start + shingleSize; i++) 
        {
        string str = words[i];
        for (size_t j = 0; j < str.size(); j++) 
            {
            hash *= fnvPrime;
            hash ^= str[j];
            }
        }
    return hash % fnvMod;
    }

//FNV hash for a band (char array)
size_t ShingleMap::shingleHash(char (&band)[BAND_SIZE])
    {
    unsigned long hash = fnvOffset;
    for (int i = 0; i < BAND_SIZE; i++)
        {
        hash *= fnvPrime;
        hash ^= band[i];
        }
    return hash % fnvMod;
    }

vector<size_t> ShingleMap::createShingles(const vector<string> &words) 
    {
    vector<size_t> shingles;
    //if the number of words is less than the shingle size, 
    //return one shingle of all the words
    if (words.size() < SHINGLE_SIZE)
        {
        size_t hash = shingleHash(words, 0, words.size());
        shingles.push_back(hash);
        return shingles;
        }
    //reserve space for the shingles
    shingles.reserve(words.size() - SHINGLE_SIZE + 1);
    for (size_t i = 0; i <= words.size() - SHINGLE_SIZE; i++) 
        {
        size_t hash = shingleHash(words, i);
        shingles.push_back(hash);
        }
    return shingles;
    }

//create a signature of the shingles
//64 hash functions in the form of ax + b mod p
//uses SIMD instructions if available (should be on GCP)
void ShingleMap::sign(const vector<size_t> &shingles, char* signature)
    {
    //initialize signatures
    for (int i = 0; i < SIGNATURE_SIZE; i++)
        signature[i] = ~char(0);
    // Process shingles in batches of 4 for AVX2
    #ifdef __AVX2__
    for (size_t i = 0; i < shingles.size(); i += 4) {
        __m256i shingle_vec = _mm256_set_epi64x(
            i + 3 < shingles.size() ? shingles[i + 3] : 0,
            i + 2 < shingles.size() ? shingles[i + 2] : 0,
            i + 1 < shingles.size() ? shingles[i + 1] : 0,
            shingles[i]
        );
        for (int j = 0; j < SIGNATURE_SIZE; j++) {
            __m256i aV = _mm256_set1_epi64x(a[j]);
            __m256i bV = _mm256_set1_epi64x(b[j]);
            __m256i pV = _mm256_set1_epi64x(p);
            __m256i hashVec = _mm256_add_epi64(
                _mm256_mul_epi64(shingle_vec, aV),
                bV
            );
            __m256i modVec = _mm256_rem_epi64(hashVec, pV);

            // Extract results and update min values
            char hashValues[4];
            _mm256_storeu_si256((__m256i*)hashValues, modVec);

            for (int k = 0; k < 4 && i + k < shingles.size(); k++) {
                if (hashValues[k] < signature[j]) {
                    signature[j] = hashValues[k];
                }
            }
        }

    }
    #else
    // if AVX2 is not supported, use a fallback implementation
    for (int i = 0; i < shingles.size(); i++) 
        for (int j = 0; j < 64; j++) 
            {
            size_t hash = (a[j] * shingles[i] + b[j]) % p;
            if (hash < signature[j]) 
                signature[j] = hash;
            }
    #endif
    }

void ShingleMap::addDocument(char* signature)
    {
    // Add the signature to our collection
    char* newSignature = new char[SIGNATURE_SIZE];
    memcpy(newSignature, signature, SIGNATURE_SIZE);
    shingles_signatures.push_back(newSignature);

    // Add to band tables
    for (int band = 0; band < NUM_BANDS; band++) 
        {
        size_t bandHash = shingleHash(*(char(*)[BAND_SIZE])(signature + band * BAND_SIZE));
        bandTables[band][bandHash].push_back(shingles_signatures.size() - 1);
        }
    }

void ShingleMap::addDocument(const vector<string> &document)
    {
    vector<size_t> shingles = createShingles(document);
    char* signature = new char[SIGNATURE_SIZE];
    sign(shingles, signature);
    addDocument(signature);
    }

bool ShingleMap::isSimilar(char* signature)
    {
    // Check each band
    for (int band = 0; band < NUM_BANDS; band++) {
        size_t bandHash = shingleHash(*(char(*)[BAND_SIZE])(signature + band * BAND_SIZE));
        auto it = bandTables[band].find(bandHash);
        if (it != bandTables[band].end()) {
            // Check similarity with each candidate
            for (size_t candidateIdx : it->second) {
                char* candidate = shingles_signatures[candidateIdx];
                int similarBits = 0;
                for (int i = 0; i < SIGNATURE_SIZE; i++) {
                    if (signature[i] == candidate[i]) {
                        similarBits++;
                    }
                }
                if (similarBits >= NUM_SIMILARITY) {
                    return true;
                }
            }
        }
    }
    return false;
    }

bool ShingleMap::isSimilar(const vector<string> &document)
    {
    vector<size_t> shingles = createShingles(document);
    char signature[SIGNATURE_SIZE];
    sign(shingles, signature);
    return isSimilar(signature);
    }
    
