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

static inline bool isVowel(const string& word, size_t i)
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

static inline bool doubleConsonant(const string& word)
    {
    if (word.size() < 2)
        return false;
    int end = word.size() - 1;
    if( word[end] == word[end - 1] && isVowel( word, end - 1 ) && isVowel( word, end ) )
        return true;
    else
        return false;
    }

static inline bool oCheck(const string& word)
    {
    if (word.size() < 3)
        return false;
    if ( *word.end() == 'x' || *word.end() == 'y' || *word.end() == 'w' )
        return false;
    int end = word.size() - 1;
    if( !isVowel(word, end - 2) && isVowel(word, end - 1) && !isVowel(word, end) )
        return true;
    else
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
        //start of new sequence VC
        if (t != v && !v)
            m++;
        v = t;       
        }
    //if sequence ends on VC, last isn't counted
    if ( !v )
        m++;
    return m;
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

static inline void step1a (string& word)
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

static inline void cont1b (string& word, size_t m)
    {
    if ( word.substr(-2) == (string)"at" )
        word.pushBack('e');
    else if ( word.substr(-2) == (string)"bl" )
        word.pushBack('e');
    else if ( word.substr(-2) == (string)"iz" )
        word.pushBack('e');
    else if ( doubleConsonant(word), 
        ( *word.end() != 'l' && *word.end() != 's' && *word.end() != 'z' ) )
        word.popBack();
    else if ( m == 1 && oCheck(word) )
        word.pushBack('e');
    }

static inline void step1b (string& word, size_t m)
    {
    if (m > 0 && word.substr(-3) == (string)"eed")
        {
        word.popBack();
        return;
        }
    bool containsVowel = false;
    if (m > 0)
    //guaranteed to have a vowel, at least on VC segment
        containsVowel = true;
    else 
    //simple way to count if there's a vowel in the word
        for (int i = 0; i < word.size(); i++)
            if( isVowel( word, i ) )
                containsVowel = true;
    if (!containsVowel)
        return;
    if ( word.substr(-2) == (string)"ed" )
        {
        word.popBack(2);
        cont1b(word, m);
        }
    else if ( word.substr(-3) == (string)"ing" )
        {
        word.popBack(3);
        cont1b(word, m);
        }
    }

static inline void step1c (string& word)
    {
    bool hasVowel = false;
    for (int i = 0; i < word.size() - 1; i++)
        if ( isVowel( word, i ) )
            hasVowel = true;
    if (*word.end() == 'y' && hasVowel)
        word[word.size() - 1] = 'i';
    }

static inline void step2 (string& word)
    {
    //if else if statement hell
    //could map it but uh I'm lazy
    int size = word.size();
    if (size > 7)
        {
        const string temp = word.substr(-7);
        if (temp == (string)"ational")
            {
            word.popBack(5);
            word.pushBack('e');
            return;
            }
        else if (temp == (string)"ization")
            {
            word.popBack(5);
            word.pushBack('e');
            return;
            }
        else if (temp == (string)"iveness")
            {
            word.popBack(4);
            return;
            }
        else if (temp == (string)"fulness")
            {
            word.popBack(4);
            return;
            }
        else if (temp == (string)"ousness")
            {
            word.popBack(4);
            return;
            }
        }
    if (word.substr(-6) == (string)"biliti")
        {
        word.popBack(5);
        word += (string)"le";
        return;
        }
    if (size > 5)
        {
        
        }

    
    }


//algorithm from Algorithm for Suffix Stripping by M.F. Porter
string stem (string word) 
    {
    word = standardize(word);
    step1a(word);
    size_t m = countM(word);
    step1b(word, m);
    step1c(word);
    if (m > 0)
        step2(word);
    return word;
    }