#include "index.h"

int main() {
   IndexReadHandler ih("testPL");
   ih.ReadIndex();
   Index *index = ih.index;

   std::cout << index->WordsInIndex << " " << index->DocumentsInIndex << std::endl;

   for (int i = 0; i < index->documents.size(); i ++) {
      std::cout << index->documents[i] << " ";
   }
   std::cout << std::endl;

   // dictionary
   HashTable<string, PostingList> *dict = index->getDict();


   for (auto it = dict->begin(); it != dict->end(); it ++) {
      // posting list
      PostingList pl = it->value;

      std::cout << "token: " << pl.getIndex() << " ";
      // std::cout << "use count: " << pl.getUseCount() << std::endl;
      // std::cout << "type: " << pl.getType() << std::endl;

      const vector<Post> *list = pl.getList(); // vector of deltas
      for (int i = 0; i < list->size(); i ++) {
         Post p = (*list)[i];
         char *data = p.getData();
         std::cout << GetCustomUtf8(reinterpret_cast<Utf8*>(data)) << " ";
      }
      std::cout << std::endl;
   
   }

}