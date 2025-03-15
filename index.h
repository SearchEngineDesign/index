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

    int get_bytes(const char first_byte) {
        uint8_t bytes = 0;
        uint8_t sentinel = 7;
        while (first_byte >> sentinel & 1) {
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

    int length() {
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
    void appendTitleDelta(size_t delta); //title token
    void appendBodyDelta(size_t delta, uint8_t style); //body token
    void appendEODDelta(size_t delta, size_t docIndex); //EOF token

    //Construct empty posting list for string str_in
    PostingList(string &str_in, Token type_in) : 
                index(str_in), type(type_in), useCount(1) {}

    //Get size of post list (in bytes)
    size_t getListSize() {
       return sizeof(list);
    }

    //Get ptr to list
    const vector<Post> *getList() const {
       return &list;
    }

    string getIndex() const {
        return index;
    }

private:
    //Common header
    size_t useCount;        //number of times token occurs
    size_t documentCount;   //number of documents containing token
    Token type;             //variety of token

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

    // addDocument should take in parsed HTML and add it to the index.
    void addDocument(HtmlParser &parser);
    vector<string> documents;
    size_t WordsInIndex = 1, 
    DocumentsInIndex = 0;

    Index() {}

    const HashTable<string, PostingList> *getDict() const {
        return &dict;
    }

private:
    HashTable<string, PostingList> dict;

    char titleMarker = '@';
    char anchorMarker = '$';
    char urlMarker = '#';
    char eodMarker = '%';
};

class IndexHandler {
public:
    Index *index;
    IndexBlob *ib;
    IndexHandler( const char * filename );
    ~IndexHandler() {
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
    void WriteIndex();
    void ReadIndex();
};
