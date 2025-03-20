//stemmer.cpp
//stems words
#include "stemmer.h"

static inline bool isNum(const char c) 
    {
    return (c >= '0' && c <= '9');
    }
static inline bool isLowerAlpha(const char c)
    {
    return (c >= 'a' && c <= 'z');
    }

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
        return word;
        }
    string newWord(result_len);
    for (int i = 0; i < result_len; i++)
        if ( isNum( result[i] ) || isLowerAlpha( result[i] ) )
            newWord.pushBack(result[i]);
    free(result);
    return newWord;
    }

static inline void removeS (string& word)
    {
    if ( word.size() < 3 )
        return;
    if ( word.substr(-1) != (string)"s" )
        return;
    if (word.substr(-4) == (string)"sses")
        word.popBack(2);
    else if ( word.substr(-3) == (string)"ies" )
        word.popBack(2);
    else if ( word.substr(-2) != (string)"ss" ) //word end guaranteed to be s from above
        word.popBack(1);
    }

//
static inline bool isVowel(const string& word, int i)
    {
    if (i < 0)
        return false;
    if ( word[i] == 'a' || word[i] == 'o' 
    || word[i] == 'e' || word[i] == 'i' || word[i] == 'u')
        return true;
    if (word[i] == 'y')
        if ( isVowel( word, i - 1 ) )
            return false;
        else
            return true;
    return false;
    }

size_t countM (const string& word)
    {
    int size = word.size();
    int m = 0;
    bool v;
    int i = 0;
    //find first vowel
    do
        v = isVowel(word, i++);
    while (!v && i < size);
    if (i == size)
        return 0;
    for (i; i < size; i++)
        {
        bool t = isVowel(word, i);
        //new vowel sequence, t is vowel, v isn't
        if (t != v && !v)
            m++;
        v = t;       
        }
    if ( !v )
        m++;
    return m;
    }


//algorithm from Algorithm for Suffix Stripping by M.F. Porter
string stem (string word) 
    {
    word = standardize(word);
    removeS(word);
    // Additional stemming can be implemented here
    return word;
    }