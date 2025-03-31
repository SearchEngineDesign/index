//index.h
#pragma once

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cmath>
#include <bitset>
#include <iostream>
#include <cstring>
#include <filesystem>

#include "../utils/string.h"
#include "../utils/vector.h"
#include "../utils/Utf8.h"
#include "../utils/HashTable.h"
#include "../parser/HtmlParser.h"
#include "post.h"
#include "../isr/isr.h"

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

   // add one posting list to the dictionary
   void setPostingList(const string & str, const PostingList & pl) {
      dict.Find(str, pl);
   }

   // open ISR
   ISRWord *OpenISRWord( char *word ) {
      ISRWord *isrWord = new ISRWord;
      string str(word);
      PostingList *postingList = &(dict.Find(str)->value); // TODO: deal with word

      isrWord->SetPostingList(postingList);

      return isrWord;
   }

   // ISRWord *OpenISREndDoc( ) {
   //    ISRWord* ISREndDoc = new ISRWord;

   //    const vector<Post> *list = dict["%"].getList();
   //    ISREndDoc->SetCurrentPost(&list[0]);

   //    // TODO: SetDocumentLength, SetTitleLength, SetUrlLength

   //    return ISREndDoc;
   // }

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
   IndexHandler( const char * foldername );
   virtual ~IndexHandler() {}

   const int MAX_CHUNKS = 4096;
   const int MAX_INDEX_SIZE = 2000000; // ? 2mb ?

protected:
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
   IndexWriteHandler() {}
   IndexWriteHandler( const char * filename ) : IndexHandler( filename ) {  }

   ~IndexWriteHandler() override {
      WriteIndex();
      // if (msync(map, fsize, MS_SYNC) == -1) {
      //    perror("Error syncing memory to file");
      //    munmap(map, fsize);
      // }
      // if (munmap(map, fsize == -1)) {
      //    perror("Error un-mmapping the file");
      // }
      close(fd);
   }

private:
   void WriteString(const string &str);
   void WritePost(const Post &post);
   void WritePostingList(const PostingList &list);
   void WriteIndex();
};


class IndexReadHandler : public IndexHandler 
{
public:
   IndexReadHandler() {}
   IndexReadHandler( const char * filename ) : IndexHandler( filename ) {  }

   ~IndexReadHandler() override {
      close(fd);
   }

   void ReadIndex();

private:
   Post ReadPost();
   void ReadPostingList();
   string ReadString();
};