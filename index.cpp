//index.cpp

#include "index.h"
#include "../utils/IndexBlob.h"


const SerialTuple *IndexReadHandler::Find(const char * key_in) {
   const SerialTuple *tup = blob->Find(key_in);
   return tup;
}

const SerialString *IndexReadHandler::getDocument( const size_t &index_in ) {
   const SerialString *str = blob->getDocument(index_in);
   return str;
}  

void IndexReadHandler::testreader() {
   IndexReadHandler ihr;
   ihr.ReadIndex("./log/chunks/0");
   const SerialTuple *t = ihr.Find("hello");
   const SerialString *s = t->Key();
   const SerialPostingList *p = t->Value();
}

const SerialUrlTuple *IndexReadHandler::FindUrl(const char * key_in) {
   const SerialUrlTuple *tup = nullptr;
   UrlBlob *init = ublob;
   int i = 0;
   // iterate through blobs
   while (i < ufileInfo.st_size) {
      tup = ublob->FindUrl(key_in);
      i += ublob->BlobSize;
      if (tup == nullptr)
         ublob = reinterpret_cast<UrlBlob*>((char*)ublob + ublob->BlobSize);
      else
         break;
   }
   ublob = init;
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

void IndexReadHandler::ReadUrlBlob(const char * fname) {
   // Open the file for reading, map it, check the header,
   // and note the blob address.
   ufd = open(fname, O_RDONLY);

   if (ufd == -1) 
      perror("open");

   if (uFileSize(ufd) == -1) //get file size
      perror("fstat");

   ublob = reinterpret_cast<UrlBlob*>(mmap(nullptr, ufileInfo.st_size, 
                              PROT_READ, MAP_PRIVATE, ufd, 0)); //map bytes to 'blob'
   if (ublob == MAP_FAILED)
      perror("mmap");
}

void IndexWriteHandler::WriteIndex() {
   //should be optimizing hash to prioritize tokens that appear less
   index->optimizeDict();
   const IndexBlob *h = IndexBlob::Create(index);
   size_t n = h->BlobSize;
   write(fd, h, n); // write hash(index)blob to fd
   IndexBlob::Discard(h);

   /*const UrlBlob *u = UrlBlob::Create(index, chunkID);
   n = u->BlobSize;
   int ufd = open("./log/frontier/UrlBlob", O_RDWR | O_CREAT | O_APPEND, (mode_t)0600);
   if (ufd == -1) {
      std::cerr << "Error opening index file";
	   exit(1);
   }
   write(ufd, u, n); // write urlblob to ufd
   UrlBlob::Discard(u);*/
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

void Index::addDocument(HtmlParser &parser) {
   Tuple<string, PostingList> *seek;
   string concat;
   /*stem(parser.bodyWords);
   stem(parser.headWords);
   stem(parser.titleWords);*/
   int n = 0;
   for (auto &i : parser.bodyWords) {
      seek = dict.Find(i, PostingList(Token::Body));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   }
   for (auto &i : parser.headWords) {
      concat = headMarker + i;
      seek = dict.Find(concat, PostingList(Token::Body));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);
   }
   for (auto &i : parser.titleWords) {
      concat = titleMarker + i;
      seek = dict.Find(concat, PostingList(Token::Title));
      seek->value.appendDelta(WordsInIndex, DocumentsInIndex);

   }
   for (auto &i : parser.links) {
      for (auto &j : i.anchorText) {
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