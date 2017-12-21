//
// Created by pi on 12/20/17.
//

#ifndef VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
#define VERTRETUNGSBOY_VERTRETUNGSBOY_HPP

#include <string>
#include <vector>

namespace VertretungsBoy {

    class plan {

    public:
        plan(std::vector<std::string> urls, std::string dbPath);
        plan(std::vector<std::string> urls);
        plan(std::string url);
        int update();

        static bool curlGlobalInit;

    private:
        std::vector<std::string> curlDownloads;
        std::vector<std::string> urls;
        bool download(size_t urlsIndex);
        static size_t writer(char *ptr, size_t size, size_t nmemb, void *userdata);

        bool parser(bool table, size_t curlDownloadsIndex); //true table, false date
        std::vector<std::vector<std::string>> table;
        std::string dbPath;
    };
};

#endif //VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
