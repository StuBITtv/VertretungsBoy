//
// Created by StuBIT on 12/20/17.
//

#ifndef VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
#define VERTRETUNGSBOY_VERTRETUNGSBOY_HPP

#include <string>
#include <vector>
#include <sqlite3.h>

namespace VertretungsBoy {

    class plan {

    public:
        plan(std::vector<std::string> urls, std::string dbPath);

        int update();

        static bool curlGlobalInit;

    private:
        std::vector<std::string> htmls;
        std::vector<std::string> urls;
        bool download(size_t urlsIndex);
        static size_t curlWriter(char *ptr, size_t size, size_t n, void *userData);

        std::vector<std::string> dates;
        std::vector<std::vector<std::vector<std::string>>> tables;
        std::vector<std::vector<std::string>> parser(const std::string &html);
        std::string toUTF8(char token);
        void tableWriter(std::string tokens, std::string &output);
        bool replace = false, styleElement = false;
        size_t replaceCounter = 0;


        std::string dbPath;
        sqlite3 *db = nullptr;
        int writeTablesToDatabase();
    };
};

#endif //VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
