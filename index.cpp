//index.cpp

#include "index.h"

void IndexHandler::ReadIndex() {
 
}

void IndexHandler::WriteString(const string &str) {
   size_t sz = str.length();
   write(fd, &sz, sizeof(size_t)); // size of string
   for (int i = 0; i < sz; i++) 
      write(fd, str.at(i), sizeof(char)); // content of string
}

void IndexHandler::WritePost(const Post &post) {
   write(fd, post.getData(), post.length());
}

void IndexHandler::WritePostingList(const PostingList &list) {

   size_t useCount = list.getUseCount();
   size_t docCount = list.getDocCount();
   char type = list.getType();
   WriteString(list.getIndex()); // associated token
   write(fd, &endl, sizeof(char));
   write(fd, &useCount, sizeof(size_t)); // use count
   write(fd, &space, sizeof(char));
   write(fd, &docCount, sizeof(size_t)); // doc count
   write(fd, &space, sizeof(char));
   write(fd, &type, sizeof(char)); // type of token
   write(fd, &space, sizeof(char));
   write(fd, &list.lastPos, sizeof(size_t)); // last appearance position
   write(fd, &space, sizeof(char));
   write(fd, &list.lastDoc, sizeof(size_t)); // last document appearance
   write(fd, &endl, sizeof(char));

   size_t seekIndex = list.getSeekIndex();
   const std::pair<size_t, size_t> * seekTable = list.getSeekTable();
   write(fd, &seekIndex, sizeof(size_t)); // number of seek rows
   write(fd, &endl, sizeof(char));
   
   for (int i = 0; i < seekIndex; i++) {
      size_t highBit = 1 << i;
      write(fd, &highBit, sizeof(size_t)); // seek row #i
      write(fd, &space, sizeof(char));
      write(fd, &seekTable[i].first, sizeof(size_t)); // index of post 
      write(fd, &space, sizeof(char));
      write(fd, &seekTable[i].second, sizeof(size_t)); // index of post 
      write(fd, &endl, sizeof(char));
   }

   const vector<Post> *pl = list.getList();

   for (int i = 0; i < useCount; i++) {
      WritePost(pl->operator[](i));
      write(fd, &space, sizeof(char));
   }

   write(fd, &endl, sizeof(char));
}

void IndexHandler::WriteIndex() {
   HashTable<string, PostingList> *dict = index->getDict();
   size_t n = dict->getKeyCount();

   write(fd, &n, sizeof(size_t)); // number of unique tokens
   write(fd, &space, sizeof(char));
   write(fd, &index->WordsInIndex, sizeof(size_t)); // total number of tokens
   write(fd, &space, sizeof(char));
   write(fd, &index->DocumentsInIndex, sizeof(size_t)); // number of documents
   write(fd, &endl, sizeof(char));

   for (int i = 0; i < index->DocumentsInIndex; i++) {
      WriteString(index->documents[i]); // every document in index
      write(fd, &space, sizeof(char));
   }
   write(fd, &endl, sizeof(char));

   for (auto i = dict->begin(); i != dict->end(); i++) {
      WritePostingList(i->value); // posting list itself
   }
}

IndexHandler::IndexHandler( const char * filename ) {
   int result;
   index = new Index();

   fd = open(filename, O_RDWR | O_CREAT | O_APPEND, (mode_t)0600);
   if (fd == -1) {
      std::cerr << "Error opening index file";
	   exit(1);
   }

   struct stat sb;
   if (fstat(fd, &sb) == -1) {
      perror("Error getting file size");
      close(fd);
      exit(1);
   }
   fsize = sb.st_size;

   result = lseek(fd, fsize-1, SEEK_SET);
   /*if (result == -1) {
      WriteIndex();
      close(fd);
      perror("Error calling lseek(): no index in file");
      exit(1);
   }

   map = mmap(nullptr, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (map == MAP_FAILED) {
      close(fd);
      std::cerr << "Error mapping index file";
      exit(EXIT_FAILURE);
   } */
   
   ReadIndex();
}

void Index::addDocument(HtmlParser &parser) {
   Tuple<string, PostingList> *seek;
   string concat;
   for (auto i : parser.bodyWords) {
      seek = dict.Find(i, PostingList(i, Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 0, DocumentsInIndex);
   }
   for (auto i : parser.headWords) {
      seek = dict.Find(i, PostingList(i, Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 1, DocumentsInIndex);
   }
   for (auto i : parser.titleWords) {
      concat = string(&titleMarker) + i;
      seek = dict.Find(concat, PostingList(concat, Token::Title));
      seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);

   }
   for (auto i : parser.links) {
      //TODO: implement a better way to index anchor text
      for (int j = 0; j < i.anchorText.size(); j++) {
         concat = string(&anchorMarker) + i.anchorText[j];
         seek = dict.Find(concat, PostingList(concat, Token::Anchor));
         seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);
      }
      concat = string(&urlMarker) + i.URL;
      seek = dict.Find(concat, PostingList(concat, Token::URL));
      seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);
   }

   concat = string(&eodMarker);
   seek = dict.Find(concat, PostingList(concat, Token::EoD));
   seek->value.appendEODDelta(WordsInIndex, DocumentsInIndex);
   
   DocumentsInIndex += 1;
   documents.push_back(parser.base);
   std::cout << WordsInIndex << std::endl;
}

//for utillity
uint8_t bitsNeeded(const size_t n) {
    if (n == 0) {
        return 1; 
    }
    return std::max(1, static_cast<int>(std::ceil(std::log2(n + 1))));
}

char *formatUtf8(const size_t &delta) {
   const uint8_t boundary = bitsNeeded(delta);
   size_t bytes = 0;
   if (boundary < 7)
      bytes = 1;
   else if (boundary < 12)
      bytes = 2;
   else if (boundary < 17)
      bytes = 3;
   else if (boundary < 22)
      bytes = 4;
   else if (boundary < 27)
      bytes = 5;
   else if (boundary < 32)
      bytes = 6;
   else if (boundary < 37)
      bytes = 7;

   char* bitset = new char[bytes];
   uint8_t bitsetIndex = 0, initDelta = 0, deltaIndex = 0, index = bytes;
   
   while(deltaIndex < boundary) { 
      if (bitsetIndex % 8 == 0)
         index--;
      initDelta = deltaIndex + 6;
      for (; deltaIndex < initDelta && deltaIndex < boundary; deltaIndex++) {
         if ((delta >> deltaIndex) & 1)
            bitset[index] |= 1 << bitsetIndex;
         bitsetIndex++;
      }
      if (deltaIndex < boundary) {
         bitset[index] |= 0 << bitsetIndex;
         bitset[index] |= 1 << (bitsetIndex + 1);
         bitsetIndex += 2;
      }
      bitsetIndex = bitsetIndex % 8;
   }
   for (int i = 7; i > 7 - bytes; i--)
      bitset[0] |= 1 << i;

   return bitset;
}

void PostingList::appendTitleDelta(size_t &WordsInIndex, size_t &doc) {
   size_t delta = Delta(WordsInIndex, doc);
   list.emplace_back(formatUtf8(delta));
   UpdateSeek(list.size()-1, WordsInIndex);
}

void PostingList::appendBodyDelta(size_t &WordsInIndex, const uint8_t style, size_t &doc) {
   size_t delta = Delta(WordsInIndex, doc);
   delta = delta << 1;
   delta += style;
   list.emplace_back(formatUtf8(delta)); 
   UpdateSeek(list.size()-1, WordsInIndex);
}

void PostingList::appendEODDelta(size_t &WordsInIndex, const size_t doc) {
   size_t delta = Delta(WordsInIndex, doc);
   delta = delta << sizeof(doc);
   delta += doc;
   list.emplace_back(formatUtf8(delta));  
   UpdateSeek(list.size()-1, WordsInIndex);
}
