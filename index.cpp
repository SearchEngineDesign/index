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

void IndexReadHandler::testReader(string name) {
   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex(name.c_str());
   const SerialPost *eof = readHandler.Find("%")->Value()->getPost(10);
   for (int i = 0; i < readHandler.getBlob()->DocumentsInIndex; i++) {
      std::cout << readHandler.getDocument(i)->c_str() << std::endl;
      std::cout << "Post: " << readHandler.Find("%")->Value()->getPost(i) << std::endl;
   }
   const SerialString *str1 = readHandler.getDocument(0);
   const SerialString *str2 = readHandler.getDocument(52);
   
   std::cout << readHandler.getBlob()->DocumentsInIndex << " documents in index" << std::endl;
   std::cout << readHandler.getBlob()->WordsInIndex<< " words in index" << std::endl;
   std::cout << readHandler.getBlob()->keyCount << " unique tokens in index" << std::endl;
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
   index->optimizeDict();
   const IndexBlob *h = IndexBlob::Create(index);
   size_t n = h->BlobSize;
   write(fd, h, n); // write hash(index)blob to fd
   IndexBlob::Discard(h);
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
   if (index != nullptr)
      delete index;
   index = new Index();

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
   folder = foldername;

   UpdateIH();
}

void Index::addDocument(HtmlParser &parser) {
   Tuple<string, PostingList> *seek;
   string concat;
   //stem(parser.bodyWords);
   //stem(parser.headWords);
   //stem(parser.titleWords);
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