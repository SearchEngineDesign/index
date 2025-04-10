//index.h
#pragma once

#ifndef INDEX_H
#define INDEX_H

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cmath>
#include <bitset>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <cassert>

#include "../utils/string.h"
#include "../utils/vector.h"
#include "../utils/Utf8.h"
#include "../utils/HashTable.h"
#include "../parser/HtmlParser.h"
#include "../frontier/ReaderWriterLock.h"
//#include "./stemmer/stemmer.h"

const int MAX_CHUNKS = 4096;
const int MAX_INDEX_SIZE = 2000000; // ? 2mb ?
const int MAX_WRITES = 5;

class IndexBlob;
class SerialTuple;
class SerialString;

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
   uint8_t *data;

   int get_bytes(const uint8_t first_byte) const {
      uint8_t bytes = 0;
      uint8_t sentinel = 7;
      if (!(first_byte >> sentinel)) // ASCII
         return 1;
      while ((first_byte >> sentinel) & 1) {
         bytes++;
         sentinel--;
      }
      assert(bytes < 7);
      return bytes;
   }
public:
   
   Post() {
      data = nullptr;
   }

   Post(const uint8_t * data_in) {
      int bytes = get_bytes(data_in[0]);
      data = new uint8_t[bytes];
      std::memcpy(data, data_in, bytes);
      delete[] data_in;
   }

   // copy constructor
   Post(const Post& other) {
      int bytes = get_bytes(other.data[0]);
      data = new uint8_t[bytes];
      std::memcpy(data, other.data, bytes);
   }

   ~Post() {
      delete[] data;
   }

   uint8_t * getData() const {
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
      data = new uint8_t[bytes];
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
    void appendDelta(size_t &WordsInIndex, size_t &doc);

    void appendURLDelta(size_t &WordsInIndex, size_t &doc, bool owner);

   //Construct empty posting list for string str_in
   PostingList(Token type_in) : type(type_in) {}

   //Construct empty posting list for string str_in and type
   PostingList(char type_in) {
      switch (type_in) {
         case 'e':
            type = Token::EoD;
            break;
         case 'a':
            type = Token::Anchor;
            break;
         case 'b':
            type = Token::Body;
            break;
         case 't':
            type = Token::Title;
            break;
         case 'u':
            type = Token::URL;
            break;
      }

   }

   // Return list's appearances
   const size_t getUseCount() const {
      return list.size();
   }

   // Return list's document appearances 
   const size_t getDocCount() const {
      return documentCount;
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

   // Get ptr to actual post list
   const vector<Post> *getList() const {
      return &list;
   }

   // Get ptr to seek table
   const vector<std::pair<size_t, size_t>> *getSeekTable() const {
      return &SeekTable;
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

   // reserve n post space in list
   void setUseCount(const size_t n) {
      list.reserve(n);
   }

   // set row i of seek table
   void setSeekTable(const size_t & i, const std::pair<size_t, size_t> & pair) {
      SeekTable[i] = pair;
   }

   // set seek index
   void setSeekIndex(const size_t & seek){
      seekIndex = seek;
   }

   void addPost(const Post & p) {
      list.push_back(p);
   }

   // last position this word occured at
   size_t lastPos = 0;
   // last document this word occured in
   size_t lastDoc = 0;


   bool operator<(const PostingList& other) const {
      return this->list.size() > other.list.size();
   }

   bool operator>(const PostingList& other) const {
      return this->list.size() < other.list.size();
   }

private:
   friend class IndexBlob;

    //Common header
    size_t documentCount = 1;   //number of documents containing token
    Token type;             //variety of token

    //Type-specific data

    //Posts
   vector<Post> list;

    //Current magnitude of the SeekIndex for this PostingList
   uint8_t seekIndex = 0;
    //Seek list
    // Array of size_t pairs -- the first is the index of the post in list, the second is its real location
   vector<std::pair<size_t, size_t>> SeekTable;
   void UpdateSeek( size_t index, const size_t location ) {
      if (location >= (1 << seekIndex)) { // Is location >= 0x1, 0x10, 0x100, etc
         SeekTable.push_back(std::make_pair(index, location));
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

   const vector<string> *getDocuments() const {
      return &documents;
   }

   const HashTable<string, PostingList> *getDict() const {
      return &dict;
   }

   void optimizeDict() {
      dict.Optimize();
   }

   // add one posting list to the dictionary
   void setPostingList(const string & str, const PostingList & pl) {
      dict.Find(str, pl);
   }

private:

   HashTable<string, PostingList> dict;

   string titleMarker = string("@");
   string anchorMarker = string("$");
   string selfRefUrlMarker = string("#1");
   string otherRefUrlMarker = string("#0");
   string eodMarker = string("%");
   string headMarker = string("<");

};

// IndexHandler

class IndexHandler {
public:

   Index *index;
   IndexHandler() {};
   IndexHandler( const char * foldername );
   void UpdateIH();

   string &getFilename() {
      return fileString;
   }

   virtual ~IndexHandler() {
      delete folder;
   }

protected:
   ReaderWriterLock rw_lock;

   string fileString;
   int chunkID;

   const char * folder;
   int fd;
   void *map;
   int fsize = 0;

   char space = ' ';
   char endl = '\n';
   string EoF = "%";
};


class IndexWriteHandler : public IndexHandler 
{
public:
   IndexWriteHandler() : IndexHandler() {} 
   IndexWriteHandler( const char * foldername ) : IndexHandler( foldername ) {  }

   int addDocument(HtmlParser &parser) {
      WithWriteLock wl(rw_lock); 
      index->addDocument(parser);
      // TODO: better evaluation of size?
      if (index->WordsInIndex > MAX_INDEX_SIZE && writeCount < MAX_WRITES) {
         ++writeCount;
         WriteIndex();
         close(fd);
         if (writeCount == MAX_WRITES) { //end program
            return -1;
         } else {
            UpdateIH();
         }
         return 1;
      }
      return 0;
   }

   ~IndexWriteHandler() override {
      WriteIndex();
      if (msync(map, fsize, MS_SYNC) == -1) {
         perror("Error syncing memory to file");
         munmap(map, fsize);
      }
      if (munmap(map, fsize == -1)) {
         perror("Error un-mmapping the file");
      }
      close(fd);
      delete folder;
   }

   void WriteIndex();

private:
   int writeCount = 0;
};


class IndexReadHandler : public IndexHandler 
{
public:
   IndexReadHandler() {}
   IndexReadHandler( const char * filename ) : IndexHandler( filename ) {  }

   ~IndexReadHandler() override {
      close(fd);
   }

   const SerialTuple *Find( const char *key_in );
   const SerialString *getDocument( const size_t &index_in );

   //for testing - delete later
   static void testReader(string name, bool verbose);

   void ReadIndex(const char * fname);

   const IndexBlob* getBlob() {
      return blob;
   }

private:
   struct stat fileInfo;
   size_t FileSize( int f )
      {
      fstat( f, &fileInfo );
      return fileInfo.st_size;
      }

   IndexBlob* blob;
};

#endif