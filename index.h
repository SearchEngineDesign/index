//index.h

#include <cstddef>
#include <bitset>
#include "HtmlTags.h"
#include "include/string.h"
#include "include/vector.h"
#include "hash/HashBlob.h"
#include "hash/HashTable.h"

enum class Token {
    EoD,            //end-of-document token
    Anchor,         //token in anchor text
    URL,            //url token
    Title,          //title text token
    Body            //body text token
};

enum class Style {
    Normal,     //represented by 00
    Heading,    //represented by 11
    Italic,     //represented by 01
    Bold        //represented by 10
};

struct Post {
    //Variable byte size char array, to be encoded in utf-8.
    //Structure: n bits to encode the offset + 2 bits to encode style (for word tokens)
    //n bits to encode the offset + log(count) bits to number of times the anchor text 
        //the word belongs to has occurred with the URL it links to (for anchor text)
    //n bits to encode the EOF + n bits to encode an index to the corresponding URL for EOF tokens
    char *data;
};

class PostingList {
public:
    void update(size_t delta);
    void update(size_t delta, uint8_t style);
private:

    //Common header
    size_t useCount;        //number of times token occurs
    size_t documentCount;   //number of documents containing token
    Token type;             //variety of token
    size_t listSize;        //size of list (in bytes)

    //Type-specific data

    //Index
    string index;

    //Posts
    vector<Post> list;

    //Sentinel
    char sentinel = '\0';
};

class Index {
public:
    //Constructor should take in parsed HTML and add it to the index.
    void addDocument(const char *buffer, size_t length);
private:
    size_t count = 0;
    string completeLink(string link, string base_url);
    HashBlob dictBlob;
    HashTable<string, PostingList> dict;

    string titleMarker = string("@");
    string anchorMarker = string("$");
    string urlMarker = string("#");
    string eodMarker = string("%");
};