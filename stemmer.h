//stemmer.h
#pragma once
#include "index.h"
#include "utf8proc/utf8proc.h"

static inline bool isNum(const char c) 
    {
    return (c >= '0' && c <= '9');
    }
static inline bool isLowerAlpha(const char c)
    {
    return (c >= 'a' && c <= 'z');
    }

//takes: UTF-8 encoded string with whatever, in unicode
//returns: ASCII string
string standardize (const string& word);

void removeS (string word);

//algorithm from Algorithm for Suffix Stripping by M.F. Porter
string stem (string word); 