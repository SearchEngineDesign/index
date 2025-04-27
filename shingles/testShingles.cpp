#include <iostream>
#include <fstream>
#include <string>
#include <cf/vec.h>
#include "../Crawler/crawler.h"
#include "../parser/HtmlParser.h"
#include "shingle.h"
#include <cf/searchstring.h>

void testShingles(const string &filePath) {
    // Step 1: Read URLs from a text file
    std::cout << "Opening file: " << filePath.c_str() << std::endl;
    std::ifstream urlFile(filePath.c_str());
    if (!urlFile.is_open()) {
        std::cerr << "Failed to open the file: " << filePath.c_str() << std::endl;
        return;
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

    // Step 2: Initialize Crawler and ShingleMap
    Crawler crawler;
    ShingleMap shingleMap;

    // Step 3: Process each URL
    for (const auto &url : urls) {
        try {
            std::cout << "Processing URL: " << url << std::endl;
            // Parse the URL
            ParsedUrl parsedUrl(url.c_str());
            char buffer[BUFFER_SIZE];
            size_t pageSize = 0;

            // Step 4: Crawl the URL
            crawler.crawl(parsedUrl, buffer, pageSize);

            // Step 5: Parse the HTML content
            HtmlParser parser(buffer, pageSize);

            // Step 6: Check similarity
            if (shingleMap.isSimilar(parser.bodyWords)) {
                std::cout << "The document at " << url.c_str() << " is similar to an existing document." << std::endl;
            } else {
                std::cout << "The document at " << url.c_str() << " is not similar to any existing document." << std::endl;
                shingleMap.addDocument(parser.bodyWords);
            }
        } catch (const std::exception &e) {
            std::cerr << "Error processing URL " << url.c_str() << ": " << e.what() << std::endl;
        }
    }
}

int main() {
    try {
        string filePath = "shingletest.txt"; // Update this path to your actual file path
        std::cout << "Starting testShingles with file: " << filePath << std::endl;
        testShingles(filePath);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught in main" << std::endl;
        return 1;
    }
} 