//index.cpp

#include "index.h"

// read a delta in utf8
Post IndexReadHandler::ReadPost() {
   Utf8 firstByte;
   read(fd, &firstByte, sizeof(Utf8));
   size_t length = IndicatedLength(&firstByte);
   
   char *post = new char[length];
   post[0] = static_cast<char>(firstByte);
   read(fd, post + 1, length - 1);

   Post newPost(post); // this will delete[] post

   return newPost;
}


// Read a string from memory mapped file
string IndexReadHandler::ReadString() {
   size_t sz;
   read(fd, &sz, sizeof(size_t)); // size of string

   char *cString = new char[sz];
   for (int i = 0; i < sz; i++)
      read(fd, &cString[i], sizeof(char)); // content of string
   
   string str(cString, sz);
   delete[] cString;

   return str;
}


// read a posting list
void IndexReadHandler::ReadPostingList() {

   size_t useCount;
   size_t docCount;
   char type;
   char junk;

   string token = ReadString(); // associated token
   read(fd, &junk, sizeof(char));

   read(fd, &useCount, sizeof(size_t)); // use count
   read(fd, &junk, sizeof(char));

   read(fd, &docCount, sizeof(size_t)); // doc count
   read(fd, &junk, sizeof(char));

   read(fd, &type, sizeof(char)); // type of token
   read(fd, &junk, sizeof(char));

   PostingList postingList(token, type); // generate empty posting list
   postingList.setUseCount(useCount);
   postingList.setDocCount(docCount);

   read(fd, &postingList.lastPos, sizeof(size_t)); // last appearance position
   read(fd, &junk, sizeof(char));

   read(fd, &postingList.lastDoc, sizeof(size_t)); // last document appearance
   read(fd, &junk, sizeof(char));

   // read and generate seek index
   size_t seekIndex;
   read(fd, &seekIndex, sizeof(size_t)); // number of seek rows
   read(fd, &junk, sizeof(char));
   postingList.setSeekIndex(seekIndex);
   
   for (int i = 0; i < seekIndex; i++) {
      size_t highBit;
      std::pair<size_t, size_t> seekData;
      
      read(fd, &highBit, sizeof(size_t)); // seek row #i
      read(fd, &junk, sizeof(char));

      read(fd, &seekData.first, sizeof(size_t)); // index of post 
      read(fd, &junk, sizeof(char));

      read(fd, &seekData.second, sizeof(size_t)); // index of post 
      read(fd, &junk, sizeof(char));

      postingList.setSeekTable(i, seekData);
   }

   // Read posts
   for (int i = 0; i < useCount; i++) {
      postingList.addPost(ReadPost());
      read(fd, &junk, sizeof(char));
   }

   read(fd, &junk, sizeof(char));

   index->setPostingList(token, postingList);
}


// Read entrie index from memory mapped file
void IndexReadHandler::ReadIndex() {

   char junk; // store space and endl
   size_t uniqueTokenNum;

   read(fd, &uniqueTokenNum, sizeof(size_t)); // number of unique tokens
   read(fd, &junk, sizeof(char));
   read(fd, &index->WordsInIndex, sizeof(size_t)); // total number of tokens
   read(fd, &junk, sizeof(char));
   read(fd, &index->DocumentsInIndex, sizeof(size_t)); // number of documents
   read(fd, &junk, sizeof(char));

   index->documents.reserve(index->DocumentsInIndex); // document list in index
   for (int i = 0; i < index->DocumentsInIndex; i++) {
      index->documents.push_back(ReadString()); // every document in index
      read(fd, &junk, sizeof(char));
   }
   read(fd, &junk, sizeof(char));
   std::cout << uniqueTokenNum << std::endl;
   for (int i = 0; i < uniqueTokenNum; i ++) {
      ReadPostingList(); // posting list itself
   }
}

void IndexWriteHandler::WriteString(const string &str) {
   size_t sz = str.length();
   write(fd, &sz, sizeof(size_t)); // size of string
   for (int i = 0; i < sz; i++) 
      write(fd, str.at(i), sizeof(char)); // content of string
}

void IndexWriteHandler::WritePost(const Post &post) {
   write(fd, post.getData(), post.length());
}

void IndexWriteHandler::WritePostingList(const PostingList &list) {

}

void IndexWriteHandler::WriteIndex() {
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

string nextChunk( const char * foldername) {
   char * out;
   const char * lastFile = "";
   for (const auto& entry : std::__fs::filesystem::directory_iterator(foldername)) {
      lastFile = entry.path().filename().c_str();
   }
   if (*lastFile == '\0')
      return string(foldername) + string("/") + string("0");
   int num = atoi(lastFile);
   num += 1;
   char * newFile;
   sprintf(newFile, "%d", num);
   return string(foldername) + string("/") +  string(newFile);
}

void IndexHandler::UpdateIH() {
   string fname = nextChunk(folder);

   fd = open(fname.c_str(), O_RDWR | O_CREAT | O_APPEND, (mode_t)0600);
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
}

IndexHandler::IndexHandler( const char * foldername ) {
   int result;
   index = new Index();

   folder = foldername;

   UpdateIH();
}

void Index::addDocument(HtmlParser &parser) {
   Tuple<string, PostingList> *seek;
   string concat;
   for (auto &i : parser.bodyWords) {
      seek = dict.Find(i, PostingList(i, Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 0, DocumentsInIndex);
   }
   for (auto &i : parser.headWords) {
      seek = dict.Find(i, PostingList(i, Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 1, DocumentsInIndex);
   }
   for (auto &i : parser.titleWords) {
      concat = string(&titleMarker) + i;
      seek = dict.Find(concat, PostingList(concat, Token::Title));
      seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);

   }
   for (auto &i : parser.links) {
      //TODO: implement a better way to index anchor text
      for (auto &j : i.anchorText) {
         concat = string(&anchorMarker) + j;
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
   // TODO: capable of encoding 31 bits ( Utf-32 ) but only Unicode in common use
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
   list.emplace_back(formatUtf8(delta)); // TODO: memory leak? change to Utf8.h, unsigned char vs char
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
