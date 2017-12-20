//
// Created by pi on 12/20/17.
//

#ifndef VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
#define VERTRETUNGSBOY_VERTRETUNGSBOY_HPP

#include <string>

namespace VertretungsBoy
{
    class plan {

    public:
        plan(std::string *urls) : urls(urls) {};

    private:
        std::string *urls = nullptr;

    };
};

#endif //VERTRETUNGSBOY_VERTRETUNGSBOY_HPP
