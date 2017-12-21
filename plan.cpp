//
// Created by pi on 12/20/17.
//

#include "plan.hpp"
#include <curl/curl.h>
#include <iostream>

bool VertretungsBoy::plan::curlGlobalInit = false;

VertretungsBoy::plan::plan(std::vector<std::string> urls, std::string dbPath) : urls(urls), dbPath(dbPath) {
    if(!VertretungsBoy::plan::curlGlobalInit){
        curl_global_init(CURL_GLOBAL_DEFAULT);
        VertretungsBoy::plan::curlGlobalInit = true;
    }

    curlDownloads.resize(urls.size());

    /*  TODO: Check if data-bank exist
     *
     *  true: load old
     *  false: make nothing until the next update
     */
}

VertretungsBoy::plan::plan(std::vector<std::string> urls) : urls(urls) {
    if (!VertretungsBoy::plan::curlGlobalInit) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        VertretungsBoy::plan::curlGlobalInit = true;
    }

    curlDownloads.resize(urls.size());
}

VertretungsBoy::plan::plan(std::string url) {
    urls.push_back(url);

    if (!VertretungsBoy::plan::curlGlobalInit) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        VertretungsBoy::plan::curlGlobalInit = true;
    }

    curlDownloads.resize(urls.size());
}

int VertretungsBoy::plan::update(){
    for(size_t i = 0; i<urls.size(); ++i){
        if(!download(i)) return 1;
    }

    return 0;
}

bool VertretungsBoy::plan::download(size_t urlsIndex) {
    CURL *handle = curl_easy_init();
    if(handle == nullptr)
        return  1;

    curl_easy_setopt(handle, CURLOPT_URL, urls[urlsIndex].c_str());
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writer);

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, &curlDownloads[urlsIndex]);

    CURLcode res = curl_easy_perform(handle);
    if(res != CURLE_OK)
        return false;

    curl_easy_cleanup(handle);

    return true;
}

size_t VertretungsBoy::plan::writer(char *ptr, size_t size, size_t nmemb, void *userdata) {
    ((std::string *) userdata) -> append(ptr, 0, size * nmemb);
    return size * nmemb;
}

bool VertretungsBoy::plan::parser(bool tag, size_t curlDownloadsIndex) {


    return true;
}
