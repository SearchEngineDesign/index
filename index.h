//index.h
#pragma once
#include <cstddef>
#include <bitset>
#include "include/custom_string.h"
#include "include/custom_vector.h"

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
    //n bits to encode the offset + log(count) bits to encode anchor text???? consult slides
    //n bits to encode the EOF + n bits to encode an index to the corresponding URL for EOF tokens
    char *data;
};

class PostingList {
public:

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
    Index();
private:

};