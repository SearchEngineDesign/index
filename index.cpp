//index.cpp

#include "index.h"
#include "../utils/IndexBlob.h"

const SerialTuple *IndexReadHandler::Find(const char * key) {
   const SerialTuple *tup = blob->Find(key);
   const SerialPostingList *pl = tup->Value();
   // do whatever with the SPL
   const SerialPost* p = pl->getPost(0);
   return tup;
}

// Read entire index from memory mapped file
void IndexReadHandler::ReadIndex(const char * fname) {
   // Open the file for reading, map it, check the header,
   // and note the blob address.
   fd = open(fname, O_RDONLY);

   if (fd == -1) 
      perror("open");

   if (FileSize(fd) == -1) //get file size
      perror("fstat");

   blob = reinterpret_cast<IndexBlob*>(mmap(nullptr, fileInfo.st_size, 
                              PROT_READ, MAP_PRIVATE, fd, 0)); //map bytes to 'blob'
   if (blob == MAP_FAILED)
      perror("mmap");
}

void IndexWriteHandler::WriteIndex() {
   //should be optimizing hash to prioritize tokens that appear less
   //index->getDict()->topbuckets();
   const IndexBlob *h = IndexBlob::Create(index);
   size_t n = h->BlobSize;

   write(fd, h, n); // write hash(index)blob to fd
}

string nextChunk( const char * foldername) {
   char * out;
   const char * lastFile = "";
   int num = -1;
   for (const auto& entry : std::__fs::filesystem::directory_iterator(foldername)) {
      lastFile = entry.path().filename().c_str();
      if (atoi(lastFile) > num)
         num = atoi(lastFile);
   }
   if (num == -1)
      return string(foldername) + string("/") + string("0");
   num += 1;
   char newFile[5];
   snprintf(newFile, sizeof(newFile), "%d", num);
   return string(foldername) + string("/") +  string(newFile);
}

void IndexHandler::UpdateIH() {
   string fname = nextChunk(folder);
   fileString = fname;
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
      seek = dict.Find(i, PostingList(Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 0, DocumentsInIndex);
   }
   for (auto &i : parser.headWords) {
      seek = dict.Find(i, PostingList(Token::Body));
      seek->value.appendBodyDelta(WordsInIndex, 1, DocumentsInIndex);
   }
   for (auto &i : parser.titleWords) {
      concat = string(&titleMarker) + i;
      seek = dict.Find(concat, PostingList(Token::Title));
      seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);

   }
   for (auto &i : parser.links) {
      //TODO: implement a better way to index anchor text
      for (auto &j : i.anchorText) {
         concat = string(&anchorMarker) + j;
         seek = dict.Find(concat, PostingList(Token::Anchor));
         seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);
      }
      concat = string(&urlMarker) + i.URL;
      seek = dict.Find(concat, PostingList(Token::URL));
      seek->value.appendTitleDelta(WordsInIndex, DocumentsInIndex);
   }

   concat = string(&eodMarker);
   seek = dict.Find(concat, PostingList(Token::EoD));
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
