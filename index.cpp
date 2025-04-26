//index.cpp

#include "index.h"
#include <cf/IndexBlob.h>
#include <ctype.h>

ReaderWriterLock chunk_lock;

const SerialTuple *IndexReadHandler::Find(const char * key_in) {
   const SerialTuple *tup = blob->Find(key_in);
   return tup;
}

const SerialString *IndexReadHandler::getDocument( const size_t &index_in ) {
   const SerialString *str = blob->getDocument(index_in);
   return str;
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

void IndexReadHandler::TestIndex() {

   const SerialTuple *t = blob->Find("word");
   assert(t != nullptr);
   const SerialTuple *t1 = blob->Find("start");
   assert(t1 != nullptr);
   const SerialPost *p1 = t1->Value()->getPost(2);
   const SerialTuple *t2 = blob->Find("wikipedia");
   assert(t2 != nullptr);
}

void IndexWriteHandler::WriteIndex() {
   std::cerr << "Writing out blob!" << std::endl;
   
   //should be optimizing hash to prioritize tokens that appear less
   index->optimizeDict();
   const IndexBlob *h = IndexBlob::Create(index);
   size_t n = h->BlobSize;
   write(fd, h, n); // write hash(index)blob to fd
   IndexBlob::Discard(h);
   close(fd);
}

string nextChunk( const char * foldername, int &chunkID ) {
   char * out;
   chunkID= -1;
   for (const auto& entry : std::filesystem::directory_iterator(foldername)) {
      int currID = atoi(entry.path().filename().c_str());
      if (currID > chunkID)
         chunkID = currID;
   } 
   if (chunkID == -1) {
      chunkID = 0;
      return string(foldername) + string("/") + string("0");
   }
   chunkID += 1;
   char newFile[5];
   snprintf(newFile, sizeof(newFile), "%d", chunkID);
   return string(foldername) + string("/") +  string(newFile);
}

void IndexHandler::UpdateIH() {
   WithWriteLock wl(chunk_lock);
   if (index != nullptr)
      delete index;
   index = new Index();

   string fname = nextChunk(folder, chunkID);
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
   folder = foldername;
   UpdateIH();
}

void lowerize(string &i) {
   char * p = i.c_str();
   for ( ; *p; ++p) *p = tolower(*p);
}

void Index::addDocument(HtmlParser &parser) {
   Tuple<string, PostingList> *seek;
   string concat;
   /*stem(parser.bodyWords);
   stem(parser.headWords);
   stem(parser.titleWords);*/
   int n = 0;
   for (auto &i : parser.bodyWords) {
      lowerize(i);
      seek = dict.Find(i, PostingList(Token::Body));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   }
   for (auto &i : parser.headWords) {
      lowerize(i);
      concat = headMarker + i;
      seek = dict.Find(concat, PostingList(Token::Body));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   }
   for (auto &i : parser.titleWords) {
      lowerize(i);
      concat = titleMarker + i;
      seek = dict.Find(concat, PostingList(Token::Title));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);

   }
   for (auto &i : parser.links) {
      for (auto &j : i.anchorText) {
         lowerize(j);
         concat = anchorMarker + j;
         seek = dict.Find(concat, PostingList(Token::Anchor));
         seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
      }    
      if (parser.pURL.Host == i.URL.substr(parser.pURL.Service.length() + 3, parser.pURL.Host.length()))
         concat = selfRefUrlMarker + i.URL;
      else
         concat = otherRefUrlMarker + i.URL;
      seek = dict.Find(concat, PostingList(Token::URL));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   }

   seek = dict.Find(eodMarker, PostingList(Token::EoD));
   seek->value.appendDelta(WordsInIndex, DocumentsInIndex);


   concat = selfUrlMarker + parser.base;
   seek = dict.Find(concat, PostingList(Token::URL));
   seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   
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

uint8_t *formatUtf8(const size_t &delta) {
   size_t size = SizeOfCustomUtf8(delta);
   uint8_t *deltaUtf8 = new uint8_t[size];
   WriteCustomUtf8(deltaUtf8, delta, size);
   assert(deltaUtf8[0] != 0xfe);
   
   return deltaUtf8;
}

void PostingList::appendDelta(size_t &WordsInIndex, size_t &doc) {
   size_t delta = Delta(WordsInIndex, doc);
   list.emplace_back(formatUtf8(delta)); // TODO: memory leak?
   UpdateSeek(list.size()-1, WordsInIndex);
}
