#include "index.h"
#include "stemmer.h"
#include <iostream>

int main (int argc, char **argv) {
    while (true) {
        char buffer[1024];
        std::cout << "% ";
        std::cin >> buffer;
        string input(buffer);
        string stemmed(stem(input));
        std::cout << stemmed << std::endl;
    }
}