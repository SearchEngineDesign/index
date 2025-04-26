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
    if (word[i] == 'y') {
        if ( isVowel( word, i - 1 ) )
            return false;
        else
            return true;
    }
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
        int end = word.size() - 1;
    char last = word[end];
    if ( last == 'x' || last == 'y' || last == 'w' )
        return false;
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
    for (int i; i < size; i++)
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

//counts M without the suffix (denoted by size)
static inline int countMSubstr(const string& word, size_t suffix)
    {
    if (word.size() < suffix)
        return -1;
    return countM( word.substr(0, word.size() - suffix ) );
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
        std::cerr << "utf8proc has encountered an error. Word: " << word << std::endl;
        free(result);
        return word;
        }
    string newWord(result_len);
    for (int i = 0; i < result_len; i++)
        if ( isNum( result[i] ) || isLowerAlpha( result[i] ) )
            newWord.push_back(result[i]);
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
    int end = word.size() - 1;
    if ( word.substr(-2) == (string)"at" )
        word.push_back('e');
    else if ( word.substr(-2) == (string)"bl" )
        word.push_back('e');
    else if ( word.substr(-2) == (string)"iz" )
        word.push_back('e');
    else if ( doubleConsonant(word) && 
        ( word[end] != 'l' && word[end] != 's' && word[end] != 'z' ) )
        word.popBack();
    else if ( m == 1 && oCheck(word) )
        word.push_back('e');
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
    if ( size < 3 )
        return;
    if (size > 7 && countMSubstr(word, 7) )
        {
        const string temp = word.substr(-7);
        if (temp == (string)"ational")
            {
            word.popBack(5);
            word.push_back('e');
            return;
            }
        else if (temp == (string)"ization")
            {
            word.popBack(5);
            word.push_back('e');
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
    if (size > 6  && countMSubstr(word, 6) )
        {
        const string temp = word.substr(-6);
        if (temp == (string)"biliti")
            {
            word.popBack(5);
            word += (string)"le";
            return;
            }
        else if (temp == (string)"tional")
            {
            word.popBack(2);
            return;
            }
        }
    if (size > 5 && countMSubstr(word, 5) )
        {
        const string temp = word.substr(-5);
        if ( temp == (string)"entli" )
            {
            word.popBack(2);
            return;
            }
        else if ( temp == (string)"alism")
            {
            word.popBack(3);
            return;
            }
        else if ( temp == (string)"ation")
            {
            word.popBack(2);
            word[word.size() - 1] = 'e';
            return;
            }
        else if ( temp == (string)"aliti")
            {
            word.popBack(3);
            return;
            }
        else if ( temp == (string)"iviti" )
            {
            word.popBack(2);
            word[word.size() - 1] = 'e';
            return;
            }
        else if ( temp == (string)"ousli" )
            {
            word.popBack(2);
            }
        }
    if (size > 4 && countMSubstr(word, 4) ) 
        {
        const string temp = word.substr(-4);
        if ( temp == (string)"enci" )
            word[word.size() - 1] = 'e';
        else if ( temp == (string)"anci" )
            word[word.size() - 1] = 'e';
        else if ( temp == (string)"izer" )
            word.popBack();
        else if ( temp == (string)"abli" )
            word[word.size() - 1] = 'e';
        else if ( temp == (string)"alli" )
            word.popBack(2);
        else if ( temp == (string)"ator" )
            {
            word.popBack();
            word[word.size() - 1] = 'e';
            }
        }
    if (word.substr(-3) == (string)"eli" && countMSubstr(word, 3) )
        word.popBack(2);
    return;
    }
static inline void step3( string &word )
    {
    size_t size = word.size();
    if (size < 3)
        return;
    if (size > 5 && countMSubstr(word, 5) )
        {
        const string temp = word.substr(-5);
        if (temp == (string)"icate")
            {
            word.popBack(3);
            return;
            }
        else if (temp == (string)"ative")
            {
            word.popBack(5);
            return;
            }
        else if (temp == (string)"alize")
            {
            word.popBack(3);
            return;
            }
        else if (temp == (string)"iciti")
            {
            word.popBack(3);
            return;
            }
        }
    if (size > 4 && countMSubstr(word, 4) )
        {
        const string temp = word.substr(-4);
        if (temp == (string)"ical")
            {
            word.popBack(2);
            return;
            }
        else if (temp == (string)"ness")
            {
            word.popBack(4);
            return;
            }
        }
    if (word.substr(-3) == (string)"ful" && countMSubstr(word, 3) )
        word.popBack(3);
    return;
    }

static inline void step4 ( string &word )
    {
    size_t size = word.size();
    if (size < 1)
        return;
    if (size > 5)
        {
        const string temp = word.substr(-5);
        if (temp == (string)"ement")
            {
            word.popBack(5);
            }
        }
    if (size > 4 && countMSubstr(word, 4) > 1 )
        {
        const string temp = word.substr(-4);
        if (temp == (string)"ance" || temp == (string)"ence" || temp == (string)"able" || 
        temp == (string)"ible" || temp == (string)"ment")
            {
            word.popBack(4);
            return;
            }   
        else if (temp == (string)"tion" || temp == (string)"sion")
            {
            word.popBack(3);
            return;
            }
        }
    if (size > 3 && countMSubstr(word, 3) > 1 )
        {
        const string temp = word.substr(-3);
        if (temp == (string)"ant" || temp == (string)"ent" || temp == (string)"ism" || temp == (string)"ate" 
        || temp == (string)"iti" || temp == (string)"ous" || temp == (string)"ive" || temp == (string)"ize")
            {
            word.popBack(3);
            return;
            }   
        }
    if (size > 2 && countMSubstr(word, 2) > 1 )
        {
        const string temp = word.substr(-2);
        if (temp == (string)"al" || temp == (string)"er" || temp == (string)"ic" || temp == (string)"ou")
            {
            word.popBack(2);
            return;
            }
        }
    }

static inline void step5a(string& word)
    {
    size_t size = word.size();
    if (size < 3)
        return;
    if (word.substr(-1) == (string)"e")
        {
        word.popBack(1);
        return;
        }
    if (countMSubstr(word, 1) == 1) 
        {
        const string temp = word.substr(size - 1);
        if (!oCheck(temp) && word[size - 1] == 'e')
            word.popBack(1);
        }
    return;
    }

static inline void step5b(string& word)
    {
    size_t size = word.size();
    if (size < 3)
        return;
    if (word.substr(-1) == (string)"l" && word.substr(-2) == (string)"ll")
        word.popBack(1);
    return;
    }

//algorithm from Algorithm for Suffix Stripping by M.F. Porter
void stem (string s) 
    {
    s = standardize(s);
    step1a(s);
    size_t m = countM(s);
    step1b(s, m);
    step1c(s);
    if (m > 0)
        {
        step2(s);
        step3(s);
        }
    if (m > 1)
        step4(s);
    step5a(s);
    if (m > 1)
        step5b(s);
    }

void stem ( vector<string> v )
    {
    for ( string& s : v)
        stem( s );
    }

string stemWord(string s)
    {
    string temp = s;
    stem(temp);
    return temp;
    }