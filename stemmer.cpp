//stemmer.cpp
//stems words
#include "stemmer.h"

string standardize (const string& word)
    {
    utf8proc_uint8_t *result = NULL;
    utf8proc_option_t options = (utf8proc_option_t)(
        UTF8PROC_STABLE | UTF8PROC_DECOMPOSE | UTF8PROC_COMPAT | UTF8PROC_CASEFOLD | UTF8PROC_STRIPMARK
    );
    int64_t result_len = utf8proc_map(
        (const utf8proc_uint8_t*)word.c_str(),  
        word.size(),                                     
        &result,                               
        options                                
    );
    if (result_len < 0)
        {
        std::cerr << "utf8proc has encountered an error. Code: " << result_len << std::endl;
        free(result);
        return string();
        }
    string newWord(result_len);
    for (int i = 0; i < result_len; i++)
        if ( isNum( result[i] ) || isLowerAlpha( result[i] ) )
            newWord.pushBack(result[i]);
    newWord.pushBack('\0');
    free(result);
    return newWord;
    }

void removeS (string word)
    {
    if( word.substr(-1) != (string)"s" )
        return;
    if ( word.substr(-4) == (string)"sses" )
        word.popBack(2);
    if ( word.substr(-3) == (string)"ies" )
        word.popBack(2);
    if ( word.substr(-2) != (string)"ss" ) //word end guaranteed to be s from above
        word.popBack(1);
    }

//algorithm from Algorithm for Suffix Stripping by M.F. Porter
string stem (string word) 
    {
    word = standardize(word);
    removeS(word);
    // Additional stemming can be implemented here
    return word;
    }