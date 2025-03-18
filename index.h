//index.h

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <cmath>
#include <bitset>

#include "../include/string.h"
#include "../include/vector.h"
#include "../include/Utf8.h"
#include "hash/HashTable.h"
#include "../parser/HtmlParser.h"

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

typedef size_t Location; // Location 0 is the null location.
typedef size_t FileOffset;

class Post {
private:
    //Variable byte size char array, to be encoded in utf-8.
    //Structure: n bits to encode the offset + 2 bits to encode style (for word tokens)
    //n bits to encode the offset + log(count) bits to number of times the anchor text 
        //the word belongs to has occurred with the URL it links to (for anchor text)
    //n bits to encode the EOF + n bits to encode an index to the corresponding URL for EOF tokens
    char *data;

    int get_bytes(const char first_byte) const {
        uint8_t bytes = 0;
        uint8_t sentinel = 7;
        while ((first_byte >> sentinel) & 1) {
            bytes++;
            sentinel--;
        }
        return bytes;
    }
public:
   
    Post() {
        data = nullptr;
    }

    Post(const char * data_in) {
        int bytes = get_bytes(data_in[0]);
        data = new char[bytes];
        std::memcpy(data, data_in, bytes);
        delete[] data_in;
    }

    ~Post() {
        if (data != nullptr)
            delete[] data;
    }

    char * getData() const {
        return data;
    }

    int length() const {
        if (data == nullptr)
            return 0;
        return get_bytes(data[0]);
    }

    Post &operator=(const Post &other) {
        if (data != nullptr)
            delete[] data;
        int bytes = get_bytes(other.data[0]);
        data = new char[bytes];
        std::memcpy(data, other.data, bytes);
        return *this;
    }

    void printBits() {
        for (int i = 0; i < get_bytes(data[0]); i++) {
            for (int j = 7; j >= 0; j--) {
                std::cout << (data[i] >> j & 1);
            }
            std::cout << ' ';
        }
        std::cout << std::endl;
    }

    Style getStyle() {
        //TODO
        return Style::Normal;
    }

    Location getDelta() {
        //TODO
        return 0;
    }
};

class PostingList {
public:
    //virtual Post *Seek( Location );
    void appendTitleDelta(size_t &WordsInIndex, size_t &doc); //title token
    void appendBodyDelta(size_t &WordsInIndex, uint8_t style, size_t &doc); //body token
    void appendEODDelta(size_t &WordsInIndex, const size_t doc); //EOF token

    //Construct empty posting list for string str_in
    PostingList(string &str_in, Token type_in) : 
                token(str_in), type(type_in) {}

    // Return list's appearances
    const size_t getUseCount() const {
        return list.size();
    }

    // Return list's document appearances 
    const size_t getDocCount() const {
        return list.size();
    }

    // Return list's document appearances 
    const char getType() const {
        switch (type) {
            case Token::EoD:
                return 'e';
            case Token::Anchor:
                return 'a';
            case Token::Body:
                return 'b';
            case Token::Title:
                return 't';
            case Token::URL:
                return 'u';
            default:
                return '0';
        }
    }

    // Return list's token
    string getIndex() const {
        return token;
    }

    // Get ptr to actual post list
    const vector<Post> *getList() const {
        return &list;
    }

    // Get ptr to seek table
    const std::pair<size_t, size_t> *getSeekTable() const {
        return SeekTable;
    }

    // Get seek index
    size_t getSeekIndex() const {
        return seekIndex;
    }

    // Update and assign delta of PostingList
    size_t Delta(size_t &WordsInIndex, const size_t doc) {
        size_t ret = WordsInIndex - lastPos;
        lastPos = WordsInIndex;
        ++WordsInIndex;
        if (doc != lastDoc) {
            lastDoc = doc;
            ++documentCount;
        }
        return ret;
    }

    // last position this word occured at
    size_t lastPos = 0;
    // last document this word occured in
    size_t lastDoc = -1;

private:

    //Common header
    size_t documentCount;   //number of documents containing token
    Token type;             //variety of token

    //Type-specific data

    //Token
    string token;

    //Posts
    vector<Post> list;

    //Current magnitude of the SeekIndex for this PostingList
    size_t seekIndex = 0;
    //Seek list
    // Array of size_t pairs -- the first is the index of the post in list, the second is its real location
    std::pair<size_t, size_t> SeekTable[256];
    void UpdateSeek( size_t index, const size_t location ) {
        if (location >= (1 << seekIndex)) { // Is location >= 0x1, 0x10, 0x100, etc
            SeekTable[seekIndex] = std::make_pair(index, location);
            seekIndex++;
        }
    }

};

class Index {
public:

    // addDocument should take in parsed HTML and add it to the index.
    void addDocument(HtmlParser &parser);
    size_t WordsInIndex = 0, 
    DocumentsInIndex = 0;

    vector<string> documents;
    

    Index() {}

    HashTable<string, PostingList> *getDict() {
        return &dict;
    }

private:

    HashTable<string, PostingList> dict;

    char titleMarker = '@';
    char anchorMarker = '$';
    char urlMarker = '#';
    char eodMarker = '%';
};

// IndexHandler

class IndexHandler {
public:
    Index *index;
    IndexHandler() {};
    IndexHandler( const char * filename );
    ~IndexHandler() {
        WriteIndex();
        if (msync(map, fsize, MS_SYNC) == -1) {
            perror("Error syncing memory to file");
            munmap(map, fsize);
        }
        if (munmap(map, fsize == -1)) {
	        perror("Error un-mmapping the file");
        }
        close(fd);
        
    };

private:
    int fd;
    void *map;
    int fsize = 0;
    void WriteString(const string &str);
    void WritePost(const Post &post);
    void WritePostingList(const PostingList &list);
    void WriteIndex();
    void ReadIndex();

    char space = ' ';
    char endl = '\n';
    string EoF = "%";
};