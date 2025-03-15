//indexBlob.h

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
#include "index.h"

using HashBucket = Bucket< string, PostingList >;

static const size_t Unknown = 0;

size_t RoundUp( size_t length, size_t boundary ) {
   // Round up to the next multiple of the boundary, which
   // must be a power of 2.

   static const size_t oneless = boundary - 1,
      mask = ~( oneless );
   return ( length + oneless ) & mask;
}

size_t BytesForString( const string &s ) {
    return s.capacity() + (sizeof(size_t) >> 1);
}

struct SerialTuple {

public:

    size_t Length, Value;

    // The Key will be a C-string of whatever length.
    char Key[ Unknown ];

    // Calculate the bytes required to encode a HashBucket as a
    // SerialTuple.

    static size_t BytesForPostingList( const PostingList *p ) {
        size_t size = 0;
        size += sizeof(size_t) >> 1; // usecount, documentcount
        size += sizeof(Token); // Type 
        size += BytesForString(p->getIndex());

        const vector<Post> *list = p->getList();
        size += sizeof(size_t) >> 1; // MVs of list
        for (auto i : *list)
            size += i.length(); // Strings of document

        return size + sizeof(char);
    }

    static size_t BytesRequired( const HashBucket *b ) {
        //Value, Length
        size_t size = sizeof(size_t) * 2;
        //string size. 
        size += b->tuple.key.length() + sizeof(char);
        size += BytesForPostingList(&(b->tuple.value));
        return RoundUp(size, sizeof(size_t));
    }

    // Write the HashBucket out as a SerialTuple in the buffer,
    // returning a pointer to one past the last character written.

    static char *Write( char *buffer, size_t len,
            const HashBucket *b ) {
        //???
    }
};

class IndexBlob
    {
    // This will be a hash specifically designed to hold an
    // entire hash table as a single contiguous blob of bytes.
    // Pointers are disallowed so that the blob can be
    // relocated to anywhere in memory

    // The basic structure should consist of some header
    // information including the number of buckets and other
    // details followed by a concatenated list of all the
    // individual lists of tuples within each bucket.

    public:

        // Define a MagicNumber and Version so you can validate
        // a HashBlob really is one of your HashBlobs.

        size_t MagicNumber,
            Version,
            BlobSize,
            NumberOfBuckets,
            Buckets[ Unknown ];

        // The SerialTuples will follow immediately after.


        const SerialTuple *Find( const char *key ) const
            {
            // Search for the key k and return a pointer to the
            // ( key, value ) entry.  If the key is not found,
            // return nullptr.

            // Your code here.
            size_t index = Hash::hashbasic(key, NumberOfBuckets);
            size_t bucketStart = Buckets[index];
            SerialTuple *curr = reinterpret_cast<SerialTuple*>((char *)this + bucketStart);

            size_t bucketEnd;
            (index == NumberOfBuckets - 1) ? bucketEnd = BlobSize : bucketEnd = Buckets[index+1];

            while (bucketStart < bucketEnd)
            {
            if (!strcmp(curr->Key, key))
            {
                return curr;
            } else {
                bucketStart += curr->Length;
                curr = reinterpret_cast<SerialTuple*>((char *)this + bucketStart);
            }
            
            }

            return nullptr;
            }


        static size_t BytesRequired( const Index *index, size_t &offset ) {
            offset += sizeof(size_t) >> 1; // MVs of document
            for (auto i : index->documents)
                offset += BytesForString(i); // Strings of document
            offset += sizeof(size_t) >> 1; // WordsInIndex, DocumentsInIndex

            // Take care of dict
            const HashTable<string, PostingList> *dict = index->getDict();
            int dictSize = dict->size();
            offset += (dictSize+4) * sizeof(size_t);
            size_t bucketSpace = 0;
            for (int i = 0; i < dictSize; i++) {
                HashBucket *curr = dict->at(i);
                while (curr != nullptr) {
                    bucketSpace += SerialTuple::BytesRequired(curr);
                    curr = curr->next;
                }
            }

            return offset + bucketSpace + (sizeof(char) >> 2); // Final 4 char MVs
        }

        // Write a HashBlob into a buffer, returning a
        // pointer to the blob.

        static HashBlob *Write( HashBlob *hb, size_t bytes,
            const Hash *hashTable )
            {

            return nullptr;
            }

        // Create allocates memory for a HashBlob of required size
        // and then converts the HashTable into a HashBlob.
        // Caller is responsible for discarding when done.

        // (No easy way to override the new operator to create a
        // variable sized object.)

        static IndexBlob *Create( const Index *index )
            {
            //allocating the whole blob
            const HashTable<string, PostingList> *dict = index->getDict();
            size_t offset;
            size_t size = IndexBlob::BytesRequired( index, offset );
            char *mem = new char[size];
            IndexBlob *blob = reinterpret_cast<IndexBlob*>(mem);

            blob->MagicNumber = 15276156;
            blob->Version = 1;
            blob->BlobSize = size;
            blob->NumberOfBuckets = dict->size();

            for (int i = 0; i < dict->size(); i++) {
                blob->Buckets[i] = offset;
                HashBucket *curr = dict->at(i);
                
                while (curr != nullptr) {
                    size_t tSize = SerialTuple::BytesRequired(curr);
                    SerialTuple::Write(mem + offset, tSize, curr);
                    offset += tSize;
                    curr = curr->next;
                }
            }
            
            return blob;
            }

        // Discard

        static void Discard( IndexBlob *blob )
            {
            // Your code here.
            }
    };