//stemmer.h
#pragma once
#include "../../utils/string.h"
#include "../../utils/vector.h"
#include "utf8proc/utf8proc.h"


//takes: UTF-8 encoded string with whatever, in unicode
//returns: ASCII string
string standardize (const string& word);

//algorithm from Algorithm for Suffix Stripping by M.F. Porter
//MODIFIES: word
void stem (string s); 

//takes: vector of strings, stems each string
//MODIFIES: each string in the vector
void stem ( vector<string> v );