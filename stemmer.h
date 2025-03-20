//stemmer.h
#pragma once
#include "index.h"
#include "utf8proc/utf8proc.h"



//takes: UTF-8 encoded string with whatever, in unicode
//returns: ASCII string
string standardize (const string& word);

//algorithm from Algorithm for Suffix Stripping by M.F. Porter
string stem (string word); 

size_t countM (const string& word);