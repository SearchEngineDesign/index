#include <iostream>
#include <fstream>
#include <string>
#include <cf/vec.h>
#include "../Crawler/crawler.h"
#include "../parser/HtmlParser.h"
#include "shingle.h"
#include <cf/searchstring.h>

int main() {
    std::cout << "Starting testShingles" << std::endl;
    
    // Step 1: Read URLs from a text file
    string filePath = "shingletest.txt";
    std::cout << "Opening file: " << filePath.c_str() << std::endl;
    std::ifstream urlFile(filePath.c_str());
    if (!urlFile.is_open()) {
        std::cerr << "Failed to open the file: " << filePath.c_str() << std::endl;
        return 1;
    }
    std::cout << "Opened file: " << filePath.c_str() << std::endl;
    
    vector<string> urls;
    string url;
    std::cout << "Reading URLs from file..." << std::endl;
    while (urlFile >> url) {  // Use >> operator to read each URL
        urls.push_back(url);
    }
    std::cout << "Read " << urls.size() << " URLs from file." << std::endl;
    urlFile.close();

    if (urls.empty()) {
        std::cerr << "No URLs found in file." << std::endl;
        return 1;
    }

    // Step 2: Initialize Crawler and ShingleMap
    Crawler crawler;
    ShingleMap shingleMap;

    // Step 3: Process each URL
    for (const auto &url : urls) {
        // Parse the URL
        ParsedUrl parsedUrl(url.c_str());
        auto buffer = std::make_unique<char[]>(BUFFER_SIZE);
        size_t pageSize = 0;

        // Step 4: Crawl the URL
        crawler.crawl(parsedUrl, buffer.get(), pageSize);

        // Step 5: Parse the HTML content
        HtmlParser parser(buffer.get(), pageSize);

        // Step 6: Check similarity
        if (shingleMap.isSimilar(parser.bodyWords)) {
            std::cout << "The document at " << url.c_str() << " has a similarity." << std::endl;
        } else {
            std::cout << "The document at " << url.c_str() << " doesn't have a similarity." << std::endl;
            shingleMap.addDocument(parser.bodyWords);
        }
    }
    return 0;
} 